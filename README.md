# MultiplayerSnake
Classic snake for 4 players using client-server communication.

STILL IN DEVELOPMENT.

## How to build and run?
Use CMake
```
mkdir build
cd build
cmake ..
make
cd ..
./build/ServerApp <port_num>
./build/ClientApp <ip_addr> <port_num>
```
It's important that you should run executable from project root directory.
In other case there will be errors while loading assets from data folder.
