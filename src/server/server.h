#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdlib>
#include <thread>
#include <mutex>
#include <list>
#include <stack>
#include "classes/Player.h"
#include "classes/Client.h"
#include "classes/Score.h"
#include "../board_parameters.h"


void server(int port_num);
void client_service(Client &client);
void server_game_service();
