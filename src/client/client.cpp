#include "client.h"

// Size of board
const int M = 30, N = 20;
int size_of_cell = 32;

int client()
{
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

    int width = M*size_of_cell;
    int height = N*size_of_cell;
    sf::RenderWindow window(sf::VideoMode(width, height), "Snake");

    menu(window, sock);
    queue(window, sock);
    game(window, sock);
    score(window, sock);

    return 0;
}

void menu(sf::RenderWindow &window, int sock){

    // Create and load font
    sf::Font font;
    font.loadFromFile("data/UbuntuMono-RI.ttf");

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
                        return;
                    }

                    // Current option - EXIT
                    else {
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

    // Create empty board
    char board[N][M];
    for (int i=0; i<N; ++i)
        for (int j=0; j<M; ++j)
            board[i][j] = 0;

    // Exemplary data in board
    board[2][3] = 1;
    board[2][4] = 1;

    board[12][6] = 2;
    board[13][6] = 2;

    board[6][18] = 3;
    board[6][19] = 3;

    board[19][0] = 4;
    board[19][1] = 4;

    // Set properties of cell
    sf::RectangleShape cell(sf::Vector2f(size_of_cell, size_of_cell));
    cell.setOutlineThickness(1.0f);
    cell.setOutlineColor(sf::Color::Black);

    // Main game loop
    while (window.isOpen())
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
        for (int i=0; i<N; ++i)
            for (int j=0; j<M; ++j){
                if (board[i][j] == 0) {
                    cell.setFillColor(sf::Color::White);
                    cell.setPosition(size_of_cell*j, size_of_cell*i);
                    window.draw(cell);
                }
                else if (board[i][j] == 1) {
                    cell.setFillColor(sf::Color::Magenta);
                    cell.setPosition(size_of_cell*j, size_of_cell*i);
                    window.draw(cell);
                }

                else if (board[i][j] == 2) {
                    cell.setFillColor(sf::Color::Blue);
                    cell.setPosition(size_of_cell*j, size_of_cell*i);
                    window.draw(cell);
                }

                else if (board[i][j] == 3) {
                    cell.setFillColor(sf::Color::Green);
                    cell.setPosition(size_of_cell*j, size_of_cell*i);
                    window.draw(cell);
                }

                else if (board[i][j] == 4) {
                    cell.setFillColor(sf::Color::Yellow);
                    cell.setPosition(size_of_cell*j, size_of_cell*i);
                    window.draw(cell);
                }

                else {
                    printf("Unknown board matrix value");
                    exit(-1);
                }

            }
        window.display();
    }
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
    msg[4] = key-71 + '0';
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
