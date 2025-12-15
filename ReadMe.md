# PongNet  

A simple net pong game.  

## Compile  

I make this project on windows, use SDL3 and MinGW.

```bash
cmake -B build -S . -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
cd build/Debug
./PongNet.exe
```

## PongNet network design

PongNet(Client)

- get input state (up/down)
- send input state to server
- receive server broadcast message (object + players)
- update game data

PontNet(Server)

- collecting input state came from two clients
- update object move and collision
- update the position of  player1 and player2
- broadcast position to two clients
