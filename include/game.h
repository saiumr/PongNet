#pragma once

#include <SDL3/SDL.h>
#include <memory>

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

class Game {
private:
    RectObject  rect_object_;
    Player      player1_;
    Player      player2_;

    WindowPtr   window_;
    RendererPtr renderer_;
    bool        running_;

    uint8_t     state_mask_;  // player control w/s and up/down key control player1/2 up/down, on BIT 0/1/2/3
                              // rect object, BIT 4/5 =1 is x/y position direction.

    void HandleInput();
    void Update(float delta_time);
    void Render();
    void Cleanup();

public:
    Game() = default;
    ~Game();
    void Init(const char* title);
    void Run();
    const uint8_t get_state_mask() const;
    const float get_rect_object_speed() const;
    const float get_player_speed() const;
    void set_rect_object_speed(float speed);
    void set_player_speed(float speed);
};
