#include <list>
#include "Point.h"

class Player {
public:
    int move_direction;
    int socket_num;
    std::list<Point> list_of_points;
    
    Player(int sock, int x, int y){
        socket_num = sock;
        list_of_points.push_front(Point(x, y));
        move_direction = 0;
    }

    bool operator==(const Player &p1){
        return socket_num == p1.socket_num;
    }
};
