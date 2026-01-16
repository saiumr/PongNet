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
            static uint32_t last_send = SDL_GetTicks();

            uint32_t now = SDL_GetTicks();
            if (now - last_send < 50) return; // 20Hz

            PlayerInputMsg msg;
            msg.tick = 0;
            msg.mask = game.get_state_mask();
            msg.p_id = game.get_player_id();

            nm.SendToServer(&msg, sizeof(msg));
            last_send = now;
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
