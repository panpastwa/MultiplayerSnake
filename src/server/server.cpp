#include "server.h"

// Number of currently connected clients to server
int num_of_connected_clients = 0;

// Array with socket numbers of connected clients and boolean array
int connected_clients[16];
bool is_slot_empty[16];

// Board
const int M = 30, N = 20;
char board[N][M];

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

    // Wait for and accept one client
    struct sockaddr_in client_structure;
    socklen_t size_of_client_structure = sizeof(client_structure);
    int client_socket = accept(server_socket, (sockaddr*)&client_structure, &size_of_client_structure);
    if (client_socket == -1){
        perror("Accept error");
        exit(-1);
    }


    // Print client's ip address and port number
    printf("New client connected %s : %d\n", inet_ntoa(client_structure.sin_addr), ntohs(client_structure.sin_port));

    num_of_connected_clients++;
    connected_clients[0] = client_socket;
    is_slot_empty[0] = false;

    std::thread t(client_service, client_socket);
    printf("Client thread called\n");
    t.join();
    printf("Client thread finished\n");

    // Close connection with client
    shutdown(client_socket, SHUT_RDWR);

    return 0;

}

void client_service(int sock){

    // Wait for client's action in game menu
    client_menu_service(sock);
    client_game_service(sock);
    printf("Client thread ended\n");
}

void client_menu_service(int sock){

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

        printf("Client %d successfuly disconnected from server", sock);
        exit(0);
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
        return;
    }

        // Unknown first character
    else {
        printf("Client %d: Unknown action\n", sock);
        exit(-1);
    }
}

void client_game_service(int sock){
    // todo
    // implement
    ;
}

