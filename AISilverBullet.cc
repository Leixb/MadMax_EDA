#include "Player.hh"
#include <list>


/**
 * Write the name of your player and save this file
 * with the same name and .cc extension.
 */
#define PLAYER_NAME SilverBullet


struct PLAYER_NAME : public Player {

    /**
     * Factory: returns a new instance of this class.
     * Do not modify this function.
     */
    static Player* factory () {
        return new PLAYER_NAME;
    }

    /**
     * Types and attributes for your player can be defined here.
     */

    // Config
    const int INF=1e8;

    // Types
    struct Warrior_t;
    struct Car_t;

    typedef vector<vector<int> > dmap;

    // Global attributes
    dmap water_map, fuel_map;
    vector<dmap> cities_map;

    map<int, Warrior_t> registered_warriors;
    map<int, Car_t> registered_cars;

    // Helper functions

    void bfs(queue<pair<Pos, int> > &q, dmap &m, const bool &cross_city=false);
    void compute_maps();

    list<Dir> get_dir_from_dmap(const Pos &p, const dmap &m);
    void remove_unsafe_dirs(const Unit &u, list<Dir> &l);

    // Game routines
    void init();
    void move_cars();
    void move_warriors();

    void move_car(const int &car_id);
    void move_warrior(const int &warrior_id);

    /**
     * Play method, invoked once per each round.
     */
    virtual void play () {
        if (round() == 0) init();
        move_cars();
        move_warriors();
    }

};

struct PLAYER_NAME::Warrior_t {
    enum state_t {
        NONE        =0,
        FOLLOW_DMAP =1<<0,
        ATTACK      =1<<1,
        HOLD        =1<<2
    };
    short state;

    stack<dmap*> dmaps;

    Warrior_t(): state(NONE) {};
};

struct PLAYER_NAME::Car_t {
    enum state_t {
        NONE        =0,
        FOLLOW_DMAP =1<<0,
        ATTACK      =1<<1,
        PATROL      =1<<2,
        HUNT        =1<<3
    };
    short state;

    int unit_id; // For hunt
    stack<dmap*> dmaps;

    Car_t(): state(HUNT|ATTACK) {};
};

void PLAYER_NAME::init() {
    compute_maps();
}

void PLAYER_NAME::compute_maps() {
    queue<pair<Pos, int> > w, f;
    for (int i=0; i < rows(); ++i) {
        for (int j=0; j < cols(); ++j) {
            const CellType ct = cell(Pos(i, j)).type;
            if (ct == Water) w.emplace(Pos(i, j), 0);
            else if (ct == Station) f.emplace(Pos(i, j), 0);
            else if (ct == City) {
                //TODO
            }
        }
    }
    bfs(w, water_map, true);
    bfs(f, fuel_map, false);
}

void PLAYER_NAME::bfs(queue<pair<Pos, int> > &q, dmap &m, const bool &cross_city) {
    m = dmap(rows(), vector<int>(cols(), INF));
    while(!q.empty()) {
        const Pos p = q.front().first;
        const int d = q.front().second;
        q.pop();

        if (m[p.i][p.j] != INF) continue;

        m[p.i][p.j] = d;

        for (int i = 0; i < DirSize-1; ++i) {
            const Pos p2 = p + Dir(i);
            if (pos_ok(p2)) {
                const CellType ct = cell(p2).type;
                if (ct == Road or ct == Desert or (ct == City and cross_city))
                    q.emplace(p2, d+1);
            }
        }
    }
}

void PLAYER_NAME::move_cars() {
    for (const int &car_id : cars(me()))
        if (can_move(car_id))
            move_car(car_id);
}

void PLAYER_NAME::move_warriors() {
    if (round()% 4 != me()) return; // This line makes a lot of sense.

    for (const int &warrior_id : warriors(me()))
        move_warrior(warrior_id);
}

void PLAYER_NAME::move_car(const int &car_id) {}
void PLAYER_NAME::move_warrior(const int &warrior_id) {}

list<Dir> PLAYER_NAME::get_dir_from_dmap(const Pos &p, const dmap &m) {}
void PLAYER_NAME::remove_unsafe_dirs(const Unit &u, list<Dir> &l) {}

/**
 * Do not modify the following line.
 */
RegisterPlayer(PLAYER_NAME);

// vim:sw=4:ts=4:et:foldmethod=indent:
