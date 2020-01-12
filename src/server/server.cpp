#include "server.h"

// Number of currently connected clients to server
int num_of_connected_clients = 0;

// List with connected clients
std::list<Client> list_of_clients;

// Board
const int M = 30, N = 20;
char board[N][M];

// Number of players in game
int num_of_players_in_game = 0;

// List with active players
std::list<Player> current_players = {};

// Stack with available slots (numbers) in the game
std::stack<int> available_player_numbers;

// Queue
std::list<int> queue = {};

int server(){

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
    std::thread game_thread(server_game_service);

    // Wait for clients
    while (num_of_connected_clients < 16){

        struct sockaddr_in client_structure;
        socklen_t size_of_client_structure = sizeof(client_structure);
        printf("Server is ready to accept a new connection\n");

        // Accept new client
        int client_socket = accept(server_socket, (sockaddr*)&client_structure, &size_of_client_structure);
        if (client_socket == -1){
            perror("Accept error");
            exit(-1);
        }

        // Find inactive clients and join their thread
        std::list<Client> clients_to_be_joined_and_removed = {};
        for (Client &c : list_of_clients){
            if (!c.is_active){
                printf("Client %d: Client inactive and soon will be removed\n", c.sock);

                // Join thread
                if (c.client_thread.joinable()){
                    c.client_thread.join();
                    printf("Client %d: Thread successfully joined\n", c.sock);
                }
                else {
                    printf("Client %d: Thread already joined no idea why\n", c.sock);
                }

                // Add client to the list of clients to be removed
                clients_to_be_joined_and_removed.emplace_back(c.sock);
            }
        }
        printf("Number of clients to be deleted: %zu\n", clients_to_be_joined_and_removed.size());

        // Remove inactive players
        for (Client &c : clients_to_be_joined_and_removed){
            list_of_clients.remove(c);
            printf("Client %d: Successfully removed from client list\n", c.sock);
        }

        // Print client's ip address and port number
        printf("New client connected %s : %d\n", inet_ntoa(client_structure.sin_addr), ntohs(client_structure.sin_port));

        // Add new client to the list_of_clients and start their thread
        list_of_clients.emplace_back(client_socket);
        list_of_clients.back().client_thread = std::thread(client_service, std::ref(list_of_clients.back()));

        // Increase number of connected clients
        num_of_connected_clients++;
    }

    return 0;
}


void client_service(Client &client){

    int sock = client.sock;

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

        printf("Client %d: Disconnecting\n", sock);

        // Close the connection
        int error = shutdown(sock, SHUT_RDWR);
        if (error == -1){
            perror("Shutdown error");
            exit(-1);
        }

        // Marking client as disconnected
        client.is_active = false;
        num_of_connected_clients--;
        printf("Client %d: Successfuly disconnected from server\n", sock);

        // Finish client thread
        printf("Client %d: End of thread\n", sock);
        return;
    }

    // Check if message has correct size for program purposes
    else if (num_read_bytes != 1024){
        printf("Client %d: Message does not have correct size %d\n", sock, num_read_bytes);
        write(1, data, num_read_bytes);
        exit(-1);
    }

    // Client sends their nickname
    if (data[0] == 'N'){
        for (int i=0; i<16; i++){
            client.nickname[i] = data[i+1];
            if (client.nickname[i] == 0){
                break;
            }
        }
        printf("Client %d: New nickname: %s\n", sock, client.nickname);
    }

    // Unknown first character
    else {
        printf("Client %d: Unknown action\n", sock);
        exit(-1);
    }

    // Client thread
    while (true) {

        // Read message from client
        num_read_bytes = read(sock, data, sizeof(data));

        // Handle possible error
        if (num_read_bytes == -1){
            perror("Read error in client's main loop");
            exit(-1);
        }

        // Read 0 bytes - client disconnects
        else if (num_read_bytes == 0){

            printf("Client %d: Disconnecting\n", sock);

            // Close the connection
            int error = shutdown(sock, SHUT_RDWR);
            if (error == -1){
                perror("Shutdown error");
                exit(-1);
            }

            // Marking client as disconnected
            client.is_active = false;
            num_of_connected_clients--;
            printf("Client %d: Successfuly disconnected from server\n", sock);

            // Finish client thread
            printf("Client %d: End of thread\n", sock);
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
                current_players.push_back(Player(sock, players_number, starting_x, starting_y, client.nickname));
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

                // Delete from game players list
                for (Player &player : current_players){
                    if (player.sock == sock){
                        current_players.remove(player);
                        break;
                    }
                }

                printf("Client %d: Disconnecting\n", sock);

                // Close the connection
                int error = shutdown(sock, SHUT_RDWR);
                if (error == -1){
                    perror("Shutdown error");
                    exit(-1);
                }

                // Marking client as disconnected
                client.is_active = false;
                num_of_connected_clients--;
                printf("Client %d: Successfuly disconnected from server\n", sock);

                // Finish client thread
                printf("Client %d: End of thread\n", sock);
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
                    if (player.sock == sock){
                        if ((player.move_direction < 2 && key_num > 1) || (player.move_direction > 1 && key_num < 2)){
                            player.move_direction = key_num;
                            printf("Client %d: New direction %d\n", player.sock, player.move_direction);
                            break;
                        }
                        else {
                            printf("Client %d: Wrong new direction - declined\n", player.sock);
                        }
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
    std::list <Score> best_scores = {};
    for (int i=0; i<3; i++){
        best_scores.push_front(Score(0, ""));
    }

    while (true){

        // Label for easy breaking game loop
        new_game_loop:

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

                // If player doesn't leave board...
                if (x >= 0 && x < N && y >= 0 && y < M){
                    player.list_of_points.push_front(Point(x, y));
                    for (Point &p : player.list_of_points){
                        board[p.x][p.y] = player.number;
                    }
                }
                // Player loses
                else {

                    printf("Player %d: Lost game by leaving board\n", player.sock);

                    // Inform player about lost
                    write(player.sock, "L", 1024);

                    // Add loser's slot number to available player numbers
                    available_player_numbers.push(player.number);

                    // Remove loser and decrease num_of_players_in_game
                    current_players.remove(player);
                    num_of_players_in_game--;

                    // If no more players in the game --> clear best scores
                    if (num_of_players_in_game == 0){
                        // todo
                        // does not stop game after all player left
                        printf("No more players in the game\n");
                        best_scores.clear();
                        for (int i=0; i<3; i++){
                            best_scores.push_front(Score(0, ""));
                        }
                    }

                    // Start new loop without deleted player
                    goto new_game_loop;
                }

                // Best score check
                bool is_already_in_best_scores = false;
                for (Score &s : best_scores) {
                    for (int i=0; i<16; i++) {
                        if (s.nickname[i] != player.nickname[i]) {
                            is_already_in_best_scores = false;
                            break;
                        }
                        else if (player.nickname[i] == 0) {
                            is_already_in_best_scores = true;
                            break;
                        }
                    }
                    if (is_already_in_best_scores){
                        if (player.list_of_points.size() > s.score){
                            s.score = player.list_of_points.size();
                        }
                        break;
                    }
                }

                if (!is_already_in_best_scores && player.list_of_points.size() > best_scores.front().score){
                    best_scores.pop_front();
                    best_scores.push_front(Score(player.list_of_points.size(), player.nickname));
                }
                best_scores.sort();

                // Collision check
                // todo
            }

            // Apple bonus
            board[bonus.x][bonus.y] = 5;

            // Prepare message conataining state of board and best scores
            char msg[1024];
            msg[0] = 'B';
            for (int i=0; i<N; ++i) {
                for (int j = 0; j < M; ++j) {
                    msg[i*M + j + 1] = board[i][j];
                    board[i][j] = 0;
                }
            }

            // Add scores to message
            int index = 601;
            for (Score &s : best_scores){
                msg[index++] = s.score;
                for (int i=0; i<16; i++){
                    msg[index++] = s.nickname[i];
                    if (s.nickname[i] == 0){
                        break;
                    }
                }
            }

            // Send state of game to all players
            for (Player &player : current_players) {
                for (Point &p : player.list_of_points){
                    printf("x: %d\ty: %d\n", p.x, p.y);
                }
                write(player.sock, msg, 1024);
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
