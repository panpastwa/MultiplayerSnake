#include <list>
#include "Point.h"

class Player {
public:
    int number;
    int move_direction;
    int sock;
    char nickname[16];

    std::list<Point> list_of_points;
    
    Player(int s, int id, int x, int y, const char *tab){
        sock = s;
        number = id;
        list_of_points.push_front(Point(x, y));
        move_direction = rand() % 4;
        for (int i=0; i<16; i++){
            nickname[i] = tab[i];
            if (nickname[i] == 0)
                break;
        }
    }

    Player(int s, int id){
        sock = s;
        number = id;
    }

    bool operator==(const Player &p){
        return sock == p.sock;
    }
};
