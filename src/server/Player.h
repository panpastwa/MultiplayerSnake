#include <list>
#include "Point.h"

class Player {
public:
    int number;
    int move_direction;
    int sock;
    std::list<Point> list_of_points;
    
    Player(int s, int id, int x, int y){
        sock = s;
        number = id;
        list_of_points.push_front(Point(x, y));
        move_direction = rand() % 4;
    }

    bool operator==(const Player &p){
        return sock == p.sock;
    }
};
