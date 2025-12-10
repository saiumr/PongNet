#include "game.h"
#include <SDL3_net/SDL_net.h>

int main(int argc, char* argv[]) {
    Game game;
    game.Init("PontNet(Client)");

    NET_Init();
    SDL_Log("hello pong net!");

    game.Run();

    NET_Quit();

    return 0;
}
