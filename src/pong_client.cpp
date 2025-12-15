#include "game.h"
#include "network_manager.h"
#include "protocol.h"

int main(int argc, char* argv[]) {
    Game game;
    NetworkManager nm;

    SDL_Log("Welcome to the PongNet!");
    SDL_Log("Use W/S or ↑/↓ arrow keys move up/down.");

    bool is_connected  { nm.ConnectToServer("127.0.0.1", 9527) };

    // set online callback
    if (is_connected) {
        game.set_is_online(true);

        game.Send2ServerCallback = [&game, &nm]() -> void {
            static int count { 0 };
            if (++count >= 30) {
                count = 0;
                // just p1 send real position reduce bias per 30 frames
                if (game.get_player_id() == PlayerId::kPlayer1) {
                    GameStateMsg gs_msg;
                    gs_msg.ball_x = game.get_rect_object().body.x;
                    gs_msg.ball_y = game.get_rect_object().body.y;
                    gs_msg.p_id   = game.get_player_id();
                    gs_msg.p_y    = game.get_player(PlayerId::kPlayer1).body.y;
                    nm.SendToServer(&gs_msg, sizeof(gs_msg));
                }
            } else {
                PlayerInputMsg pi_msg;
                pi_msg.mask = game.get_state_mask();
                nm.SendToServer(&pi_msg, sizeof(pi_msg));
            }
            
        };

        // just accept messages in handle receive thread, do not set game state
        nm.HandleReceivedDataCallback = [&game](const void* data, size_t size) {
            NetEvent ev;
            ev.type = static_cast<MessageType>(*reinterpret_cast<const uint8_t*>(data));
            ev.data.assign((uint8_t*)data, (uint8_t*)data + size);
            game.AddNetEvent(ev);
        };
    }

    game.Loop();

    return 0;
}
