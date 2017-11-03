#include <iostream>
#include <cmath>
#include <random>

using namespace std;

/// Number of optional launch arguments
#define kOPT_ARGS_COUNT     5

/// Default values of the launch arguments
#define kDEF_NOISE_RATIO    2.0f
#define kDEF_NOISE_PERIOD   3
#define kDEF_THREEOPT_TRSH  30
#define kDEF_BACKTRACK_PERIOD 1 // 1 => Always backtrack if necessary
#define kDEF_DB_PERIOD      10 // INT_MAX // => Never perform DB

class Location {
public:
    double x;
    double y;
    bool visited = false;
};

// Functions
int distance(Location);
void greedy_tour(Location *);
void three_opt();
void two_opt_fast();
void two_opt();
void double_bridge_move();
void noise();
int distance(Location, Location);
void swap_tour_pointers();
int swap(int, int, int);
int swap4(int, int, int, int, int, int, int, int);
int swap3(int, int, int, int, int, int);
int swap2(int, int, int, int);
int calculate_tour_length(int *);
void set_best_tour_if_possible(int, int *t);
//void check_correctness();
bool time_is_up();
bool time_is_up_noise();
void print_tour_coordinates(Location *);
void print_tour(int* t, string name);

// Global Variables
int** distance_between;
int* tour;
int* new_tour;
int* best_tour;
int best_length = numeric_limits<int>::max();;

static float kNoiseRatio;
static int   kNoisePeriod;
static int   kThreeOptThreshold;
static int   kBacktrackPeriod;
static int   kDoubleBridgePeriod;
static bool  kOutputTourLengthOnly;

int N;
clock_t start;

int main(int argc, char **argv) {

    if (argc == kOPT_ARGS_COUNT+1) {

        // Use passed values
        kNoiseRatio = atof(argv[1]);
        kNoisePeriod = atoi(argv[2]);
        kThreeOptThreshold = atoi(argv[3]);
        kBacktrackPeriod = atoi(argv[4]);
        kDoubleBridgePeriod = atoi(argv[5]);
        kOutputTourLengthOnly = true;

    } else if (argc > 1) {
        cerr << "Unhandled extra parameters!\n";
        exit(EXIT_FAILURE);

    } else {

        // Use default values
        kNoiseRatio = kDEF_NOISE_RATIO;
        kNoisePeriod = kDEF_NOISE_PERIOD;
        kThreeOptThreshold = kDEF_THREEOPT_TRSH;
        kBacktrackPeriod = kDEF_BACKTRACK_PERIOD;
        kDoubleBridgePeriod = kDEF_DB_PERIOD;
        kOutputTourLengthOnly = false;
    }

    start = clock();
    string line;
    // Read first line from stdin
    getline(cin, line);
    // First line is number of locations
    N = stoi(line);

    // Allocate locations array
    Location* locations = new Location[N];

    tour = new int[N]; // allocate tour array
    new_tour = new int[N]; // holds improved tour attempt
    best_tour = new int[N];

    // Allocate distance_between
    distance_between = new int*[N];
    for(int i = 0; i < N; i++) {
        distance_between[i] = new int[N];
    }


    // Setup locations array
    for(int i = 0, space = 0; i < N; i++, space = 0) {
        getline(cin, line);
        while(line[space] != ' ') { space ++; }
        line[space] = '\0';
        locations[i].x = stod(line); // x now ends at index: space
        locations[i].y = stod(&line[space+1]); // y starts at index: space+1
    }

    for(int i = 0; i < N; i++) {
        for(int j = i; j < N; j++) {
            distance_between[i][j] = distance_between[j][i] = distance(locations[i], locations[j]);
        }
    }

    int do_greedy = true;
    if (do_greedy) {
        // set tour to greedy tour
        greedy_tour(locations);
    } else {
        for (int i = 0; i < N; i++) {
            tour[i] = i;
        }
    }

    if (N > 3) {
        two_opt_fast();
        int i = 0;
        // Really important that I save best tour before noise;
        set_best_tour_if_possible(calculate_tour_length(tour), tour);
        while (true) {
            if (time_is_up_noise()) break;
            if (N < 10) noise(); // I think this can be removed
            else {
                if (i % kNoisePeriod == 0) {
                    noise();
                    if (time_is_up_noise()) break;
                }
                else double_bridge_move();

                if (i % kDoubleBridgePeriod == 0) {
                    double_bridge_move();
                }
            }
            two_opt_fast();
            if (N < kThreeOptThreshold) three_opt(); // I think this can be removed
            int length = calculate_tour_length(tour);
            set_best_tour_if_possible(length, tour);
            if (length > best_length && (i % kBacktrackPeriod == 0)) {
                // Revert to best tour
                for (int j = 0; j < N; j++) {
                    tour[j] = best_tour[j];
                }
            }
            i++;
        }
    }

    three_opt();
    two_opt();

    set_best_tour_if_possible(calculate_tour_length(tour), tour);
    //cout << "end distance " << best_length << endl;

    if (kOutputTourLengthOnly) {
        cout << best_length << "\n";
    } else {
        for (int i = 0; i < N; i++) {
            cout << best_tour[i] << "\n";
        }
    }

    return 0;
}


/**
 * The greedy algorithm starts from some node and finds the shortest distance
 * from this node. The tour-variable is set in this function.
 * @param locations -
 */
void greedy_tour(Location *locations) {
    int from = 0;
    tour[0] = best_tour[0] = from; // set start location
    for(int i = 1; i < N; i++) {
        locations[from].visited = true; // location is now visited
        int min_distance = numeric_limits<int>::max(); // smallest distance
        int next_from = -1;
        // find smallest distance
        for(int to = 0; to < N; to++) {
            if(locations[to].visited) continue;
            if(distance_between[from][to] < min_distance) {
                min_distance = distance_between[from][to];
                next_from = to;
            }
        }
        tour[i] = best_tour[i] = next_from;
        from = next_from;
    }
}


void double_bridge_move() {
    int i = rand() % (N-7);
    int j = (rand() % (N-5-(i+2))) + i+2;
    int k = (rand() % (N-3-(j+2))) + j+2;
    int l = (rand() % (N-1-(k+2))) + k+2;
    swap4(i, k+1, l, j+1, k, i+1, j, l+1);
    swap_tour_pointers();

}


void noise() {
    if (N < 4) return (void) 0;
    for (int z = 0; z < N; z++) {
        int i = rand() % (N-3);
        int k_min = i+2;
        // Generate random number from k_min to N-1 (or between i+1 to N)
        int k = rand() % (N-1-k_min) + k_min;
        int new_dist = distance_between[tour[i]][tour[k]] + distance_between[tour[i+1]][tour[k+1]];
        int old_dist = distance_between[tour[i]][tour[i+1]] + distance_between[tour[k]][tour[k+1]];
        if (new_dist < old_dist*kNoiseRatio) {
            swap2(i, k, i+1, k+1);
            swap_tour_pointers();
        }
    }
}


void two_opt_fast() {
    int last;
    bool found = false;
    int times = 0;
    int pos = 0;
    while (times <= N*6) {
        // do rand on i
        int i = rand() % (N-3);

        for (int j = i + 2; j < N; j++) {
            if (j + 1 == N) last = 0;
            else last = j + 1;

            if (distance_between[tour[i]][tour[j]] +
                distance_between[tour[i + 1]][tour[last]] <
                distance_between[tour[i]][tour[i + 1]] +
                distance_between[tour[j]][tour[last]]) {
                if (found == false) {
                    pos = swap(0, 0, i);
                    found = true;
                }
                pos = swap(pos, j, i+1);
                i = j;
                j = i + 2;
            }
        }

        // if we have found something, fill the rest up until N-1
        if (found) {
            if (pos <= N - 1) pos = swap(pos, pos, N - 1);
            swap_tour_pointers();
            found = false;
        }
        times ++;
    }

}


void two_opt() {
    int last;
    start:
    for(int i = 0; i < N-1; i++) {
        for(int j = i+2; j < N; j++) {
            if (time_is_up()) return (void) 0;
            // If (j+1 == N) it means we are the edge that is between the last node and the first node
            // Swapping the first edge with the last edge is wrong, because there has to be an edge between
            // (i, i+1) and (j, j+1) i.e. i+1 != j
            if (i == 0 && j+1 == N) continue;
            if (j+1 == N) last = 0;
            else last = j+1;

            int old_dist =
                    distance_between[tour[i]][tour[i+1]] +
                    distance_between[tour[j]][tour[last]];
            int dist =
                    distance_between[tour[i]][tour[j]] +
                    distance_between[tour[i+1]][tour[last]];

            if (dist < old_dist) {
                swap2(i, j, i+1, j+1);
                swap_tour_pointers();
                goto start;
            }
        }
    }
}


void three_opt() {
    int last;
    if (N < 6) return (void) 0;
    start:
    // There always has to be one apart,
    // For instance, if i is 10, j must start at 12 and k must start at 14
    // k may end at N-1, which means the edge (N-1, 0) which is the last edge,
    // that means j must end at N-3 and i at N-5
    for(int i = 0; i <= N-5; i++) {
        for(int j = i+2; j <= N-3; j++) {
            for(int k = j+2; k <= N-1; k++) {
                if (i == 0 && k+1 == N) continue;
                if (k+1 == N) last = 0;
                else last = k+1;

                if (time_is_up()) return (void) 0;
                int old_dist =
                        distance_between[tour[i]][tour[i + 1]] +
                        distance_between[tour[j]][tour[j + 1]] +
                        distance_between[tour[k]][tour[last]];
                // Real three-opt moves
                int dist1 =
                        distance_between[tour[i]][tour[j]] +
                        distance_between[tour[i + 1]][tour[k]] +
                        distance_between[tour[j + 1]][tour[last]];

                int dist2 =
                        distance_between[tour[i]][tour[j + 1]] +
                        distance_between[tour[k]][tour[i + 1]] +
                        distance_between[tour[j]][tour[last]];
                int dist3 =
                        distance_between[tour[i]][tour[k]] +
                        distance_between[tour[j + 1]][tour[i + 1]] +
                        distance_between[tour[j]][tour[last]];

                int min_dist = min(min(dist1, dist2), dist3);

                if (min_dist < old_dist) {
                    if (min_dist == dist1) {
                        // From 0 to i, from j to i+1, from k to j+1, from k+1
                        swap3(i, j, i+1, k, j+1, k+1);
                    } else if (min_dist == dist2) {
                        swap3(i, j+1, k, i+1, j, k+1);
                    } else if (min_dist == dist3) {
                        swap3(i, k, j+1, i+1, j, k+1);
                    }
                    swap_tour_pointers();
                    goto start;
                }
            }
        }
    }
}

int swap(int pos, int from, int to) {
    if (from < to) {
        // go forward
        for (int j = from; j <= to; j++) {
            new_tour[pos] = tour[j];
            pos++;
        }
    } else if (from > to) {
        // go backward
        for (int j = from; j >= to; j--) {
            new_tour[pos] = tour[j];
            pos++;
        }
    } else {
        new_tour[pos] = tour[pos];
        pos++;
    }
    return pos;
}

int swap4(int a, int b, int c, int d, int e, int f, int g, int h) {
    int pos = swap(0, 0, a);
    pos = swap(pos, b, c);
    pos = swap(pos, d, e);
    pos = swap(pos, f, g);
    if (h <= N-1) pos = swap(pos, h, N-1);
    return pos;
}

int swap3(int a, int b, int c, int d, int e, int f) {
    int pos = swap(0, 0, a);
    pos = swap(pos, b, c);
    pos = swap(pos, d, e);
    if (f <= N-1) pos = swap(pos, f, N-1);
    return pos;
}


int swap2(int a, int b, int c, int d) {
    int pos = swap(0, 0, a);
    pos = swap(pos, b, c);
    if (d <= N-1) swap(pos, d, N-1);
    return pos;
}

/**
 * 1. tour becomes new_tour after swap
 * 2. new_tour takes over the array that tour occupied
 */
void swap_tour_pointers() {
    swap(tour, new_tour);

}

void set_best_tour_if_possible(int tour_length, int *t) {
    if (tour_length < best_length) {
        // cout << "setting best tour " << tour_length << endl;
        for(int i = 0; i < N; i++) {
            best_tour[i] = t[i];
        }
        best_length = tour_length;
    }
}

/**
 * Calculates the euclidean distance between two locations
 * @param loc1 A Location
 * @param loc2 A Location
 * @return Rounded value
 */
int distance(Location loc1, Location loc2) {
    return round(sqrt((loc1.x-loc2.x)*(loc1.x-loc2.x)+(loc1.y-loc2.y)*(loc1.y-loc2.y)));
}

int calculate_tour_length(int *t) {
    int sum = 0;
    for(int i = 0; i < N-1; i++) {
        sum += distance_between[t[i]][t[i+1]];
    }
    sum += distance_between[t[N-1]][0];
    return sum;
}

bool time_is_up() {
    return ((clock() - start)/(double)CLOCKS_PER_SEC) > 1.98;
}

bool time_is_up_noise() {
    return ((clock() - start)/(double)CLOCKS_PER_SEC) > 1.95;
}

//void check_correctness() {
//    sort(best_tour, best_tour + N);
//    for (int i = 0; i < N; i++) {
//        if (best_tour[i] != i)  { cout << "ERROR: malformed path"; exit(0); }
//        if (best_tour[i] < 0 || best_tour[i] > N-1) { cout << "ERROR: outside range"; exit(0); }
//    }
//}

void print_tour_coordinates(Location *locations) {
    for(int i = 0; i < N; i++) {
        cout << locations[tour[i]].x << " " << locations[tour[i]].y << "\n";
    }
    cout << "\n";
}

void print_tour(int* t, string name) {
    cout << "~printing " << name << "'s tour" << endl;
    for(int i = 0; i < N; i++) {
        cout << "[" << i << "]: "<< t[i] << endl;
    }
}
