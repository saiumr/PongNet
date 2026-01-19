#include "game.h"
#include "network_manager.h"
#include "protocol.h"
#include <algorithm>

constexpr float SERVER_DT    { 1.0f / 30.0f }; // 30Hz

static SDL_FRect p1_body   { 0.0f, 0.0f, PLAYER_WIDTH, PLAYER_HEIGHT };
static SDL_FRect p2_body   { WINDOW_WIDTH - PLAYER_WIDTH, 0.0f, PLAYER_WIDTH, PLAYER_HEIGHT };
static SDL_FRect ball_body { 0.0f, 0.0f, BALL_WIDTH, BALL_HEIGHT };

struct ServerPlayer {
    float y { (WINDOW_HEIGHT - 150.0f) / 2.0f };
    uint8_t input_mask { 0 };
};

struct ServerBall {
    float x, y;
    float vx, vy;
};

struct ServerGameState {
    float ball_x  { (WINDOW_WIDTH  - 50.0f) / 2.0f };
    float ball_y  { (WINDOW_HEIGHT - 50.0f) / 2.0f };

    float ball_vx { BALL_SPEED };
    float ball_vy { BALL_SPEED };

    ServerPlayer p1;
    ServerPlayer p2;

    Tick tick;
};

struct PlayerMatch {
    PlayerId id;
    int      match_player_index;
};

void UpdateServerGame(ServerGameState& gs) {
    // player 1
    if (gs.p1.input_mask & (1 << 0))
        gs.p1.y -= PLAYER_SPEED * SERVER_DT;
    if (gs.p1.input_mask & (1 << 1))
        gs.p1.y += PLAYER_SPEED * SERVER_DT;

    // player 2
    if (gs.p2.input_mask & (1 << 2))
        gs.p2.y -= PLAYER_SPEED * SERVER_DT;
    if (gs.p2.input_mask & (1 << 3))
        gs.p2.y += PLAYER_SPEED * SERVER_DT;

    // clamp players
    gs.p1.y = std::clamp(gs.p1.y, 0.0f, WINDOW_HEIGHT - PLAYER_HEIGHT);
    gs.p2.y = std::clamp(gs.p2.y, 0.0f, WINDOW_HEIGHT - PLAYER_HEIGHT);

    // ball move
    gs.ball_x += gs.ball_vx * SERVER_DT;
    gs.ball_y += gs.ball_vy * SERVER_DT;

    // player collision
    p1_body.y = gs.p1.y;
    p2_body.y = gs.p2.y;
    ball_body.x = gs.ball_x;
    ball_body.y = gs.ball_y;
    if (AABB_Collision(p1_body, ball_body)) {
        gs.ball_vx = -gs.ball_vx;
    } else if (AABB_Collision(p2_body, ball_body)) {
        gs.ball_vx = -gs.ball_vx;
    }

    // wall collision
    if (gs.ball_y < 0 || gs.ball_y > WINDOW_HEIGHT - PLAYER_WIDTH)
        gs.ball_vy = -gs.ball_vy;

    if (gs.ball_x < 0 || gs.ball_x > WINDOW_WIDTH - PLAYER_WIDTH)
        gs.ball_vx = -gs.ball_vx;

    gs.tick++;
}

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

    ServerGameState gs;
    gs.tick = 0;
    while (is_server_started) {
        // first player is p1, second p2, they are matched together
        bool is_new_connection { nm.AcceptClients() };  // here emplace_back new connection client
        int  last_index { static_cast<int>(cs.size() - 1) };
        // The client with even index is p1, and the client with odd index is p2
        if (is_new_connection) {
            InitMsg init_msg;
            SDL_Log("clients size: %d", cs.size());
            init_msg.tick = gs.tick;
            init_msg.p_id = last_index%2 == 0 ? PlayerId::kPlayer1 : PlayerId::kPlayer2;
            cs_match.emplace_back( PlayerMatch { init_msg.p_id, -1 } );
            for (int i = 0; i < cs.size() && cs.size() > 1; ++i) {
                if (cs_match[i].match_player_index == -1) {
                    cs_match[i].match_player_index = last_index;
                    cs_match[last_index].match_player_index = i;
                    cs_match[last_index].id = cs_match[i].id == PlayerId::kPlayer1 ? PlayerId::kPlayer2 : PlayerId::kPlayer1;
                }
                SDL_Log("clients match[%d]: id - %d, matched_index - %d", i, cs_match[i].id, cs_match[i].match_player_index);
            }
            nm.SendToClient(last_index, &init_msg, sizeof(init_msg));
        }

        nm.PollClients([&cs_match, &nm, &gs](int index, const void* data, int size) -> void {
            // receive input message
            auto* msg { reinterpret_cast<const PlayerInputMsg*>(data) };
            auto& p   { (msg->p_id == PlayerId::kPlayer1) ? gs.p1 : gs.p2 };
            p.input_mask = msg->mask;

            // update world state
            UpdateServerGame(gs);

            // convey world state to p1 and p2(they are in a same world)
            int matched_index { cs_match[index].match_player_index };
            if (matched_index != -1) {
                GameStateMsg s;
                s.tick   = gs.tick;
                s.echo_client_time_ms = msg->client_time_ms;  // echo time, assist the client in determining the timing of its own message sending
                s.ball_x = gs.ball_x;
                s.ball_y = gs.ball_y;
                s.p1_y   = gs.p1.y;
                s.p2_y   = gs.p2.y;
                nm.SendToClient(index, &s, sizeof(s));
                nm.SendToClient(matched_index, &s, sizeof(s));
            }
        });

        gs.tick++;
        SDL_Delay(33);
    }

    return 0;
}