#pragma once

#include "common.h"

enum class MessageType : uint8_t {
    kInitMsg,
    kPlayerInputMsg,
    kGameStateMsg
};

// hand shake message
struct InitMsg {
    MessageType msg_type { MessageType::kInitMsg };
    PlayerId p_id;
};

// send input mask per frame
struct PlayerInputMsg {
    MessageType msg_type { MessageType::kPlayerInputMsg };
    uint8_t   mask;      // bit 0/1/2/3 p1/2 up/down
};

// the whole key datas (regularly send to correct deviations)
struct GameStateMsg {
    MessageType msg_type { MessageType::kGameStateMsg };
    float ball_x, ball_y;
    PlayerId p_id;
    float p_y;
};

// transfrom every message type to byte streams
struct NetEvent {
    MessageType type;
    std::vector<uint8_t> data;
};
