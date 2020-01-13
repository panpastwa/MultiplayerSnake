#include <SFML/Graphics.hpp>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <string>
#include "../board_parameters.h"

void client(char *ip_addr, int port_num);
void send_key_to_server(sf::Keyboard::Key key, int server_sock);
void enter_nickname(sf::RenderWindow &window, int sock);
void menu(sf::RenderWindow &window, int sock);
void queue(sf::RenderWindow &window, int sock);
void game(sf::RenderWindow &window, int sock);
void score(sf::RenderWindow &window, int sock);
void update_game_state(int sock);
