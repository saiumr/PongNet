#include "network_manager.h"

NetworkManager::NetworkManager() {
    running_ = true;
    if (!NET_Init()) {
        SDL_Log("NET_Init failed: %s", SDL_GetError());
        running_ = false;
    }
}

NetworkManager::~NetworkManager() {
    running_ = false;
    if (client_receive_thread_.joinable()) {
        client_receive_thread_.join();  // >= c++11, you can use std::jthread since c++20
    }

    if (client_socket_) {
        NET_DestroyStreamSocket(client_socket_);
        client_socket_ = nullptr;
    }

    for (auto& c: clients_) {
        if (c) {
            NET_DestroyStreamSocket(c);
            c = nullptr;
        }
    }

    if (server_socket_) {
        NET_DestroyServer(server_socket_);
        server_socket_ = nullptr;
    }

    NET_Quit();
}

bool NetworkManager::ConnectToServer(const char* ip, int port) {
    if (!running_) return false;  // init failed
    running_ = false;

    NET_Address* address = NET_ResolveHostname(ip);
    if (!address) {
        SDL_Log("Resolve hostname failed: %s", SDL_GetError());
        return false;
    }

    if (NET_WaitUntilResolved(address, -1) != NET_SUCCESS) {
        NET_UnrefAddress(address);
        return false;
    }

    client_socket_ = NET_CreateClient(address, port);
    NET_UnrefAddress(address);
    if (!client_socket_) {
        SDL_Log("Create socket failed: %s", SDL_GetError());
        return false;
    }

    SDL_Log("connecting...");
    if (NET_WaitUntilConnected(client_socket_, 2000) != NET_SUCCESS) {
        SDL_Log("connect to server failed!");
        NET_DestroyStreamSocket(client_socket_);
        client_socket_ = nullptr;
        return false;
    }

    SDL_Log("connect to server success!");
    running_ = true;
    client_receive_thread_ = std::thread(&NetworkManager::ClientReceiveLoop, this);

    return true;
}

void NetworkManager::ClientReceiveLoop() {
    char buf[1024];

    while (running_) {
        void* s[1] { client_socket_ };
        // wait new connection
        if (NET_WaitUntilInputAvailable(s, 1, 0) > 0) {
            int r { NET_ReadFromStreamSocket(client_socket_, buf, sizeof(buf)) };
            if (r > 0) {
                if (HandleReceivedDataCallback) HandleReceivedDataCallback(buf, r);
            }
        }
    }
}

bool NetworkManager::SendToServer(const void* data, int size) {
    if (!client_socket_) return false;
    std::lock_guard<std::mutex> lock(send_mutex_);
    return NET_WriteToStreamSocket(client_socket_, data, size) == size; 
}

bool NetworkManager::StartServer(int port) {
    server_socket_ = NET_CreateServer(nullptr, port);  // nullptr <=> "0.0.0.0" <=> "127.0.0.1", bind to all interfaces
    if (!server_socket_) {
        SDL_Log("Create server socket failed!");
        return false;
    }
    return true;
}

bool NetworkManager::AcceptClients() {
    NET_StreamSocket* c { nullptr };
    if (NET_AcceptClient(server_socket_, &c)) {
        if (c) {
            clients_.emplace_back(c);
            SDL_Log("new connection added!");
            SDL_Log("now clients: %d", clients_.size());
            return true;
        }
    }

    return false;
}

void NetworkManager::Broadcast(const void* data, int size) {
    for (auto c : clients_)
        NET_WriteToStreamSocket(c, data, size);
}

bool NetworkManager::SendToClient(int client_index, const void* data, int size) {
    if (client_index < 0 || client_index >= static_cast<int>(clients_.size())) return false;
    return NET_WriteToStreamSocket(clients_[client_index], data, size) == size;
}

void NetworkManager::PollClients(std::function<void(int, const void*, int)> callback) {
    char buf[1024];

    for (int i = 0; i < clients_.size(); ++i) {
        void* s[1] { clients_[i] };

        if (NET_WaitUntilInputAvailable(s, 1, 0) > 0) {
            int r { NET_ReadFromStreamSocket(clients_[i], buf, sizeof(buf)) };
            if (r <= 0) {
                NET_DestroyStreamSocket(clients_[i]);
                clients_.erase(clients_.begin() + i);
                SDL_Log("a client disconnected.");
                if (HandleClientDisconnectedCallback) HandleClientDisconnectedCallback(i);
                i--;
            } else {
                callback(i, buf, r);
            }
        }
    }
}

const Clients& NetworkManager::get_clients() const {
    return clients_;
}
