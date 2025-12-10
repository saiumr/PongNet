#include "game.h"

Game::~Game() {
    Cleanup();
}
void Game::Run() {
    Uint64 prev_time = SDL_GetPerformanceCounter();
    while (running_) {
        HandleInput();

        Uint64 current_time = SDL_GetPerformanceCounter();
        float delta_time = (current_time - prev_time) / (float)SDL_GetPerformanceFrequency();
        prev_time = current_time;
        Update(delta_time);
        Render();
    }
}

const uint8_t Game::get_state_mask() const { return state_mask_; }
const float Game::get_rect_object_speed() const { return rect_object_.speed; }
const float Game::get_player_speed() const { return player1_.speed; }
void Game::set_rect_object_speed(float speed) { rect_object_.speed = speed; }
void Game::set_player_speed(float speed) { player1_.speed = speed; }

void Game::Init(const char* title) {
    running_ = false;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Init SDL failed! Log: %s", SDL_GetError());
        return;
    }

    SDL_Window*   w { nullptr };
    SDL_Renderer* r { nullptr };
    if (!SDL_CreateWindowAndRenderer(title, static_cast<int>(WINDOW_WIDTH), static_cast<int>(WINDOW_HEIGHT), SDL_WINDOW_RESIZABLE, &w, &r)) {
        SDL_Log("Create window and renderer failed! Log: %s", SDL_GetError());
        return;
    }
    window_.reset(w);
    renderer_.reset(r);

    rect_object_.color = OBJECT_COLOR;
    rect_object_.speed = 200;
    rect_object_.body.w = 50;
    rect_object_.body.h = 50;
    rect_object_.body.x = (WINDOW_WIDTH - rect_object_.body.w) / 2;
    rect_object_.body.y = (WINDOW_HEIGHT - rect_object_.body.h) / 2;

    player1_.color = player2_.color = PLAYER_COLOR;
    player1_.speed = player2_.speed = 300;
    player1_.body.w = player2_.body.w = 45;
    player1_.body.h = player2_.body.h = 150;
    player1_.body.y = player2_.body.y = (WINDOW_HEIGHT - player1_.body.h) / 2;
    player1_.body.x = 0;
    player2_.body.x = WINDOW_WIDTH - player2_.body.w;

    running_ = true;
}

void Game::Cleanup() {
    SDL_Quit();
}

void Game::Render() {
    SDL_SetRenderDrawColor(renderer_.get(), BG_COLOR.r, BG_COLOR.g, BG_COLOR.b, BG_COLOR.a);
    SDL_RenderClear(renderer_.get());

    SDL_SetRenderDrawColor(renderer_.get(), rect_object_.color.r, rect_object_.color.g, rect_object_.color.b, rect_object_.color.a);
    SDL_RenderFillRect(renderer_.get(), &rect_object_.body);
    SDL_SetRenderDrawColor(renderer_.get(), player1_.color.r, player1_.color.g, player1_.color.b, player1_.color.a);
    SDL_RenderFillRect(renderer_.get(), &player1_.body);
    SDL_SetRenderDrawColor(renderer_.get(), player2_.color.r, player2_.color.g, player2_.color.b, player2_.color.a);
    SDL_RenderFillRect(renderer_.get(), &player2_.body);

    SDL_RenderPresent(renderer_.get());
}

void Game::HandleInput() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            running_ = false;
        }

        if (event.type == SDL_EVENT_KEY_DOWN) {
            if (event.key.key == SDLK_ESCAPE) {
                running_ = false;
            }
        }
    }

    state_mask_ &= 0xF0;  // reset BIT 0/1/2/3
    const bool* keys { SDL_GetKeyboardState(nullptr) };
    if (keys[SDL_SCANCODE_W])       state_mask_ |= 1 << 0; 
    if (keys[SDL_SCANCODE_S])       state_mask_ |= 1 << 1; 
    if (keys[SDL_SCANCODE_UP])      state_mask_ |= 1 << 2; 
    if (keys[SDL_SCANCODE_DOWN])    state_mask_ |= 1 << 3; 
}

bool AABB_Collision(const SDL_FRect& a, const SDL_FRect& b) {
    return a.x < b.x + b.w &&
           a.x + a.w > b.x &&
           a.y < b.y + b.h &&
           a.y + a.h > b.y;
}

void Game::Update(float delta_time) {
    // players
    if (state_mask_ & (1 << 0)) {
        player1_.body.y -= player1_.speed * delta_time;
    } 
    if (state_mask_ & (1 << 1)) {
        player1_.body.y += player1_.speed * delta_time;
    }
    if (state_mask_ & (1 << 2)) {
        player2_.body.y -= player2_.speed * delta_time;
    } 
    if (state_mask_ & (1 << 3)) {
        player2_.body.y += player2_.speed * delta_time;
    }

    // rect object left/right move 
    if (state_mask_ & (1 << 4)) {
        rect_object_.body.x += rect_object_.speed * delta_time;
    } else {
        rect_object_.body.x -= rect_object_.speed * delta_time;
    }
    // rect object up/down move   
    if (state_mask_ & (1 << 5)) {
        rect_object_.body.y += rect_object_.speed * delta_time;
    } else {
        rect_object_.body.y -= rect_object_.speed * delta_time;
    }

    // player bound limit
    if (player1_.body.y < 0) player1_.body.y = 0;
    else if (player1_.body.y > WINDOW_HEIGHT - player1_.body.h) player1_.body.y = WINDOW_HEIGHT - player1_.body.h;
    if (player2_.body.y < 0) player2_.body.y = 0;
    else if (player2_.body.y > WINDOW_HEIGHT - player2_.body.h) player2_.body.y = WINDOW_HEIGHT - player2_.body.h;

    // rect object bound limit
    if (rect_object_.body.x < 0) {
        rect_object_.body.x = 0;
        state_mask_ |= (1<<4);
    } else if (rect_object_.body.x > WINDOW_WIDTH - rect_object_.body.w) {
        rect_object_.body.x = WINDOW_WIDTH - rect_object_.body.w;
        state_mask_ &= ~(1<<4);
    }
    if (rect_object_.body.y < 0) {
        rect_object_.body.y = 0;
        state_mask_ |= (1<<5);
    } else if (rect_object_.body.y > WINDOW_HEIGHT - rect_object_.body.h) {
        rect_object_.body.y = WINDOW_HEIGHT - rect_object_.body.h;
        state_mask_ &= ~(1<<5);
    }

    if (AABB_Collision(player1_.body, rect_object_.body)) {
        state_mask_ |= (1<<4);
    } else if (AABB_Collision(player2_.body, rect_object_.body)) {
        state_mask_ &= ~(1<<4);
    }

}
