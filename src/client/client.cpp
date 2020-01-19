#include "client.h"

// Board
char board[N][M];
bool is_in_game;
std::mutex board_mutex;

// Best scores
int best_scores[3];
char nicknames[3][16];

// Font
sf::Font font;

// Position in queue
char queue_position = ' ';
std::mutex queue_position_mutex;

// Thread for communication
std::thread game_updater;

// Used for properly exiting application and joining thread
bool is_disconnecting = false;
std::mutex disconnecting_mutex;


void client(char *ip_addr, int port_num)
{
    // Load a font
    font.loadFromFile("data/UbuntuMono-RI.ttf");

    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1){
        perror("Socket error");
        exit(-1);
    }

    // Create structure with server's port number and ip address
    struct sockaddr_in ip_address{};
    ip_address.sin_family = AF_INET;
    ip_address.sin_port = htons(port_num);
    ip_address.sin_addr.s_addr = inet_addr(ip_addr);

    // Connect to server
    int error = connect(sock, (sockaddr*) &ip_address, sizeof(ip_address));
    if (error == -1){
        perror("Connect error");
        exit(-1);
    }
    printf("Connection successful\n");

    // Wait for welcome message
    char data[16];
    printf("Waiting for welcome message from server\n");
    int num_of_bytes = read(sock, data, 16);
    if (num_of_bytes == -1) {
        perror("Read error");
        exit(-1);
    }
    else if (num_of_bytes == 2 && data[0] == 'H' && data[1] == 'i'){
        printf("Welcome message received\n");
    }
    else {
        printf("Error during receiving welcome message from server\n");
        exit(-1);
    }

    // Render window
    int width = (M + 10)*size_of_cell;
    int height = N*size_of_cell;
    sf::RenderWindow window(sf::VideoMode(width, height), "Snake");

    // Enter your nickname
    enter_nickname(window, sock);

    // Client's main loop
    while (true){
        printf("Enter menu\n");
        menu(window, sock);

        // Read initial board state and enable updating real-time
        printf("Start game_updater thread\n");
        game_updater = std::thread(update_game_state, sock);

        printf("Enter queue\n");
        queue(window, sock);
        printf("Enter game\n");
        game(window, sock);

        printf("Join game_updater\n");
        game_updater.join();
    }
}


void enter_nickname(sf::RenderWindow &window, int sock) {

    // Create ENTER NICKNAME text
    sf::Text enter_nick_text("Enter your nickname", font);
    enter_nick_text.setCharacterSize(50);
    enter_nick_text.setFillColor(sf::Color::Black);
    enter_nick_text.setPosition(340.0f, 200.0f);
    enter_nick_text.setStyle(sf::Text::Bold);

    // Create NICKNAME text
    sf::Text nick_text("", font);
    nick_text.setCharacterSize(50);
    nick_text.setFillColor(sf::Color::Black);
    nick_text.setPosition(360.0f, 350.0f);

    // Nick to be sent
    char text_to_send[17];
    text_to_send[0] = 'N';
    int current_index_in_text = 1;

    // Main menu loop
    while (window.isOpen()) {

        sf::Event event;
        while (window.pollEvent(event)) {

            // Close window and close connection
            if (event.type == sf::Event::Closed){
                printf("Window closed\n");
                window.close();
                int error = shutdown(sock, SHUT_RDWR);
                if (error == -1){
                    perror("Shutdown error");
                    exit(-1);
                }
                printf("Disconnected\n");
                exit(0);
            }

            // Any key pressed
            if (event.type == sf::Event::KeyPressed) {

                int key = event.key.code;

                // If character is between A and Z
                if (key <= sf::Keyboard::Z) {

                    // Nick must be shorter than 16 characters
                    if (current_index_in_text < 16){
                        char new_char = key + 'A';
                        nick_text.setString(nick_text.getString() + new_char);
                        text_to_send[current_index_in_text++] = new_char;
                    }
                    else {
                        printf("Too long nick\n");
                    }

                }

                // Backspace --> remove one character
                else if (key == sf::Keyboard::Key::BackSpace){
                    if (current_index_in_text > 1){
                        int len = nick_text.getString().getSize();
                        nick_text.setString(nick_text.getString().substring(0, len-1));
                        current_index_in_text--;
                    }
                }

                // Client accepts his nickname --> send it to a server
                else if (key == sf::Keyboard::Key::Enter) {

                    // Nick must be at least 4 characters long
                    if (current_index_in_text < 5){
                        printf("Too short nick\n");
                        continue;
                    }

                    // Adding \0 char at the end
                    text_to_send[current_index_in_text] = 0;
                    // Send message to server
                    printf("Nick accepted: %s\n", text_to_send);
                    int num_of_bytes = write(sock, text_to_send, current_index_in_text+1);
                    if (num_of_bytes == -1){
                        perror("Start game info send error");
                        exit(-1);
                    } else if (num_of_bytes != current_index_in_text+1){
                        printf("Wrong number of bytes send\n");
                        exit(-1);
                    }
                    return;
                }
            }
        }

        // Drawing
        window.clear(sf::Color::White);
        window.draw(enter_nick_text);
        window.draw(nick_text);
        window.display();
    }
}


void menu(sf::RenderWindow &window, int sock){

    // Create START GAME text
    sf::Text start_text("Join game", font);
    start_text.setCharacterSize(50);
    start_text.setFillColor(sf::Color::Black);
    start_text.setPosition(340.0f, 200.0f);
    start_text.setStyle(sf::Text::Bold);

    // Create EXIT text
    sf::Text exit_text("Exit", font);
    exit_text.setCharacterSize(50);
    exit_text.setFillColor(sf::Color::Black);
    exit_text.setPosition(360.0f, 350.0f);

    // Create variable holding information about highlighted state in menu
    // 0 - start game
    // 1 - exit
    int menu_option = 0;

    // Main menu loop
    while (window.isOpen()){

        sf::Event event;
        while (window.pollEvent(event)) {

            // Close window and close connection
            if (event.type == sf::Event::Closed){
                printf("Window closed\n");
                window.close();
                int error = shutdown(sock, SHUT_RDWR);
                if (error == -1){
                    perror("Shutdown error");
                    exit(-1);
                }
                printf("Disconnected\n");
                exit(0);
            }

            // Any key pressed
            if (event.type == sf::Event::KeyPressed) {

                // Navigation in menu using arrow key up
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) {
                    menu_option = 0;
                    start_text.setStyle(sf::Text::Bold);
                    exit_text.setStyle(sf::Text::Regular);
                }

                // Navigation in menu using arrow key down
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) {
                    menu_option = 1;
                    start_text.setStyle(sf::Text::Regular);
                    exit_text.setStyle(sf::Text::Bold);
                }

                // User chooses current option
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Enter)){

                    // Current option - START GAME
                    if (menu_option == 0){

                        // Send message to server
                        int num_of_bytes = write(sock, "S", 1);
                        if (num_of_bytes == -1){
                            perror("Start game info send error");
                            exit(-1);
                        } else if (num_of_bytes != 1){
                            printf("Wrong number of bytes send\n");
                            exit(-1);
                        }

                        return;
                    }

                    // Current option - EXIT
                    else {

                        // Close window and close connection
                        window.close();
                        int error = shutdown(sock, SHUT_RDWR);
                        if (error == -1){
                            perror("Shutdown error");
                            exit(-1);
                        }
                        exit(0);
                    }
                }
            }
        }

        // Drawing
        window.clear(sf::Color::White);
        window.draw(start_text);
        window.draw(exit_text);
        window.display();
    }
}

void queue(sf::RenderWindow &window, int sock){

    // Create POSITION IN QUEUE text
    sf::Text position_in_queue_text("Position in queue:", font);
    position_in_queue_text.setCharacterSize(25);
    position_in_queue_text.setFillColor(sf::Color::White);
    position_in_queue_text.setPosition(1000.0f, 100.0f);
    position_in_queue_text.setStyle(sf::Text::Bold);

    // Create POSITION NUMBER text
    queue_position_mutex.lock();
    if (queue_position == '0'){
        queue_position_mutex.unlock();
        return;
    }
    sf::Text position_number_text(queue_position, font);
    queue_position_mutex.unlock();
    position_number_text.setCharacterSize(70);
    position_number_text.setFillColor(sf::Color::White);
    position_number_text.setPosition(1080.0f, 170.0f);
    position_number_text.setStyle(sf::Text::Bold);

    // Set properties of cell
    sf::RectangleShape cell(sf::Vector2f(size_of_cell, size_of_cell));
    cell.setOutlineThickness(1.0f);
    cell.setOutlineColor(sf::Color::White);

    // Create a fruit
    sf::Sprite fruit;
    sf::Texture fruit_texture;
    fruit.setColor(sf::Color::White);
    fruit_texture.loadFromFile("data/fruit.png");
    fruit.setTexture(fruit_texture);

    // Main game loop
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            // Close window and close connection
            if (event.type == sf::Event::Closed){

                window.close();
                printf("Window closed\n");

                // Inform thread about exitting
                disconnecting_mutex.lock();
                is_disconnecting = true;
                disconnecting_mutex.unlock();

                int error = shutdown(sock, SHUT_RDWR);
                if (error == -1){
                    perror("Shutdown error");
                    exit(-1);
                }
                printf("Disconnected\n");

                if (game_updater.joinable()){
                    game_updater.join();
                    printf("Thread successfully joined\n");
                }

                exit(0);
            }
        }

        // Check current position
        queue_position_mutex.lock();
        if (queue_position == '0') {
            printf("Leaving queue\n");
            queue_position_mutex.unlock();
            return;
        } else {
            position_number_text.setString(queue_position);
        }
        queue_position_mutex.unlock();

        // Drawing
        window.clear();
        board_mutex.lock();
        for (int i=0; i<N; ++i){
            for (int j=0; j<M; ++j) {
                if (board[i][j] == 0) {
                    cell.setFillColor(sf::Color::Black);
                    cell.setPosition(size_of_cell * j, size_of_cell * i);
                    window.draw(cell);
                } else if (board[i][j] == 1) {
                    cell.setFillColor(sf::Color::Magenta);
                    cell.setPosition(size_of_cell * j, size_of_cell * i);
                    window.draw(cell);
                } else if (board[i][j] == 2) {
                    cell.setFillColor(sf::Color::Blue);
                    cell.setPosition(size_of_cell * j, size_of_cell * i);
                    window.draw(cell);
                } else if (board[i][j] == 3) {
                    cell.setFillColor(sf::Color::Green);
                    cell.setPosition(size_of_cell * j, size_of_cell * i);
                    window.draw(cell);
                } else if (board[i][j] == 4) {
                    cell.setFillColor(sf::Color::Yellow);
                    cell.setPosition(size_of_cell * j, size_of_cell * i);
                    window.draw(cell);
                } else if (board[i][j] == 5) {
                    fruit.setPosition(size_of_cell * j+1, size_of_cell * i+1);
                    window.draw(fruit);
                } else {
                    printf("Unknown board matrix value: %d\n", board[i][j]);
                    exit(-1);
                }
            }
        }
        board_mutex.unlock();
        window.draw(position_in_queue_text);
        window.draw(position_number_text);
        window.display();
    }
    printf("Leaving queue\n");
}

void game(sf::RenderWindow &window, int sock){

    // Create SCOREBOARD text
    sf::Text scoreboard_text("Scoreboard", font);
    scoreboard_text.setCharacterSize(25);
    scoreboard_text.setFillColor(sf::Color::White);
    scoreboard_text.setPosition(1050.0f, 100.0f);
    scoreboard_text.setStyle(sf::Text::Bold);

    // Scorebaord record template
    sf::Text score_text("", font);
    score_text.setCharacterSize(20);
    score_text.setFillColor(sf::Color::White);

    for (int i=0; i<3; i++){
        best_scores[i] = 0;
        nicknames[i][0] = 0;
    }

    // Set properties of cell
    sf::RectangleShape cell(sf::Vector2f(size_of_cell, size_of_cell));
    cell.setOutlineThickness(1.0f);
    cell.setOutlineColor(sf::Color::White);

    // Create a fruit
    sf::Sprite fruit;
    sf::Texture fruit_texture;
    fruit.setColor(sf::Color::White);
    fruit_texture.loadFromFile("data/fruit.png");
    fruit.setTexture(fruit_texture);

    // If not set - player lost and will leave main game loop
    is_in_game = true;

    // Main game loop
    while (window.isOpen() && is_in_game)
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            // Close window and close connection
            if (event.type == sf::Event::Closed){

                window.close();
                printf("Window closed\n");

                // Inform thread about exitting
                disconnecting_mutex.lock();
                is_disconnecting = true;
                disconnecting_mutex.unlock();

                int error = shutdown(sock, SHUT_RDWR);
                if (error == -1){
                    perror("Shutdown error");
                    exit(-1);
                }
                printf("Disconnected\n");

                if (game_updater.joinable()){
                    game_updater.join();
                    printf("Thread successfully joined\n");
                }

                exit(0);
            }

            // Check if key is pressed and send info to server
            if (event.type == sf::Event::KeyPressed) {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) {
                    send_key_to_server(sf::Keyboard::Key::Up, sock);
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) {
                    send_key_to_server(sf::Keyboard::Key::Down, sock);
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
                    send_key_to_server(sf::Keyboard::Key::Right, sock);
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
                    send_key_to_server(sf::Keyboard::Key::Left, sock);
                }
            }
        }

        // Drawing
        window.clear();
        board_mutex.lock();
        for (int i=0; i<N; ++i){
            for (int j=0; j<M; ++j) {
                if (board[i][j] == 0) {
                    cell.setFillColor(sf::Color::Black);
                    cell.setPosition(size_of_cell * j, size_of_cell * i);
                    window.draw(cell);
                } else if (board[i][j] == 1) {
                    cell.setFillColor(sf::Color::Magenta);
                    cell.setPosition(size_of_cell * j, size_of_cell * i);
                    window.draw(cell);
                } else if (board[i][j] == 2) {
                    cell.setFillColor(sf::Color::Blue);
                    cell.setPosition(size_of_cell * j, size_of_cell * i);
                    window.draw(cell);
                } else if (board[i][j] == 3) {
                    cell.setFillColor(sf::Color::Green);
                    cell.setPosition(size_of_cell * j, size_of_cell * i);
                    window.draw(cell);
                } else if (board[i][j] == 4) {
                    cell.setFillColor(sf::Color::Yellow);
                    cell.setPosition(size_of_cell * j, size_of_cell * i);
                    window.draw(cell);
                } else if (board[i][j] == 5) {
                    fruit.setPosition(size_of_cell * j+1, size_of_cell * i+1);
                    window.draw(fruit);
                } else {
                    printf("Unknown board matrix value: %d\n", board[i][j]);
                    exit(-1);
                }
            }
        }
        for (int i=0; i<3; i++){
            if (best_scores[i]){
                score_text.setPosition(1050.f, 400.0f - 50.0f * i);
                score_text.setString((sf::String(std::to_string(best_scores[i]))));
                window.draw(score_text);
                score_text.setPosition(1100.0f, 400.0f - 50.0f * i);
                score_text.setString(sf::String(nicknames[i]));
                window.draw(score_text);
            }
        }
        board_mutex.unlock();
        window.draw(scoreboard_text);
        window.display();
    }
}


void send_key_to_server(sf::Keyboard::Key key, int server_sock){
    printf("Key %d pressed\n", key-71);
    char msg[2];
    msg[0] = 'K';
    msg[1] = key-71;
    int num_of_bytes = write(server_sock, msg, sizeof(msg));
    if (num_of_bytes == -1){
        perror("Send key to server error");
        exit(-1);
    }
}

void update_game_state(int sock){

    while (true){

        // Read message from server
        char data[1024];
        int num_read_bytes = read(sock, data, sizeof(data));

        // Returning from thread in order to properly close application
        disconnecting_mutex.lock();
        if (is_disconnecting){
            disconnecting_mutex.unlock();
            return;
        }
        disconnecting_mutex.unlock();

        // Handle possible error
        if (num_read_bytes == -1){
            perror("Read error in client's game loop");
            exit(-1);
        }

        // Read 0 bytes - server disconnects
        else if (num_read_bytes == 0){
            // server disconnects?
            printf("Server disconnected\n");
            exit(0);
        }

        // Process whole message from server
        int num_of_bytes_processed = 0;
        while (num_of_bytes_processed < num_read_bytes){

            // Server sends board state
            if (data[num_of_bytes_processed] == 'B'){

                // Lock board_mutex
                board_mutex.lock();

                // Read board state
                for (int i=0; i<N; ++i) {
                    for (int j = 0; j < M; ++j) {
                        board[i][j] = data[++num_of_bytes_processed];
                    }
                }

                // Read best scores
                for (int i=0; i<3; i++){
                    best_scores[i] = data[++num_of_bytes_processed];
                    for (int j=0; j<16; j++){
                        nicknames[i][j] = data[++num_of_bytes_processed];
                        if (nicknames[i][j] == 0){
                            break;
                        }
                    }
                }

                // Unlock board_mutex
                board_mutex.unlock();
                num_of_bytes_processed++;
            }

            // Server sends information about a lose --> Reply and go back to menu
            else if (data[num_of_bytes_processed] == 'L'){
                printf("You lost\n");
                is_in_game = false;
                board_mutex.lock();
                write(sock, "L", 1024);
                for (int i=0; i<N; ++i) {
                    for (int j = 0; j < M; ++j) {
                        board[i][j] = 0;
                    }
                }
                board_mutex.unlock();
                queue_position = ' ';
                return;
            }

            // Server sends update about queue position
            else if (data[num_of_bytes_processed] == 'Q'){
                if (data[num_of_bytes_processed+1] >= '0' && data[num_of_bytes_processed+1] <= '9'){
                    printf("Update queue position message from server\n");
                    queue_position = data[++num_of_bytes_processed];
                    num_of_bytes_processed++;
                }
                else {
                    printf("Unknown error\n");
                    exit(-1);
                }
            }

            // Unknown first character
            else {
                printf("Client %d: Unknown action\n", sock);
                exit(-1);
            }
        }
    }
}
