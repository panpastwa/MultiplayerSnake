#include <SFML/Graphics.hpp>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int client()
{
    // creating socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    // creating a structure and assigning values
    struct sockaddr_in ip_address{};
    ip_address.sin_family = AF_INET;
    ip_address.sin_port = htons(8888);
    ip_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    // connecting...
    connect(sock, (sockaddr*) &ip_address, sizeof(ip_address));

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

    sf::RenderWindow window(sf::VideoMode(200, 200), "SFML works!");
    sf::CircleShape shape(100.f);
    shape.setFillColor(sf::Color::Yellow);

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        window.clear();
        window.draw(shape);
        window.display();
    }

    return 0;
}
