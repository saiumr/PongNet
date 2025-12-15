#include "game.h"
#include "network_manager.h"
#include "protocol.h"

struct PlayerMatch {
    PlayerId id;
    int      match_player_index;
};

int main(int argc, char* argv[]) {
    NetworkManager nm;
    
    const bool is_server_started { nm.StartServer(9527) };
    const Clients& cs { nm.get_clients() };
    std::vector<PlayerMatch> cs_match;
    cs_match.reserve(8);

    nm.HandleClientDisconnectedCallback = [&cs_match](int index) -> void {
        int matched_index { cs_match[index].match_player_index };
        if (matched_index != -1) {
            // someone disconnected, then matched player no friend
            cs_match[matched_index].match_player_index = -1;
        }
        cs_match.erase(cs_match.begin() + index);  // sync clients erase
    };

    while (is_server_started) {
        bool is_new_connection { nm.AcceptClients() };  // here emplace_back new connection client
        int  last_index { static_cast<int>(cs.size() - 1) };
        // The client with even index is p1, and the client with odd index is p2
        if (is_new_connection) {
            InitMsg init_msg;
            SDL_Log("cs size: %d", cs.size());
            init_msg.p_id = last_index%2 == 0 ? PlayerId::kPlayer1 : PlayerId::kPlayer2;
            cs_match.emplace_back( PlayerMatch { init_msg.p_id, -1 } );
            for (int i = 0; i < cs.size() && cs.size() > 1; ++i) {
                if (cs_match[i].match_player_index == -1) {
                    cs_match[i].match_player_index = last_index;
                    cs_match[last_index].match_player_index = i;
                    cs_match[last_index].id = cs_match[i].id == PlayerId::kPlayer1 ? PlayerId::kPlayer2 : PlayerId::kPlayer1;
                }
                SDL_Log("cs_match[%d]: id - %d, matched_index - %d", i, cs_match[i].id, cs_match[i].match_player_index);
            }
            nm.SendToClient(last_index, &init_msg, sizeof(init_msg));
        }

        nm.PollClients([&cs_match, &nm](int index, const void* data, int size) -> void {
            int matched_index { cs_match[index].match_player_index };
            if (matched_index != -1) {
                nm.SendToClient(matched_index, data, size);
            }
        });

        SDL_Delay(16);
    }

    return 0;
}