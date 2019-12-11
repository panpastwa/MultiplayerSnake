#include <SFML/Graphics.hpp>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int server()
{
    // Create socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1){
        perror("Socket error");
        return -1;
    }

    // Create structure for any clients with port number
    struct sockaddr_in server_structure = {AF_INET, htons(8888), INADDR_ANY};

    // Binding server socket with sockaddr structure
    int error = bind(server_socket, (sockaddr*)&server_structure, sizeof(server_structure));
    if (error == -1){
        perror("Bind error");
        return -1;
    }

    // Listen for incoming clients
    listen(server_socket, 16);

    // Wait for and accept one client
    struct sockaddr_in client_structure;
    socklen_t size_of_client_structure;
    int client_socket = accept(server_socket, (sockaddr*)&client_structure, &size_of_client_structure);
    if (client_socket == -1){
        perror("Accept error");
        return -1;
    }

    // Print client's ip address and port number
    printf("Witaj %s : %d\n", inet_ntoa(client_structure.sin_addr), ntohs(client_structure.sin_port));

    // Close connection with client
    shutdown(client_socket, SHUT_RDWR);

    return 0;
    sf::RenderWindow window(sf::VideoMode(200, 200), "SFML works!");
    sf::CircleShape shape(100.f);
    shape.setFillColor(sf::Color::Green);

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
