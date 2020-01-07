#include <thread>

class Client {
public:
    int sock;
    std::thread client_thread;
    bool is_active;
    char nickname[16]{};

    Client(int s){
        sock = s;
        is_active = true;
    }

    bool operator==(const Client &c){
        return sock == c.sock;
    }
};
