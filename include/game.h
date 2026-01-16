#pragma once

#include <SDL3/SDL.h>
#include <memory>
#include <functional>
#include <mutex>
#include <queue>
#include <optional>
#include "protocol.h"

// ball
struct RectObject {
    SDL_FRect body;
    float speed;
    SDL_Color color;
};

struct Player {
    SDL_FRect body;
    float speed;
    SDL_Color color;
};

struct SDLDeleter {
    void operator()(SDL_Window* w) const { if (w) SDL_DestroyWindow(w); }
    void operator()(SDL_Renderer* r) const { if(r) SDL_DestroyRenderer(r); }
};
using WindowPtr = std::unique_ptr<SDL_Window, SDLDeleter>;
using RendererPtr = std::unique_ptr<SDL_Renderer, SDLDeleter>;

constexpr SDL_Color PLAYER_COLOR { 255, 255, 255, 255 };
constexpr SDL_Color OBJECT_COLOR { 255, 255, 255, 255 };
constexpr SDL_Color BG_COLOR     { 0, 0, 0, 255 };
constexpr float WINDOW_WIDTH  { 800.0f };
constexpr float WINDOW_HEIGHT { 600.0f };
constexpr float PLAYER_WIDTH  { 45.0f  };
constexpr float PLAYER_HEIGHT { 150.0f };
constexpr float PLAYER_SPEED  { 300.0f };
constexpr float BALL_WIDTH    { 50.0f  };
constexpr float BALL_HEIGHT   { 50.0f  };
constexpr float BALL_SPEED    { 200.0f };

class Game {
private:
    RectObject  rect_object_;
    Player      player1_;
    Player      player2_;

    PlayerId    player_id_;

    WindowPtr   window_;
    RendererPtr renderer_;
    bool        running_;

    uint8_t     state_mask_;  // player control w/s and up/down key control player1/2 up/down, on BIT 0/1/2/3
                              // rect object, BIT 4/5 =1 is x/y position direction.
    std::queue<NetEvent> net_events_;
    std::mutex           net_event_mutex_;

    std::optional<GameStateMsg> latest_server_state_;
    std::mutex server_state_mutex_;

    bool        is_online_;
    float       fps_;

    // smooth animation
    SDL_FRect render_ball_;
    float     render_p1_y_;
    float     render_p2_y_;

    void Init(const char* title);
    void HandleInput();
    void Update(float dt);
    void Render();
    void Cleanup();

    // online
    // each frame is processed once
    void ProcessNetEvents();
    
    // send an input message per 50 frames
    void PredictLocalPlayer(float dt);
    void InterpolateFromServer(float dt);

public:
    std::function<void()> Send2ServerCallback { nullptr };

    Game();
    ~Game();
    void Loop();
    
    const uint8_t get_state_mask() const;
    void set_state_mask(const uint8_t state_mask);

    const float get_fps() const;

    const RectObject get_rect_object() const;
    void  set_rect_object(const RectObject& r);

    const Player get_player(PlayerId id) const;
    void  set_player(PlayerId id, const Player& p);

    // online
    const bool get_is_online() const;
    void set_is_online(bool is_online);
    const PlayerId get_player_id() const;
    void set_player_id(PlayerId id);
    void AddNetEvent(const NetEvent& e);
};

bool AABB_Collision(const SDL_FRect& a, const SDL_FRect& b);
float Lerp(float a, float b, float t);
