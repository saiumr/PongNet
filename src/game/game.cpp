#include "game.h"

Game::Game() {
    Init("PongNet");
}

Game::~Game() {
    Cleanup();
}
void Game::Loop() {
    const float freq = 1.0f / (float)SDL_GetPerformanceFrequency();
    Uint64 prev = SDL_GetPerformanceCounter();
    while (running_) {
        HandleInput();
        if (is_online_) {
            ProcessNetEvents();  // eat server state firstly (latest server state + local prediction = current frame rendering)
            if(Send2ServerCallback) Send2ServerCallback();  // online, accept state_mask_ and position
        }

        Uint64 now = SDL_GetPerformanceCounter();
        float  dt  = (now - prev) * freq;
        prev       = now;
        if (dt > 0.0f) fps_ = 1.0f / dt;
        Update(dt);
        Render();

        SDL_Delay(16);
    }
}

void Game::ProcessNetEvents() {
    std::lock_guard<std::mutex> lock(net_event_mutex_);
    while (!net_events_.empty()) {
        auto& ne { net_events_.front() };

        switch (ne.type) {
        case MessageType::kInitMsg: {
            auto* msg { reinterpret_cast<InitMsg*>(ne.data.data()) };
            player_id_ = msg->p_id;
            if (player_id_ == PlayerId::kPlayer1) {
                player1_.color = SDL_Color { 0, 255, 0, 255 };
                SDL_Log("you are player 1 (left side), use w/s control move.");
            } else if(player_id_ == PlayerId::kPlayer2) {
                player2_.color = SDL_Color { 0, 255, 0, 255 };
                SDL_Log("you are player 2 (right side), use ↑/↓ control move.");
            }
            break;
        }
        // case MessageType::kPlayerInputMsg: {
        //     auto* msg { reinterpret_cast<PlayerInputMsg*>(ne.data.data()) };
        //     state_mask_ |= (msg->mask & 0x0F);
        //     break;
        // }
        case MessageType::kGameStateMsg: {
            auto* msg { reinterpret_cast<GameStateMsg*>(ne.data.data()) };
            std::lock_guard<std::mutex> lock(server_state_mutex_);
            latest_server_state_ = *msg;
            break;
        }
        default:
            break;
        }

        net_events_.pop();
    }
}

void Game::PredictLocalPlayer(float dt) {
    if (player_id_ == PlayerId::kPlayer1) {
        if (state_mask_ & (1 << 0))
            player1_.body.y -= player1_.speed * dt;
        if (state_mask_ & (1 << 1))
            player1_.body.y += player1_.speed * dt;
    } else {
        if (state_mask_ & (1 << 2))
            player2_.body.y -= player2_.speed * dt;
        if (state_mask_ & (1 << 3))
            player2_.body.y += player2_.speed * dt;
    }
}

// smooth animation
void Game::InterpolateFromServer(float dt) {
    std::lock_guard<std::mutex> lock(server_state_mutex_);
    if (!latest_server_state_) return;

    // calculate rtt
    rtt_ = SDL_GetTicks() - latest_server_state_->echo_client_time_ms;

    constexpr float SMOOTH { 0.32f };

    // ball
    render_ball_.x = Lerp(render_ball_.x, latest_server_state_->ball_x, SMOOTH);
    render_ball_.y = Lerp(render_ball_.y, latest_server_state_->ball_y, SMOOTH);

    // players
    render_p1_y_ = Lerp(render_p1_y_, latest_server_state_->p1_y, SMOOTH);
    render_p2_y_ = Lerp(render_p2_y_, latest_server_state_->p2_y, SMOOTH);
}

const uint8_t Game::get_state_mask() const { return state_mask_; }
void Game::set_state_mask(const uint8_t state_mask) {
    state_mask_ = state_mask;
}

const float Game::get_fps() const { return fps_; }

const RectObject Game::get_rect_object() const { return rect_object_; }
void  Game::set_rect_object(const RectObject& r) { rect_object_ = r; }

const Player Game::get_player(PlayerId id) const {
    if (id == PlayerId::kPlayer1) {
        return player1_;
    } else if (id == PlayerId::kPlayer2) {
        return player2_;
    }

    return player1_;
}

void  Game::set_player(PlayerId id, const Player& p) {
    if (id == PlayerId::kPlayer1) {
        player1_ = p;
    } else if (id == PlayerId::kPlayer2) {
        player2_ = p;
    }
}

// online
const bool Game::get_is_online() const { return is_online_; }
void Game::set_is_online(bool is_online) { is_online_ = is_online; }
const PlayerId Game::get_player_id() const { return player_id_; }
void Game::set_player_id(PlayerId id) { player_id_ = id; }
void Game::AddNetEvent(const NetEvent& e) {
    std::lock_guard<std::mutex> lock(net_event_mutex_);
    net_events_.push(std::move(e));
}



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
    rect_object_.speed = BALL_SPEED;
    rect_object_.body.w = BALL_WIDTH;
    rect_object_.body.h = BALL_HEIGHT;
    rect_object_.body.x = (WINDOW_WIDTH - rect_object_.body.w) / 2;
    rect_object_.body.y = (WINDOW_HEIGHT - rect_object_.body.h) / 2;

    player1_.color = player2_.color = PLAYER_COLOR;
    player1_.speed = player2_.speed = PLAYER_SPEED;
    player1_.body.w = player2_.body.w = PLAYER_WIDTH;
    player1_.body.h = player2_.body.h = PLAYER_HEIGHT;
    player1_.body.y = player2_.body.y = (WINDOW_HEIGHT - player1_.body.h) / 2;
    player1_.body.x = 0;
    player2_.body.x = WINDOW_WIDTH - player2_.body.w;

    // for smooth animation
    render_ball_ = rect_object_.body;
    render_p1_y_ = player1_.body.y;
    render_p2_y_ = player2_.body.y;

    player_id_ = PlayerId::kPlayer1;

    running_ = true;
    is_online_ = false;
    state_mask_ = 0;
}

void Game::Cleanup() {
    SDL_Quit();
}

void Game::RenderInfo() {
    static uint32_t last_render = SDL_GetTicks();
    uint32_t now = SDL_GetTicks();
    if (now - last_render < 3000) return;   // 3s

    // print as render
    SDL_Log("fps: %u, rtt: %u ms", fps_, rtt_);

    last_render = now;
}

void Game::Render() {
    SDL_FRect ball { rect_object_.body  };
    SDL_FRect p1   { player1_.body };
    SDL_FRect p2   { player2_.body }; 
    
    // smooth animation
    if (is_online_) {
        ball = render_ball_;
        p1.y = render_p1_y_;
        p2.y = render_p2_y_;
    }

    SDL_SetRenderDrawColor(renderer_.get(), BG_COLOR.r, BG_COLOR.g, BG_COLOR.b, BG_COLOR.a);
    SDL_RenderClear(renderer_.get());

    SDL_SetRenderDrawColor(renderer_.get(), rect_object_.color.r, rect_object_.color.g, rect_object_.color.b, rect_object_.color.a);
    SDL_RenderFillRect(renderer_.get(), &ball);
    SDL_SetRenderDrawColor(renderer_.get(), player1_.color.r, player1_.color.g, player1_.color.b, player1_.color.a);
    SDL_RenderFillRect(renderer_.get(), &p1);
    SDL_SetRenderDrawColor(renderer_.get(), player2_.color.r, player2_.color.g, player2_.color.b, player2_.color.a);
    SDL_RenderFillRect(renderer_.get(), &p2);

    RenderInfo();

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

    uint8_t input_mask { 0 };  // bit 0/1 for p1, bit 2/3 for p2
    const bool* keys { SDL_GetKeyboardState(nullptr) };
    if (keys[SDL_SCANCODE_W])       input_mask |= 1 << 0;
    if (keys[SDL_SCANCODE_S])       input_mask |= 1 << 1;
    if (keys[SDL_SCANCODE_UP])      input_mask |= 1 << 2;
    if (keys[SDL_SCANCODE_DOWN])    input_mask |= 1 << 3;

    state_mask_ &= 0xF0;       // keep rect object move, reset player move state

    if (is_online_) {
        switch (player_id_) {
        case PlayerId::kPlayer1:
            state_mask_ |= (input_mask & 0x03);  // just update local p1
            break;
        case PlayerId::kPlayer2:
            state_mask_ |= (input_mask & 0x0C);  // update local p2
            break;
        default:
            break;
        }
    } else {
        state_mask_ |= input_mask;
    }
}

bool AABB_Collision(const SDL_FRect& a, const SDL_FRect& b) {
    return a.x < b.x + b.w &&
           a.x + a.w > b.x &&
           a.y < b.y + b.h &&
           a.y + a.h > b.y;
}

float Lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

void Game::Update(float dt) {
    if (is_online_) {
        PredictLocalPlayer(dt);     // just process self prediction
        InterpolateFromServer(dt);  // use server state
    } else {
        // players
        if (state_mask_ & (1 << 0)) {
            player1_.body.y -= player1_.speed * dt;
        } 
        if (state_mask_ & (1 << 1)) {
            player1_.body.y += player1_.speed * dt;
        }
        if (state_mask_ & (1 << 2)) {
            player2_.body.y -= player2_.speed * dt;
        } 
        if (state_mask_ & (1 << 3)) {
            player2_.body.y += player2_.speed * dt;
        }

        // rect object left/right move 
        if (state_mask_ & (1 << 4)) {
            rect_object_.body.x += rect_object_.speed * dt;
        } else {
            rect_object_.body.x -= rect_object_.speed * dt;
        }
        // rect object up/down move   
        if (state_mask_ & (1 << 5)) {
            rect_object_.body.y += rect_object_.speed * dt;
        } else {
            rect_object_.body.y -= rect_object_.speed * dt;
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
}
