#include <SFML/Graphics.hpp>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdlib>
#include <thread>
#include <list>
#include <stack>
#include "Player.h"
#include "Client.h"

int server();
void client_service(Client &client);
void server_game_service();
