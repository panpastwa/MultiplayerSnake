#include "client.h"


int client()
{
    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1){
        perror("Socket error");
        return -1;
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
        return -1;
    }

    int M=30, N=20;
    int size_of_cell = 16;
    int width = M*size_of_cell;
    int height = N*size_of_cell;

    char board[N][M];
    for (int i=0; i<N; ++i)
        for (int j=0; j<M; ++j)
            board[i][j] = 0;

    board[2][3] = 1;
    board[2][4] = 1;

    board[12][6] = 2;
    board[13][6] = 2;

    sf::RenderWindow window(sf::VideoMode(width, height), "Snake");
    sf::RectangleShape cell(sf::Vector2f(16.0f, 16.0f));

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            // Close window and close connection
            if (event.type == sf::Event::Closed){
                window.close();
                // todo
                // close connection
            }

            // Check if key is pressed and send info to server
            if (event.type == sf::Event::KeyPressed) {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) {
                    send_key_to_server(sf::Keyboard::Key::Up);
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) {
                    send_key_to_server(sf::Keyboard::Key::Down);
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
                    send_key_to_server(sf::Keyboard::Key::Right);
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
                    send_key_to_server(sf::Keyboard::Key::Left);
                }
            }
        }

        window.clear();
        for (int i=0; i<N; ++i)
            for (int j=0; j<M; ++j){
                if (board[i][j] == 0){
                    cell.setFillColor(sf::Color::Magenta);
                    cell.setPosition(size_of_cell*j, size_of_cell*i);
                    window.draw(cell);
                }
                else if (board[i][j] == 1){
                    cell.setFillColor(sf::Color::Yellow);
                    cell.setPosition(size_of_cell*j, size_of_cell*i);
                    window.draw(cell);
                }

                else if (board[i][j] == 2){
                    cell.setFillColor(sf::Color::Blue);
                    cell.setPosition(size_of_cell*j, size_of_cell*i);
                    window.draw(cell);
                }

            }
        window.display();
    }

    return 0;


    char text[1024];

    // reading from socket
    int status = read(sock, text, 1024);

    if (status == -1){
        perror("Error while reading from socket");
        exit(1);
    }

    if (status > 0){
        write(1, text, status);
    }

    return 0;
}

void send_key_to_server(sf::Keyboard::Key key){
    printf("Key %d pressed\n", key);
    // todo
    // send key to server
}
