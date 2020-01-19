#include "server.h"

// Number of currently connected clients to server
int num_of_connected_clients = 0;
std::condition_variable slot_for_client_available;

// List with connected clients
std::list<Client> list_of_clients;
std::mutex clients_mutex;

// Board
char board[N][M];

// Number of players in game
int num_of_players_in_game = 0;

// List with active players
std::list<Player> current_players = {};
std::mutex current_players_mutex;
std::condition_variable some_new_player_in_queue;

// Stack with available slots (numbers) in the game
std::stack<int> available_player_numbers;

// Queue - list containing socket numbers of clients
std::list<int> queue = {};
std::mutex queue_mutex;

void server(int port_num){

    // Create socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1){
        perror("Socket error");
        exit(-1);
    }

    // Create structure for any clients with port number
    struct sockaddr_in server_structure = {AF_INET, htons(port_num), INADDR_ANY};

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
    printf("Server is listening on port %d\n", port_num);

    // Push available player numbers onto stack
    available_player_numbers.push(1);
    available_player_numbers.push(2);
    available_player_numbers.push(3);
    available_player_numbers.push(4);

    // Start server game thread
    std::thread game_thread(server_game_service);

    // Accept new connections forever
    while (true){

        // Accepting max 16 clients
        while (num_of_connected_clients >= 16){
            // Suspend thread and wait for signal (some client disconnecting)
            printf("Too many players - suspend accepting new clients thread\n");
            std::unique_lock<std::mutex> lk(clients_mutex);
            slot_for_client_available.wait(lk);
            printf("Accepting new clients again\n");
        }

        // Structures for keeping info about new client
        struct sockaddr_in client_structure;
        socklen_t size_of_client_structure = sizeof(client_structure);

        // Accept new client
        printf("Server is ready to accept a new connection\n");
        int client_socket = accept(server_socket, (sockaddr*)&client_structure, &size_of_client_structure);
        if (client_socket == -1){
            perror("Accept error");
            // Try to accept new clients again
            continue;
        }

        // Critical section for clients
        clients_mutex.lock();

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

        // Welcome client
        int num_of_bytes = write(client_socket, "Hi", 2);
        if (num_of_bytes == -1) {
            perror("Write error");
            disconnect_client(list_of_clients.back());
        }

        // Unlock
        clients_mutex.unlock();
    }
}


void disconnect_client(Client &client){

    printf("Client %d: Disconnecting\n", client.sock);

    // Close the connection
    int error = shutdown(client.sock, SHUT_RDWR);
    if (error == -1){
        perror("Shutdown error");
        exit(-1);
    }

    // Marking client as disconnected
    clients_mutex.lock();
    client.is_active = false;
    num_of_connected_clients--;
    clients_mutex.unlock();
    printf("Client %d: Successfuly disconnected from server\n", client.sock);

    // Notify thread accpeting new connections about client disconnection
    slot_for_client_available.notify_one();

    // Finish client thread
    printf("Client %d: End of thread\n", client.sock);
}


void client_service(Client &client){

    int sock = client.sock;

    // Read message from client
    char data[1024];
    int num_read_bytes = read(sock, data, sizeof(data));

    // Handle possible error
    if (num_read_bytes == -1){
        perror("Read error in client's main loop");
        disconnect_client(client);
        return;
    }

    // Read 0 bytes - client disconnects
    else if (num_read_bytes == 0){
        disconnect_client(client);
        return;
    }

    // Client sends their nickname
    if (num_read_bytes > 0 && data[0] == 'N'){
        for (int i=0; i<16; i++){
            client.nickname[i] = data[i+1];
            if (client.nickname[i] == 0){
                if (i+2 != num_read_bytes){
                    printf("Client %d: Unknown size of message with nickname\n", sock);
                }
                break;
            }
        }
        printf("Client %d: New nickname: %s\n", sock, client.nickname);
    }

    // Unknown message
    else {
        printf("Client %d: Unknown action\n", sock);
        disconnect_client(client);
        return;
    }

    // Client thread
    while (true) {

        // Read message from client
        num_read_bytes = read(sock, data, sizeof(data));

        // Handle possible error
        if (num_read_bytes == -1){
            perror("Read error in client's main loop");
            disconnect_client(client);
            return;
        }

        // Read 0 bytes - client disconnects
        else if (num_read_bytes == 0){
            disconnect_client(client);
            return;
        }

        // Client wants to join game
        if (num_read_bytes > 0 && data[0] == 'S'){

            printf("Client %d: Wants to join the game\n", sock);

            // Lock queue
            queue_mutex.lock();

            // Add player to game queue
            printf("Client %d: Added to queue\n", sock);
            queue.push_back(sock);
            char msg[2];
            msg[0] = 'Q';
            msg[1] = '0' + queue.size();

            // Send information about inital position in game queue
            int num_of_bytes = write(sock, msg, 2);
            if (num_of_bytes == -1){
                perror("Write error");
                queue.pop_back();
                queue_mutex.unlock();
                disconnect_client(client);
                return;
            }
            if (num_of_bytes != 2){
                printf("Wrong number of bytes send\n");
                queue.pop_back();
                queue_mutex.unlock();
                disconnect_client(client);
                return;
            }

            // Unlock queue
            queue_mutex.unlock();

            // Notify and wake up server game service
            some_new_player_in_queue.notify_one();

        }

        // Keyboard input (possibly from last game --> ignore
        else if (num_read_bytes > 0 && data[0] == 'K'){
            printf("Client %d: Keyboard input from last game - ignore\n", sock);
            continue;
        }

        // Unknown first character
        else {
            printf("Client %d: Unknown action\n", sock);
            disconnect_client(client);
            return;
        }

        // Game loop
        while (true){

            // Read message from client
            num_read_bytes = read(sock, data, sizeof(data));

            // Handle possible error
            if (num_read_bytes == -1){
                perror("Read error");

                // Remove from queue
                queue_mutex.lock();
                queue.remove(sock);
                queue_mutex.unlock();

                // Delete from game players list
                current_players_mutex.lock();
                for (Player &p : current_players){
                    if (sock == p.sock){

                        // Add loser's slot number to available player numbers
                        available_player_numbers.push(p.number);

                        // Remove loser and decrease num_of_players_in_game
                        current_players.remove(p);
                        num_of_players_in_game--;
                        break;
                    }
                }
                current_players_mutex.unlock();

                disconnect_client(client);
                return;
            }

            // Read 0 bytes - client disconnects
            else if (num_read_bytes == 0){

                // Remove from queue
                queue_mutex.lock();
                queue.remove(sock);
                queue_mutex.unlock();

                // Delete from game players list
                current_players_mutex.lock();
                for (Player &p : current_players){
                    if (sock == p.sock){

                        // Add loser's slot number to available player numbers
                        available_player_numbers.push(p.number);

                        // Remove loser and decrease num_of_players_in_game
                        current_players.remove(p);
                        num_of_players_in_game--;
                        break;
                    }
                }
                current_players_mutex.unlock();

                disconnect_client(client);
                return;
            }

            // Client sends new direction to server
            if (num_read_bytes > 1 && data[0] == 'K' && data[1] >= 0 && data[1] < 4){
                int key_num = data[1];
                current_players_mutex.lock();
                for (Player &player : current_players){
                    if (player.sock == sock){
                        if ((player.move_direction < 2 && key_num > 1) || (player.move_direction > 1 && key_num < 2)){
                            // Change direction
                            current_players_mutex.lock();
                            player.move_direction = key_num;
                            break;
                        }
                    }
                }
                current_players_mutex.unlock();
            }

            // Clients replies to message about losing --> Client lost and backs to menu
            else if (num_read_bytes > 1 && data[0] == 'L'){
                printf("Client %d: Knows about lost game and is back in menu\n", sock);
                break;
            }

            // Unknown first character
            else {
                printf("Client %d: Unknown action\n", sock);

                // Remove from queue
                queue_mutex.lock();
                queue.remove(sock);
                queue_mutex.unlock();

                // Delete from game players list
                current_players_mutex.lock();
                for (Player &p : current_players){
                    if (sock == p.sock){

                        // Add loser's slot number to available player numbers
                        available_player_numbers.push(p.number);

                        // Remove loser and decrease num_of_players_in_game
                        current_players.remove(p);
                        num_of_players_in_game--;
                        break;
                    }
                }
                current_players_mutex.unlock();

                disconnect_client(client);
                return;
            }
        }
    }
}


void server_game_service(){

    // Set random position of fruit
    Point bonus(rand()%N, rand()%M);

    // Initialize best scores
    std::list <Score> best_scores = {};

    // List of players that lost and need to be removed from the game
    std::list<Player> players_to_be_removed = {};

    // Main loop of server game service
    while (true){

        // Label for easy breaking game loop
        new_game_loop:

        if (num_of_players_in_game == 0){

            // Clear best scores
            best_scores.clear();
            for (int i=0; i<3; i++){
                best_scores.push_front(Score(0, ""));
            }

            std::unique_lock<std::mutex> lk(queue_mutex);
            if (queue.empty()){

                // Wait for some player to be added to queue
                printf("SUSPEND SERVER GAME SERVIE\n");
                some_new_player_in_queue.wait(lk);
                printf("New player in queue\n");
                printf("UNSUSPEND SERVER GAME SERVIE\n");
            }
            queue_mutex.unlock();
        }

        // Let players in queue join game if there are available slots
        while (num_of_players_in_game < 4 && !queue.empty()){

            // Lock queue
            queue_mutex.lock();

            // Get player from front of the list
            int new_player_sock = queue.front();
            printf("Client %d: Joining game\n", new_player_sock);

            // Inform all clients in queue about changes in queue
            char msg[2];
            msg[0] = 'Q';
            int i = 0;
            for (int sock : queue){
                msg[1] = '0' + i++;
                int num_of_bytes = write(sock, msg, 2);
                if (num_of_bytes == -1){
                    perror("Write error");
                    exit(-1);
                }
                if (num_of_bytes != 2){
                    printf("Wrong number of bytes send\n");
                }
            }

            // Delete front player
            queue.pop_front();

            // Unlock queue
            queue_mutex.unlock();

            // Lock players_mutex
            current_players_mutex.lock();

            // Add new player to the game
            num_of_players_in_game++;
            int players_number = available_player_numbers.top();
            available_player_numbers.pop();
            int starting_x = rand() % N;
            int starting_y = rand() % M;
            while (board[starting_x][starting_y] != 0){
                starting_x = rand() % N;
                starting_y = rand() % M;
            }

            // Find client's nickname
            for (Client &c : list_of_clients){
                if (c.sock == new_player_sock){
                    current_players.emplace_back(new_player_sock, players_number, starting_x, starting_y, c.nickname);
                    printf("Client %d: Successfully joined the game\n", new_player_sock);
                }
            }

            // Unlock players_mutex
            current_players_mutex.unlock();
        }

        // Lock players_mutex
        current_players_mutex.lock();

        // While someone is in the game
        if (num_of_players_in_game > 0) {

            // Calculate new front point for each player
            for (Player &player : current_players) {

                Point point = player.list_of_points.front();
                int x, y;
                if (player.move_direction == 0) {
                    // left
                    x = point.x;
                    y = point.y - 1;
                } else if (player.move_direction == 1) {
                    // right
                    x = point.x;
                    y = point.y + 1;
                } else if (player.move_direction == 2) {
                    // up
                    x = point.x - 1;
                    y = point.y;
                } else if (player.move_direction == 3) {
                    // down
                    x = point.x + 1;
                    y = point.y;
                } else {
                    printf("Wrong direction\n");
                    exit(-1);
                }

                // Add new point in front of the snake
                player.list_of_points.push_front(Point(x, y));

            }

            // Clear list of players to be removed
            players_to_be_removed.clear();

            // Check collisions and leaving board
            for (Player &player : current_players){

                // Get front point of player
                int x = player.list_of_points.front().x;
                int y = player.list_of_points.front().y;

                // Check if player leaves board
                if (x < 0 || x >= N || y < 0 || y >= M){
                    players_to_be_removed.push_front(Player(player.sock, player.number));
                    continue;
                }

                // Check collision with other snakes
                for (Player &p : current_players){
                    // For other players check all point
                    if (p.sock != player.sock){
                        for (Point &point : p.list_of_points){
                            // Check collision
                            if (x == point.x && y == point.y){
                                players_to_be_removed.push_front(Player(player.sock, player.number));
                                goto leave_loop;
                            }
                        }
                    }
                    // For yourself check only other points (without head)
                    else {
                        for (auto iterator = std::next(p.list_of_points.begin()); iterator != p.list_of_points.end(); ++iterator){
                            // Check collision
                            if (x == iterator->x && y == iterator->y){
                                players_to_be_removed.push_front(Player(player.sock, player.number));
                                goto leave_loop;
                            }
                        }
                    }
                }
                // Label for easy leaving loop
                leave_loop:;
            }

            // Remove players that lost
            for (Player &player : players_to_be_removed){

                // Inform player about lost
                int num_of_bytes = write(player.sock, "L", 1);
                if (num_of_bytes == -1){
                    perror("Write error");
                    exit(-1);
                }
                if (num_of_bytes != 1){
                    printf("Wrong number of bytes send\n");
                }

                // Add loser's slot number to available player numbers
                available_player_numbers.push(player.number);

                // Remove loser and decrease num_of_players_in_game
                current_players.remove(player);
                num_of_players_in_game--;

                // If no more players in the game --> clear best scores
                if (num_of_players_in_game == 0){
                    printf("No more players in the game\n");
                    current_players_mutex.unlock();
                    goto new_game_loop;
                }
            }

            for (Player &player : current_players){

                // Get front point of player
                int x = player.list_of_points.front().x;
                int y = player.list_of_points.front().y;

                // Check if player collects fruit
                if (x == bonus.x && y == bonus.y){
                    do {
                        bonus.x = rand() % N;
                        bonus.y = rand() % M;
                    } while (board[bonus.x][bonus.y] != 0);

                }
                else {
                    player.list_of_points.pop_back();
                }

                // Update board matrix
                for (Point &p : player.list_of_points){
                    board[p.x][p.y] = player.number;
                }

                // Best score check for players already in scoreboard
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

                // Best score check for players not yet in scoreboard
                if (!is_already_in_best_scores && player.list_of_points.size() > best_scores.front().score){
                    best_scores.pop_front();
                    best_scores.push_front(Score(player.list_of_points.size(), player.nickname));
                }
                best_scores.sort();
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
                int num_of_bytes = write(player.sock, msg, index);
                if (num_of_bytes == -1){
                    perror("Write error");
                    exit(-1);
                }
                if (num_of_bytes != index){
                    printf("Wrong number of bytes send\n");
                }
            }

            // Send state of game to all clients in queue
            queue_mutex.lock();
            for (int c : queue){
                int num_of_bytes = write(c, msg, index);
                if (num_of_bytes == -1){
                    perror("Write error");
                    exit(-1);
                }
                if (num_of_bytes != index){
                    printf("Wrong number of bytes send\n");
                }
            }
            queue_mutex.unlock();
        }

        // Unlock players_mutex
        current_players_mutex.unlock();

        usleep(300000);
    }

}
