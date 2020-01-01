#include <SFML/Graphics.hpp>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdlib>
#include <thread>
#include <list>
#include "Player.h"

int server();
void client_service(int sock);
void client_menu_service(int sock);
void client_game_service(int sock);
void server_game_service();
