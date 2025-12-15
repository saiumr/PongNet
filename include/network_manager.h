#pragma once
#include <SDL3_net/SDL_net.h>
#include <thread>
#include <atomic>
#include <functional>
#include <mutex>
#include <vector>
#include <iostream>

using Clients = std::vector<NET_StreamSocket*>;

class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();
    NetworkManager(const NetworkManager&)            = delete;
    NetworkManager& operator=(const NetworkManager&) = delete;
    NetworkManager(NetworkManager&&)                 = delete;
    NetworkManager& operator=(NetworkManager&&)      = delete;


    // on client: init client and connect to server
    bool ConnectToServer(const char* ip, int port);

    // on server
    bool StartServer(int port);     // init server
    bool AcceptClients();           // call per frame in game
    void Broadcast(const void* data, int size); 
    void PollClients(std::function<void(int, const void*, int)> callback);  // callback parameters: index, data, data size
    const Clients& get_clients() const;

    // for client and server
    bool SendToServer(const void* data, int size);
    bool SendToClient(int client_index, const void* data, int size);

    // on client: client received message
    std::function<void(const void*, int)> HandleReceivedDataCallback;
    // on server: client[index] disconnected
    std::function<void(int)> HandleClientDisconnectedCallback;

private:
    void ClientReceiveLoop();

private:
    // for client
    std::atomic<bool> running_ { false };
    NET_StreamSocket* client_socket_  { nullptr };
    std::thread client_receive_thread_;
    std::mutex send_mutex_;

    // for server
    NET_Server* server_socket_ { nullptr };
    Clients clients_;
};

