#include "Player.hh"
#include <list>

#ifdef DEBUG
#define LOG(x) do { cerr << "debug: " << x << endl; } while(0);
#else
#define LOG(x)
#endif

/**
 * Write the name of your player and save this file
 * with the same name and .cc extension.
 */
#define PLAYER_NAME S1lv3rBull3t

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

    const char CITY_FIGHT_RATIO=75; // Percentatge of probability to win when taking fight
    const char WATER_MARGIN=8;
    const char FOOD_MARGIN=7;
    const char FUEL_MARGIN=7;
    const char MOVE_OUT_LIMIT=3; // Warriors to leave in city
    const char MAX_DIST_CAR=50;

    const bool AVOID_ENEMY_CARS=true;

    // Types
    struct Warrior_t;
    struct Car_t;

    typedef vector<vector<int> > dmap;

    // Global attributes
    dmap water_map, fuel_map, fuel_map_empty;

    dmap nearest_city;
    vector<dmap> cities_map;
    vector<vector<Pos> > cities;

    map<int, Warrior_t> registered_warriors;
    map<int, Car_t> registered_cars;

    dmap movements;
    dmap enemy_cars;

    vector<vector<int> > warriors_player_city;
    vector<int> enemy_warriors_city;
    vector<int> moved_warriors_city;

    // Helper functions

    void bfs(queue<pair<Pos, int> > &q, dmap &m, const bool &cross_city=false);
    void compute_maps();
    void explore_city(const Pos &p, const int &city, queue<pair<Pos, int> > &q);
    void map_nearest_city(queue<pair<Pos, int> > &q);

    void compute_warriors_city();
    void compute_warriors_movable();

    inline int warrior_diff_city(const int &city);

    inline dmap &nearest_city_dmap(const Pos &p);

    typedef pair<int, Pos> fq_t;
    void compute_fuel_map(priority_queue<fq_t, vector<fq_t>, greater<fq_t> > &q);
    Dir get_dir_to_warrior(const list<Dir> &l, const Pos &_p);
    int dist_pos(const Pos &a, const Pos &b);

    list<Dir> get_dir_from_dmap(const Unit &u, const dmap &m);
    void remove_unsafe_dirs(const Unit &u, list<Dir> &l);
    bool is_unsafe(const Unit &u, const Dir &dr);

    void mark_enemy_cars();
    void mark_car_reach(const Pos &p, const int &depth=5);

    inline bool fight(const int &attacker_id, const int &victim_id);
    bool fight_desert(const Unit &attacker_id, const Unit &victim_id);
    inline bool fight_city(const Unit &attacker_id, const Unit &victim_id);

    void assign_city(Warrior_t &w, const Pos &p);

    void check_suplies(Warrior_t &w, const Unit &u);
    inline bool needs_water(const Unit &u);
    inline bool needs_food(const Unit &u);

    void check_suplies(Car_t &c, const Unit &u);
    inline bool needs_fuel(const Unit &u);

    inline void register_and_move(const int &id, const Pos &p, const Dir &d);

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

#ifdef DEBUG
    void show_dmap(const dmap &m) {
        for (int i=0; i < rows(); ++i) {
            for (int j=0; j < cols(); ++j) {
                const int &v = m[i][j];
                if (v == INF) cerr << "++";
                else if (v == -1) cerr << "[]";
                else if (v%100 < 10) cerr << ' ' << v%100;
                else cerr << v%100;
                cerr << ' ';
            }
            cerr << endl;
        }
    }
#endif

};

struct PLAYER_NAME::Warrior_t {

    bool water, food;

    short last_seen, city;

    Warrior_t(): water(false), food(false), last_seen(0), city(-1) {};

};

struct PLAYER_NAME::Car_t {
    bool fuel;
    short last_seen;

    Car_t(): fuel(false), last_seen(0) {};
};

void PLAYER_NAME::init() {
    movements = dmap(rows(), vector<int> (cols(), -1));
    enemy_cars = dmap(rows(), vector<int> (cols(), -1));
    compute_maps();
#ifdef DEBUG
    LOG("-- WATER_MAP --")
        show_dmap(water_map);
    LOG("-- FUEL_MAP EMPTY --")
        show_dmap(fuel_map_empty);
    LOG("-- FUEL_MAP --")
        show_dmap(fuel_map);
    LOG("-- NEAREST CITY MAP --")
        show_dmap(nearest_city);
    for (int i = 0; i < nb_cities(); ++i) {
        LOG("-- CITY " << i << " --");
        show_dmap(cities_map[i]);
    }
#endif
}

void PLAYER_NAME::compute_maps() {
    queue<pair<Pos, int> > w, f; // water & fuel queues

    priority_queue<fq_t, vector<fq_t>, greater<fq_t> > fq;

    vector<queue<pair<Pos, int> > > cq(nb_cities());
    nearest_city = dmap(rows(), vector<int> (cols(), INF));
    cities_map = vector<dmap> (nb_cities(), dmap(rows(), vector<int> (cols(), INF)));
    cities = vector<vector<Pos> > (nb_cities());

    int city = 0;

    for (int i=0; i < rows(); ++i) {
        for (int j=0; j < cols(); ++j) {
            const CellType ct = cell(Pos(i, j)).type;
            if (ct == Water) w.emplace(Pos(i, j), -1); //-1 since we cannot get into water
            else if (ct == Station) {
                f.emplace(Pos(i, j), -1);  // pair<Pos, int>
                fq.emplace(-1, Pos(i, j)); // pair<int, Pos>
            } else if (ct == City) {
                if (nearest_city[i][j] != INF) continue;
                explore_city(Pos(i, j), city, cq[city]);
                ++city;
            }
        }
    }

    LOG("FOUND " << city << "/" << nb_cities() << " cities ");

    compute_fuel_map(fq);

    bfs(w, water_map, true);
    bfs(f, fuel_map_empty, false);

    queue<pair<Pos, int> > ncq; // nearest_city queue

    for (int i = 0; i < nb_cities(); ++i) {
        for (const Pos &p : cities[i]) ncq.emplace(p, i);
        bfs(cq[i], cities_map[i], true);
    }

    map_nearest_city(ncq);
}

void PLAYER_NAME::mark_enemy_cars() {
    for(const int &player : random_permutation(nb_players()-1)) {
        if (player == me()) continue;
        for (const int &car_id : cars(player)) {
            mark_car_reach(unit(car_id).pos);
        }
    }
}


void PLAYER_NAME::mark_car_reach(const Pos &p, const int &depth) {

    priority_queue<pair<int, Pos> > q;
    q.emplace(0, p);
    while (!q.empty()) {
        const int d = q.top().first;
        const Pos p = q.top().second;
        q.pop();

        if (d > depth) continue;

        if (enemy_cars[p.i][p.j] == round()) continue;
        enemy_cars[p.i][p.j] = round();

        for (int i = 0; i < DirSize-1; ++i) {
            const Pos p2 = p + Dir(i);
            if (!pos_ok(p2)) continue;

            const CellType ct = cell(p2).type;

            if (ct == Desert) q.emplace(d + nb_players(), p2);
            else if (ct == Road) q.emplace(d + 1, p2);
        }
    }
}

void PLAYER_NAME::compute_fuel_map(priority_queue<fq_t, vector<fq_t>, greater<fq_t> > &q) {
    fuel_map = dmap(rows(), vector<int>(cols(), INF));
    while(!q.empty()) {
        const int d = q.top().first;
        const Pos p = q.top().second;
        q.pop();

        if (fuel_map[p.i][p.j] != INF) continue;
        fuel_map[p.i][p.j]=d;

        for (int i = 0; i < DirSize-1; ++i) {
            const Pos p2 = p + Dir(i);
            if (!pos_ok(p2)) continue;

            const CellType ct = cell(p2).type;

            if (d == -1) q.emplace(0, p2);
            else if (ct == Desert) q.emplace(d + nb_players(), p2);
            else if (ct == Road) q.emplace(d + 1, p2);
        }
    }
}

void PLAYER_NAME::map_nearest_city(queue<pair<Pos, int> > &q) {
    while (!q.empty()) {
        const Pos p = q.front().first;
        const int city = q.front().second;
        q.pop();

        if (nearest_city[p.i][p.j] < INF) continue; // !! care explore_city use of nearest_city
        nearest_city[p.i][p.j] = city;

        for (int i = 0; i < DirSize-1; ++i) {
            const Pos p2 = p + Dir(i);
            if (!pos_ok(p2)) continue;

            const CellType ct = cell(p2).type;
            if (ct == Road or ct == Desert) q.emplace(p2, city);
        }
    }
}

void PLAYER_NAME::bfs(queue<pair<Pos, int> > &q, dmap &m, const bool &cross_city) {
    m = dmap(rows(), vector<int>(cols(), INF));

    // Move all -1 to borders without using diagonal moves (they dont refill)

    while(!q.empty()) {
        const Pos p = q.front().first;
        if (q.front().second != -1) break;
        q.pop();

        m[p.i][p.j] = -1;

        for (Dir i : {Bottom, Right, Top, Left}) {
            const Pos p2 = p + Dir(i);
            if (!pos_ok(p2)) continue;

            const CellType ct = cell(p2).type;
            if (ct == Road or ct == Desert or (ct == City and cross_city))
                q.emplace(p2, 0);
        }
    }

    // We can start now

    while(!q.empty()) {
        const Pos p = q.front().first;
        const int d = q.front().second;
        q.pop();

        if (m[p.i][p.j] != INF) continue;

        m[p.i][p.j] = d;

        for (int i = 0; i < DirSize-1; ++i) {
            const Pos p2 = p + Dir(i);
            if (!pos_ok(p2)) continue;

            const CellType ct = cell(p2).type;
            if (ct == Road or ct == Desert or (ct == City and cross_city))
                q.emplace(p2, d+1);
        }
    }
}

void PLAYER_NAME::explore_city(const Pos &_p, const int &city, queue<pair<Pos, int> > &q) {
    queue<Pos> Q;
    Q.push(_p);

    while (!Q.empty()) {
        const Pos p = Q.front();
        Q.pop();

        if (nearest_city[p.i][p.j] != INF) continue;
        nearest_city[p.i][p.j] = INF+1; // !! care with map_nearest_city

        cities[city].push_back(p);

        for (int i = 0; i < DirSize-1; ++i) {
            const Pos p2 = p + Dir(i);
            if (!pos_ok(p2)) continue;

            if (cell(p2).type == City) {
                Q.push(p2);
                q.emplace(p2, 0);
            }
        }
    }
}

void PLAYER_NAME::compute_warriors_city() {
    warriors_player_city = vector<vector<int> > (nb_players(), vector<int>(nb_cities(), 0));
    for (int i = 0; i < nb_cities(); ++i) {
        for (const Pos &p : cities[i]) {
            if (cell(p).id != -1) {
                ++warriors_player_city[unit(cell(p).id).player][i];
            }
        }
    }
}
void PLAYER_NAME::compute_warriors_movable() {
    enemy_warriors_city = vector<int> (nb_cities(), 0);
    for (int i = 0; i < nb_cities(); ++i) {
        for (int j = 0; j < nb_players(); ++j) {
            if (j == me()) continue;
            enemy_warriors_city[i] = max(enemy_warriors_city[i], warriors_player_city[j][i]);
        }
    }
}

void PLAYER_NAME::move_cars() {
    for (const int &car_id : cars(me()))
        if (can_move(car_id))
            move_car(car_id);
}

void PLAYER_NAME::move_warriors() {
    if (round()%nb_players() != me()) return; // This line makes a lot of sense.

    if (AVOID_ENEMY_CARS) {
        mark_enemy_cars();
    }

    compute_warriors_city();
    compute_warriors_movable();

    moved_warriors_city = vector<int> (nb_cities(), 0);

#ifdef DEBUG
    LOG("-- ENEMY CARS --");
    show_dmap(enemy_cars);
#endif

    for (const int &warrior_id : warriors(me()))
        move_warrior(warrior_id);
}

void PLAYER_NAME::move_car(const int &car_id) {
    Car_t &c = registered_cars[car_id];
    const Unit u = unit(car_id);

    LOG("Car ID:" << car_id);

    // If we haven't seen him for 4(nb_players) rounds he has died and respawned
    if (c.last_seen+nb_players() < round()) {
        LOG("RESPAWNED");
        c = Car_t();
    }

    c.last_seen = round();

    check_suplies(c, u);

    if (c.fuel) {
        dmap *fm = &fuel_map;
        if (u.food == 0)  *fm = fuel_map_empty;

        list<Dir> l = get_dir_from_dmap(u, *fm);
        if (l.empty()) {
            for (const int &i : random_permutation(DirSize-1)) {
                if (!pos_ok(u.pos + Dir(i))) continue;
                l.push_back(Dir(i));
            }
            remove_unsafe_dirs(u, l);
            if (l.empty()) {
                LOG("No safe moves " << car_id);
                return;
            }
        }
        register_and_move(car_id, u.pos, l.front());
        return;
    }

    list<Dir> l;
    for (const int &i : random_permutation(DirSize-1)) {
        Pos p = u.pos + Dir(i);

        if (!pos_ok(p)) continue;

        const CellType ct = cell(p).type ;
        if (ct == Road) l.emplace_front(Dir(i));
        else if (ct == Desert) l.emplace_back(Dir(i));
    }

    remove_unsafe_dirs(u, l);

    if (l.empty()) {
        LOG("NO SAFE MOVES FOR CAR");
        return;
    }

    const Dir dr = get_dir_to_warrior(l, u.pos);

    // If no warrior near, go to fuel station
    if (dr == None) {
        list<Dir> l = get_dir_from_dmap(u, fuel_map);
        if (l.empty()) {
            LOG("WILL NOT MOVE, CAR DOOMED");
            return;
        }
        register_and_move(car_id, u.pos, l.front());
    } else register_and_move(car_id, u.pos, dr);
}

// Cartesian distance(without sqrt)
int PLAYER_NAME::dist_pos(const Pos &a, const Pos &b) {
    return (a.i - b.i)*(a.i-b.i) + (a.j - b.j)*(a.j-b.j);
}

Dir PLAYER_NAME::get_dir_to_warrior(const list<Dir> &l, const Pos &_p) {

    priority_queue<tuple<int, Dir, Pos>, vector<tuple<int, Dir, Pos> >, greater<
        tuple<int, Dir, Pos> > > q;

    for (const Dir &dr : l) {
        const int d = (cell(_p+dr).type == Road)? 1 : nb_players();
        q.emplace(d, dr, _p+dr);
    }

    vector<vector<bool> > visited(rows(), vector<bool> (cols(), false));

    while (!q.empty()) {
        int d;
        Dir dr;
        Pos p;
        tie(d, dr, p) = q.top();
        q.pop();

        if (d >= MAX_DIST_CAR) continue;

        if (visited[p.i][p.j]) continue;
        visited[p.i][p.j] = true;

        const int c_id = cell(p).id;
        if (c_id != -1) {
            const Unit u = unit(c_id);
            if (u.player != me() and u.type == Warrior) {
                LOG("FOUND WARRIOR at: (" << u.pos.i << ',' << u.pos.j << ')');
                LOG("Direction: " << dr);
                return dr;
            }
        }

        for (int i = 0; i < DirSize-1; ++i) {
            const Pos p2 = p + Dir(i);
            if (!pos_ok(p2)) continue;

            const CellType ct = cell(p2).type;

            if (ct == Desert) q.emplace(d + nb_players(), dr, p2);
            else if (ct == Road) q.emplace(d + 1, dr, p2);
        }

    }

    return Dir(None);
}

void PLAYER_NAME::check_suplies(Car_t &c, const Unit &u) {
    c.fuel = needs_fuel(u);
}

bool PLAYER_NAME::needs_fuel(const Unit &u) {
    const int d = fuel_map[u.pos.i][u.pos.j];
    return u.food - FUEL_MARGIN < d;
}

void PLAYER_NAME::move_warrior(const int &warrior_id) {
    Warrior_t &w = registered_warriors[warrior_id];
    const Unit u = unit(warrior_id);

    LOG("Warrior ID: " << warrior_id);
    LOG("  Water: " << u.water << " Food: " << u.food);
    LOG("  d2Water:" << water_map[u.pos.i][u.pos.j]);
    LOG("  d2Food: " << nearest_city_dmap(u.pos)[u.pos.i][u.pos.j]);

    // If we haven't seen him for 4(nb_players) rounds he has died and respawned
    if (w.last_seen+nb_players() < round()) {
        LOG("RESPAWNED");
        w = Warrior_t();
    }

    w.last_seen = round();

    assign_city(w, u.pos);

    check_suplies(w, u);

    dmap *m = &cities_map[w.city];

    if (w.water) m = &water_map;
    if (w.food) m = &nearest_city_dmap(u.pos);

    if (w.food and w.water) {
        if (u.food > u.water) m = &water_map;
        else m = &nearest_city_dmap(u.pos);
    }

    list<Dir> l = get_dir_from_dmap(u, *m);
    if (l.empty()) {
        for (const int &i : random_permutation(DirSize-1)) {
            if (!pos_ok(u.pos + Dir(i))) continue;
            l.push_back(Dir(i));
        }
        remove_unsafe_dirs(u, l);
        if (l.empty()) {
            LOG("No safe moves " << warrior_id);
            return;
        }
    }

    if (cell(u.pos).type == City and cell(u.pos + l.front()).type != City) {
        ++moved_warriors_city[nearest_city[u.pos.i][u.pos.j]];
    }

    register_and_move(warrior_id, u.pos, l.front());
}

void PLAYER_NAME::check_suplies(Warrior_t &w, const Unit &u) {
    w.water = (w.water and u.water < warriors_health()) or needs_water(u);
    w.food = (w.food and u.water < warriors_health()) or needs_food(u);
}

bool PLAYER_NAME::needs_water(const Unit &u) {
    const int d = water_map[u.pos.i][u.pos.j];
    return u.water - WATER_MARGIN< d;
}

bool PLAYER_NAME::needs_food(const Unit &u) {
    const int c = nearest_city[u.pos.i][u.pos.j];
    const int d = cities_map[c][u.pos.i][u.pos.j];
    return u.food - FOOD_MARGIN < d;
}

void PLAYER_NAME::register_and_move(const int &id, const Pos &p, const Dir &d) {
    const Pos p2 = p + d;
    movements[p2.i][p2.j] = round();
    command(id, d);
}

PLAYER_NAME::dmap &PLAYER_NAME::nearest_city_dmap(const Pos &p) {
    return cities_map[nearest_city[p.i][p.j]];
}

inline int PLAYER_NAME::warrior_diff_city(const int &city) {
    return warriors_player_city[me()][city] - enemy_warriors_city[city];
}

void PLAYER_NAME::assign_city(Warrior_t &w, const Pos &p) {
    const int &city = nearest_city[p.i][p.j];
    w.city = city;
    //if (cell(p).type == City) {
        // If leaving makes us lose city, stay.
        LOG("ENEMIES IN CITY: " << enemy_warriors_city[city] << endl
            << "ALLIES:" << warriors_player_city[me()][city] << endl
            << "moved: " << moved_warriors_city[city] << endl);
        //if (cell(p).owner != me()) return;
        if (warrior_diff_city(city) - moved_warriors_city[city] - 1 < MOVE_OUT_LIMIT) {
            LOG("STAYING NEAREST CITY");
            return;
        }
    //}
    for (const int &i : random_permutation(nb_cities()-1)) {
        if (cities_map[i][p.i][p.j] < warriors_health()-FOOD_MARGIN) {
            w.city = i;
            // try to find unowned cities
            if (cell(cities[i][0]).owner != me()) return;
        }
    }
    LOG("No suitable cities found Nearest taken");
}

list<Dir> PLAYER_NAME::get_dir_from_dmap(const Unit &u, const dmap &m) {
    const Pos p = u.pos;
    const int d = m[p.i][p.j];
    list<Dir> ls, eq;
    for (const int &i : random_permutation(DirSize-1)) {
        const Pos p2 = p + Dir(i);
        if (!pos_ok(p2)) continue;

        if (m[p2.i][p2.j] < d ) ls.push_back(Dir(i));
        else if (m[p2.i][p2.j] == d) eq.push_back(Dir(i));
    }
    remove_unsafe_dirs(u, ls);
    if (!ls.empty()) return ls;

    remove_unsafe_dirs(u, eq);
    return eq;
}

bool PLAYER_NAME::is_unsafe(const Unit &u, const Dir &dr) {
    const Pos p = u.pos + dr;
    const int u_id = cell(p).id;
    if (movements[p.i][p.j] == round()) return true;

    if (AVOID_ENEMY_CARS) {
        if (u.type == Warrior and enemy_cars[p.i][p.j] == round()) return true;
    }

    if (u_id != -1) {
        if (unit(u_id).player == me()) return true;
        if (!fight(u.id, u_id)) return true;
    }
    for (int i = 0; i < DirSize-1; ++i) {
        const Pos p2 = p + Dir(i);
        if (!pos_ok(p2)) continue;

        const int u_id2 = cell(p2).id;
        if (u_id2 == -1) continue;

        if (unit(u_id2).player != me()) {
            // If moving inside city and menaced by car, we ok
            if (unit(u_id2).type == Car and cell(p).type == City)
                continue;
            // cheap fix for Car vs Car bug
            if (unit(u_id2).type == Car or fight(u_id2, u.id))
                return true;
        }
    }
    return false;
}

void PLAYER_NAME::remove_unsafe_dirs(const Unit &u, list<Dir> &l) {
    auto it = l.begin();
    while (it != l.end()) {
        if (is_unsafe(u, *it)) it = l.erase(it);
        else ++it;
    }
}

bool PLAYER_NAME::fight(const int &attacker_id, const int &victim_id) {
    const Unit attacker=unit(attacker_id), victim=unit(victim_id);
    if (cell(victim.pos).type == City) return fight_city(attacker, victim);
    return fight_desert(attacker, victim);
}

bool PLAYER_NAME::fight_desert(const Unit &attacker, const Unit &victim) {
    if (victim.type == Car) return false; // Never attack cars
    if (attacker.type == Car) return true; // Car wins
    if (victim.food <= 6 or victim.water <= 6) return true;

    if (victim.food > attacker.food and victim.water > attacker.water)
        return false;

    // TODO: check this is correct
    if (victim.food < victim.water)
        return attacker.food > victim.food and attacker.water > victim.food;
    return attacker.water > victim.water and attacker.food > victim.water;
}

bool PLAYER_NAME::fight_city(const Unit &attacker, const Unit &victim) {
    if (attacker.type == Car) return false; // Cannot move into victim
    return attacker.water*100/(attacker.water + victim.water) >= CITY_FIGHT_RATIO;
}

/**
 * Do not modify the following line.
 */
RegisterPlayer(PLAYER_NAME);

// vim:sw=4:ts=4:et:foldmethod=indent:foldlevel=20:
