#include <SDL3/SDL.h>
#include <SDL3_net/SDL_net.h>

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_EVENTS);
    NET_Init();
    SDL_Log("hello pont server!");
    NET_Quit();
    SDL_Quit();
    return 0;
}
