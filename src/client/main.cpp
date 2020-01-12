#include "client.h"

int main(int argc, char *argv[]){

    // Check program arguments
    if (argc == 3){

        // Check if port_num contains only digits
        for (int i=0; argv[2][i] != 0; i++){
            if (argv[2][i] < '0' || argv[2][i] > '9'){
                printf("Wrong argument! Port argument must be numeric!\n");
                exit(0);
            }
        }

        // Convert port_num to int
        int port_num = atoi(argv[2]);

        // Start client function
        client(argv[1], port_num);

        printf("Client finishes successfully\n");
        return 0;
    }

    // Wrong usage of program
    else {
        printf("Wrong number of arguments! Usage: ./ClientApp <ip_address> <port_num>\n");
        exit(0);
    }
}
