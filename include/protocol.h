#pragma once

#include "common.h"

using Tick = uint32_t;

enum class MessageType : uint8_t {
    kInitMsg,
    kPlayerInputMsg,
    kGameStateMsg
};

// hand shake message
struct InitMsg {
    MessageType msg_type { MessageType::kInitMsg };
    Tick     tick;
    PlayerId p_id;
};

// send input mask per frame
struct PlayerInputMsg {
    MessageType msg_type { MessageType::kPlayerInputMsg };
    Tick      tick;      // input moment
    Tick      client_time_ms;
    uint8_t   mask;      // bit 0/1/2/3 p1/2 up/down
    PlayerId  p_id;
};

// the whole key datas (regularly send to correct deviations)
// world state(server keep)
struct GameStateMsg {
    MessageType msg_type { MessageType::kGameStateMsg };
    Tick  tick;    // server authentic tick
    Tick  echo_client_time_ms;
    float ball_x;
    float ball_y;
    float p1_y;
    float p2_y;
};

// transfrom every message type to byte streams
struct NetEvent {
    MessageType type;
    std::vector<uint8_t> data;
};
