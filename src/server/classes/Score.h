class Score {
public:
    char score;
    char nickname[16] = {};

    Score(char s, const char *tab){
        score = s;
        for (int i=0; i<16; i++){
            nickname[i] = *(tab+i);
            if (nickname[i] == 0){
                break;
            }
        }
    }

    bool operator<(const Score &s){
        return score < s.score;
    }
};
