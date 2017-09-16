#include <iostream>
#include <cmath>
#include <random>
#include <cstdlib>
#include <new>
#include <limits>
#include <ctime>
using namespace std;

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
void two_opt();
void noise();
int distance(Location, Location);
void swap_tour_pointers();
int swap(int, int, int);
int swap3(int, int, int, int, int, int);
int swap2(int, int, int, int);
int calculate_tour_length(int *);
void set_best_tour_if_possible(int, int *t);
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

random_device rd;  //Will be used to obtain a seed for the random number engine
mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
uniform_real_distribution<> dis(0, 1);
typedef numeric_limits< double > dbl;

int N;
clock_t start;

int main() {
    start = clock();
    // Can print doubles
    cout.precision(dbl::max_digits10);
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

    // cout << "greedy tour distance " << calculate_tour_length(tour) << endl;

    if (N > 3) {
        two_opt();
        int i = 0;
        // Really important that I save best tour before noise;
        set_best_tour_if_possible(calculate_tour_length(tour), tour);
        while (true) {
            if (time_is_up()) break;
            noise();
            two_opt();
            if (i % 3 == 0) {
                three_opt();
                int length = calculate_tour_length(tour);
                set_best_tour_if_possible(length, tour);
                if (length > best_length) {
                    // Revert to best tour
                    for (int j = 0; j < N; j++) {
                        tour[j] = best_tour[j];
                    }
                }
            }
            i++;
        }
    }


    set_best_tour_if_possible(calculate_tour_length(tour), tour);
    // cout << "end distance " << best_length << endl;
    for(int i = 0; i < N; i++) {
        cout << best_tour[i] << "\n";
    }


    return 0;
}


/**
 * The greedy algorithm starts from some node and finds the shortest distance
 * from this node. The tour-variable is set in this function.
 * @param locations
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


void noise() {
    if (N < 4) return (void) 0;
    for (int z = 0; z < N; z++) {
        if (time_is_up_noise()) return (void) 0;
        int i = rand() % (N-3);
        int k_min = i+2;
        // Generate random number from k_min to N-1 (or between i+1 to N)
        int k = rand() % (N-1-k_min) + k_min;
        int new_dist = distance_between[tour[i]][tour[k]] + distance_between[tour[i+1]][tour[k+1]];
        int old_dist = distance_between[tour[i]][tour[i+1]] + distance_between[tour[k]][tour[k+1]];
        if (new_dist < old_dist + (int)(old_dist/5)) {
            swap2(i, k, i+1, k+1);
            swap_tour_pointers();
        }
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

int swap3(int a, int b, int c, int d, int e, int f) {
    int pos = swap(0, 0, a);
    pos = swap(pos, b, c);
    pos = swap(pos, d, e);
    if (f <= N-1) pos = swap(pos, f, N-1);
}


int swap2(int a, int b, int c, int d) {
    int pos = swap(0, 0, a);
    pos = swap(pos, b, c);
    if (d <= N-1) swap(pos, d, N-1);
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
    for(int i = 0; i < N; i++) {
        if (i == N-1) {
            sum += distance_between[t[i]][0];
        } else {
            sum += distance_between[t[i]][t[i+1]];
        }
    }
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