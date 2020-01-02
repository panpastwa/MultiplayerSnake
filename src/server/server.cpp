#include "server.h"

// Number of currently connected clients to server
int num_of_connected_clients = 0;

// Array with socket numbers of connected clients and boolean array
int connected_clients[16];
bool is_slot_empty[16];
std::thread client_threads[16];

// Board
const int M = 30, N = 20;
char board[N][M];

// Players in game
int num_of_players_in_game = 0;
std::list<Player> current_players = {};
std::stack<int> available_player_numbers;

// Queue
std::list<int> queue = {};

int server()
{
    // Create socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1){
        perror("Socket error");
        exit(-1);
    }

    // Create structure for any clients with port number
    struct sockaddr_in server_structure = {AF_INET, htons(8888), INADDR_ANY};

    // Binding server socket with sockaddr structure
    int error = bind(server_socket, (sockaddr*)&server_structure, sizeof(server_structure));
    if (error == -1){
        perror("Bind error");
        exit(-1);
    }

    // Listen for incoming clients
    error = listen(server_socket, 16);
    if (error == -1){
        perror("Listen error");
        exit(-1);
    }

    // Push available player numbers onto stack
    available_player_numbers.push(1);
    available_player_numbers.push(2);
    available_player_numbers.push(3);
    available_player_numbers.push(4);

    // Start server game thread
    std::thread t(server_game_service);

    // Wait for clients
    while (num_of_connected_clients < 16){

        struct sockaddr_in client_structure;
        socklen_t size_of_client_structure = sizeof(client_structure);
        printf("Server is ready to accept a new connection\n");
        int client_socket = accept(server_socket, (sockaddr*)&client_structure, &size_of_client_structure);
        if (client_socket == -1){
            perror("Accept error");
            exit(-1);
        }
        // Print client's ip address and port number
        printf("New client connected %s : %d\n", inet_ntoa(client_structure.sin_addr), ntohs(client_structure.sin_port));

        // Add client to connected clients
        connected_clients[num_of_connected_clients] = client_socket;
        is_slot_empty[num_of_connected_clients] = false;

        // Start client thread
        client_threads[num_of_connected_clients] = std::thread(client_service, client_socket);
        printf("Client thread called\n");

        num_of_connected_clients++;

    }

    return 0;

}

void client_service(int sock){

    // Client thread
    while (true) {

        // Read message from client
        char data[1024];
        int num_read_bytes = read(sock, data, sizeof(data));

        // Handle possible error
        if (num_read_bytes == -1){
            perror("Read error in client's main loop");
            exit(-1);
        }

        // Read 0 bytes - client disconnects
        else if (num_read_bytes == 0){

            printf("Client %d disconnecting\n", sock);

            // Close the connection
            int error = shutdown(sock, SHUT_RDWR);
            if (error == -1){
                perror("Shutdown error");
                exit(-1);
            }

            // Find correct index in array
            int index = -1;
            for (int i=0; i<16; ++i)
                if (connected_clients[i] == sock)
                    index = i;
            if (index == -1) {
                printf("Could not find correct index in clients array");
                exit(-1);
            }

            // Update values in arrays
            is_slot_empty[index] = true;
            connected_clients[index] = 0;
            num_of_connected_clients--;
            printf("Client %d: Successfuly disconnected from server", sock);

            // Finish client thread
            printf("Client %d: End of thread", sock);
            return;
        }

        // Check if message has correct size for program purposes
        else if (num_read_bytes != 1024){
            printf("Client %d: Message does not have correct size %d\n", sock, num_read_bytes);
            write(1, data, num_read_bytes);
            exit(-1);
        }

        // Client wants to join game
        if (data[0] == '1'){

            printf("Client %d: Wants to join the game\n", sock);

            // If there is enough space for new player --> join game
            if (num_of_players_in_game < 4){

                // Add new player to the game
                num_of_players_in_game++;
                int players_number = available_player_numbers.top();
                available_player_numbers.pop();
                int starting_x = rand() % N;
                int starting_y = rand() % M;
                current_players.push_back(Player(sock, players_number, starting_x, starting_y));
                printf("Client %d: Joined the game\n", sock);

            } else {

                // Add to queue
                queue.push_back(sock);
            }
        }

        // Unknown first character
        else {
            printf("Client %d: Unknown action\n", sock);
            exit(-1);
        }

        // Game loop
        while (true){

            // Read message from client
            num_read_bytes = read(sock, data, sizeof(data));

            // Handle possible error
            if (num_read_bytes == -1){
                perror("Read error in client's game loop");
                exit(-1);
            }

            // Read 0 bytes - client disconnects
            else if (num_read_bytes == 0){

                printf("Client %d disconnecting\n", sock);

                // Close the connection
                int error = shutdown(sock, SHUT_RDWR);
                if (error == -1){
                    perror("Shutdown error");
                    exit(-1);
                }

                // Find correct index in array
                int index = -1;
                for (int i=0; i<16; ++i)
                    if (connected_clients[i] == sock)
                        index = i;
                if (index == -1) {
                    printf("Could not find correct index in clients array");
                    exit(-1);
                }

                // Delete from game players list
                for (Player player : current_players){
                    if (player.socket_num == sock){
                        current_players.remove(player);
                        break;
                    }
                }
                num_of_players_in_game--;

                // Update values in arrays
                is_slot_empty[index] = true;
                connected_clients[index] = 0;
                num_of_connected_clients--;
                printf("Client %d: Successfuly disconnected from server", sock);


                // Finish client thread
                printf("Client %d: End of thread", sock);
                return;
            }

            // Check if message has correct size for program purposes
            else if (num_read_bytes != 1024){
                printf("Client %d: Message does not have correct size %d\n", sock, num_read_bytes);
                write(1, data, num_read_bytes);
                exit(-1);
            }

            // Client sends key to server
            if (data[0] == 'K'){
                int key_num = data[4];
                printf("Client %d: Send key %d to server\n", sock, key_num);
                for (Player &player : current_players){
                    if (player.socket_num == sock){
                        player.move_direction = key_num;
                        printf("Client %d: New direction %d\n", player.socket_num, player.move_direction);
                    }
                }
            }

            // Clients replies to message about losing --> Client lost and backs to menu
            else if (data[0] == 'L'){
                printf("Client %d: Knows about lost game and is back in menu\n", sock);
                break;
            }

            // Unknown first character
            else {
                printf("Client %d: Unknown action\n", sock);
                exit(-1);
            }
        }
    }
}


void server_game_service(){

    Point bonus(rand()%N, rand()%M);

    while (true){

        // While someone is in the game
        if (num_of_players_in_game > 0) {

            for (Player &player : current_players) {

                Point point = player.list_of_points.front();
                int x, y;
                if (player.move_direction == 0){
                    // left
                    x = point.x;
                    y = point.y - 1;
                } else if (player.move_direction == 1){
                    // right
                    x = point.x;
                    y = point.y + 1;
                } else if (player.move_direction == 2){
                    // up
                    x = point.x - 1;
                    y = point.y;
                } else if (player.move_direction == 3){
                    // down
                    x = point.x + 1;
                    y = point.y;
                } else {
                    printf("Wrong direction\n");
                    exit(-1);
                }

                if (x == bonus.x && y == bonus.y){
                    bonus.x = rand() % N;
                    bonus.y = rand() % M;
                }
                else {
                    player.list_of_points.pop_back();
                }

                if (x >= 0 && x < N && y >= 0 && y < M){
                    player.list_of_points.push_front(Point(x, y));
                    for (Point &p : player.list_of_points){
                        board[p.x][p.y] = player.number;
                    }
                }
                else {
                    // Player loses
                    printf("Player %d lost\n", player.socket_num);
                    write(player.socket_num, "L", 1024);
                    available_player_numbers.push(player.number);
                    current_players.remove(player);
                    num_of_players_in_game--;
                    if (num_of_players_in_game == 0){
                        printf("Game thread ended\n");
                        break;
                    }
                }
            }

            // Apple bonus
            board[bonus.x][bonus.y] = 5;

            // Send state of game
            char msg[1024];
            msg[0] = 'B';
            for (int i=0; i<N; ++i) {
                for (int j = 0; j < M; ++j) {
                    msg[i*M + j + 1] = board[i][j];
                    board[i][j] = 0;
                }
            }

            for (Player &player : current_players) {
                for (Point &p : player.list_of_points){
                    printf("x: %d\ty: %d\n", p.x, p.y);
                }
                write(player.socket_num, msg, 1024);
            }
        }

        else {
            // todo
            // some conditional waiting for new player instead of active waiting
            ;
        }

        usleep(300000);
    }

}
