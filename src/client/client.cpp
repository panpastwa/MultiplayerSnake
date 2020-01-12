#include "client.h"

// Board
char board[N][M];
bool is_in_game;

// Best scores
int best_scores[3];
char nicknames[3][16];

// Font
sf::Font font;

int client()
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
    ip_address.sin_port = htons(8888);
    ip_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to server
    int error = connect(sock, (sockaddr*) &ip_address, sizeof(ip_address));
    if (error == -1){
        perror("Connect error");
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
        menu(window, sock);
        queue(window, sock);
        game(window, sock);
        score(window, sock);
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
            if (event.type == sf::Event::Closed) {
                window.close();
                int error = shutdown(sock, SHUT_RDWR);
                if (error == -1) {
                    perror("Shutdown error");
                    exit(-1);
                }
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
                    for (int i=current_index_in_text; i<=18; i++){
                        text_to_send[i] = 0;
                    }

                    // Send message to server
                    printf("Nick accepted: %s\n", text_to_send);
                    int num_of_bytes = write(sock, text_to_send, 1024);
                    if (num_of_bytes == -1){
                        perror("Start game info send error");
                        exit(-1);
                    } else if (num_of_bytes < 1024){
                        printf("Too little bytes send\n");
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
            if (event.type == sf::Event::Closed) {
                window.close();
                int error = shutdown(sock, SHUT_RDWR);
                if (error == -1){
                    perror("Shutdown error");
                    exit(-1);
                }
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
                        int num_of_bytes = write(sock, "1", 1024);
                        if (num_of_bytes == -1){
                            perror("Start game info send error");
                            exit(-1);
                        } else if (num_of_bytes < 1024){
                            printf("Too little bytes send\n");
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
    // todo
    // implement
    ;
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

    // Read initial board state and enable updating real-time
    std::thread game_updater(update_game_state, sock);

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
                int error = shutdown(sock, SHUT_RDWR);
                if (error == -1){
                    perror("Shutdown error");
                    exit(-1);
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
        window.draw(scoreboard_text);
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
        window.display();
    }

    // Join game updater since player lost
    game_updater.join();
}

void score(sf::RenderWindow &window, int sock){
    // todo
    // implement
    ;
}

void send_key_to_server(sf::Keyboard::Key key, int server_sock){
    printf("Key %d pressed\n", key-71);
    char msg[1024] = "KEY:";

    // adding '0' for clean printf
    msg[4] = key-71;
    msg[5] = '\n';
    msg[6] = '\0';
    int num_of_bytes = write(server_sock, msg, sizeof(msg));
    // todo
    // check if server is connected and handle this error
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

        // Check if message has correct size for program purposes
        else if (num_read_bytes != 1024){
            printf("Message does not have correct size %d\n", num_read_bytes);
            write(1, data, num_read_bytes);
            exit(-1);
        }

        // Server sends board state
        if (data[0] == 'B'){

            // Read board state
            for (int i=0; i<N; ++i) {
                for (int j = 0; j < M; ++j) {
                    board[i][j] = data[i*M + j + 1];
                }
            }

            // Read best scores
            int index = 601;
            for (int i=0; i<3; i++){
                best_scores[i] = data[index++];
                for (int j=0; j<16; j++){
                    nicknames[i][j] = data[index++];
                    if (nicknames[i][j] == 0){
                        break;
                    }
                }
            }
            printf("Read new game state from server\n");
        }

        // Server sends information about a lose --> Reply and go back to menu
        else if (data[0] == 'L'){
            printf("You lost\n");
            is_in_game = false;
            write(sock, "L", 1024);
            for (int i=0; i<N; ++i) {
                for (int j = 0; j < M; ++j) {
                    board[i][j] = 0;
                }
            }
            return;
        }

        // Unknown first character
        else {
            printf("Client %d: Unknown action\n", sock);
            exit(-1);
        }
    }

}
