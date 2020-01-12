#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdlib>
#include <thread>
#include <list>
#include <stack>
#include "classes/Player.h"
#include "classes/Client.h"
#include "classes/Score.h"
#include "../board_parameters.h"


int server();
void client_service(Client &client);
void server_game_service();
