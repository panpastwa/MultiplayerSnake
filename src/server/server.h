#include <SFML/Graphics.hpp>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>

int server();
void client_service(int sock);
void client_menu_service(int sock);
void client_game_service(int sock);
