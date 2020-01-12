#include "server.h"

int main(int argc, char *argv[]){

    // Check if there is an argument
    if (argc == 2){
        for (int i=0; argv[1][i] != 0; i++){
            if (argv[1][i] < '0' || argv[1][i] > '9'){
                printf("Wrong argument! Port argument must be numeric!\n");
                exit(0);
            }
        }
        // convert to int
        int port_num = atoi(argv[1]);
        server(port_num);
    }

    // Wrong usage of program
    else {
        printf("Wrong number of arguments! Usage: ./ServerApp <port_num>\n");
    }
    return 0;
}
