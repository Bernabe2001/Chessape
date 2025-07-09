#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <limits>
#include <utility>
#include <chrono>
#include <iomanip>
#include <cassert>
#include <math.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <fstream>         
#include <sqlite3.h>
#include <random>
#include <bitset>

#include "../../chess-library/include/chess.hpp"


using namespace chess;
using namespace std;
using Clock = std::chrono::steady_clock;

// Debug macro: prints only when compiled with -DDEBUG
#ifdef DEBUG
    #define DEBUG_PRINT(x) (std::cerr << x << std::endl)
    // Global counter for nodes analyzed during search (each recursive call is counted)
    uint64_t nodesAnalyzed = 0;
#else
    #define DEBUG_PRINT(x) ((void)0)
#endif

// Configuration values
constexpr int HASH_TABLE_SIZE = 2097152;  // 2^21
double R_FACTOR = 0.5;
double RANDOM_COEFF = 0.5;
int EARLY_GAME_MOVES = 50;
int MID_GAME_MOVES = 40;
int END_GAME_MOVES = 20;
double MOVETIME_MINIMUM = 0.2;
int INCR_MIN_DEPTH = 1;
int INCR_MAX_DEPTH = 2;
int FIRST_MIN_DEPTH = 5;
int FIRST_MAX_DEPTH = 14;

// Load configuration from file
void loadConfig() {
    ifstream file("../config.txt");
    if (!file.is_open()) {
        DEBUG_PRINT("[DEBUG] Could not open config.txt, using default values");
        return;
    }

    string line;
    while (getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;
        
        // Parse key=value pairs
        size_t pos = line.find('=');
        if (pos != string::npos) {
            string key = line.substr(0, pos);
            string value = line.substr(pos + 1);
            
            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            // Set values
            if (key == "r_factor") R_FACTOR = stod(value);
            else if (key == "random_coeff") RANDOM_COEFF = stod(value);
            else if (key == "early_game_moves") EARLY_GAME_MOVES = stoi(value);
            else if (key == "mid_game_moves") MID_GAME_MOVES = stoi(value);
            else if (key == "end_game_moves") END_GAME_MOVES = stoi(value);
            else if (key == "movetime_minimum") MOVETIME_MINIMUM = stod(value);
            else if (key == "incr_min_depth") INCR_MIN_DEPTH = stoi(value);
            else if (key == "incr_max_depth") INCR_MAX_DEPTH = stoi(value);
            else if (key == "first_min_depth") FIRST_MIN_DEPTH = stoi(value);
            else if (key == "first_max_depth") FIRST_MAX_DEPTH = stoi(value);
        }
    }
    
    DEBUG_PRINT("[DEBUG] Configuration loaded successfully");
}

constexpr short INFINITY_VAL = numeric_limits<short>::max();

short EXPECTED_MOVES_LEFT = EARLY_GAME_MOVES;

// Pawn structure tracking
vector<short> white_pawn_counts = vector<short>(8, 0);
vector<short> black_pawn_counts = vector<short>(8, 0);
unsigned char white_pawns = 0;
unsigned char black_pawns = 0;

enum class GameStage { EARLY, MID, END };

// In the Evaluation namespace, define separate piece–square tables for each stage.
// For now we copy the current values for early game into mid and endgame; you can tune them later.
namespace Evaluation {
    constexpr short PST_early[2][6][64] = {
        { // White tables
            {   // Pawn table × 1
                0,   0,   0,   0,   0,   0,   0,   0,
                90,  95,  95,  95,  95, 100,  95,  90,
                95,  95,  95, 110, 110,  90,  90,  95,
                90,  90, 100, 130, 130,  90,  90,  90,
               105, 105, 120, 140, 140, 120, 105, 105,
               125, 130, 140, 150, 150, 140, 130, 125,
               170, 180, 190, 210, 210, 190, 180, 170,
                 0,   0,   0,   0,   0,   0,   0,   0
           },
           {   // Knight table × 3
               240, 280, 280, 280, 280, 280, 260, 240,
               270, 290, 310, 315, 315, 310, 290, 270,
               280, 310, 315, 330, 330, 315, 310, 280,
               280, 315, 330, 335, 335, 330, 315, 280,
               280, 315, 330, 335, 335, 330, 315, 280,
               280, 310, 315, 330, 330, 315, 310, 280,
               270, 290, 310, 315, 315, 310, 290, 270,
               240, 280, 280, 280, 280, 280, 280, 240
           },
           {   // Bishop table × 3
               260, 280, 300, 290, 290, 300, 280, 260,
               280, 335, 300, 315, 315, 300, 335, 280,
               300, 305, 325, 310, 310, 325, 305, 300,
               315, 315, 325, 325, 325, 325, 315, 315,
               290, 325, 325, 325, 325, 325, 325, 290,
               300, 300, 325, 325, 325, 325, 300, 300,
               280, 335, 300, 315, 315, 300, 335, 280,
               260, 280, 300, 290, 290, 300, 280, 260
           },
           {   // Rook table × 5
               480, 480, 480, 505, 505, 480, 480, 480,
               480, 480, 490, 510, 510, 490, 480, 480,
               490, 490, 490, 515, 515, 490, 490, 490,
               490, 490, 490, 490, 490, 490, 490, 490,
               490, 490, 490, 490, 490, 490, 490, 490,
               490, 490, 490, 490, 490, 490, 490, 490,
               515, 515, 515, 530, 530, 515, 515, 515,
               500, 500, 500, 510, 510, 500, 500, 500
           },
           {   // Queen table × 9
               850, 870, 890, 905, 905, 890, 870, 850,
               870, 890, 905, 905, 905, 905, 890, 870,
               885, 905, 910, 910, 910, 910, 905, 885,
               905, 910, 910, 910, 910, 910, 910, 900,
               900, 910, 910, 910, 910, 910, 910, 905,
               900, 900, 910, 910, 910, 910, 900, 900,
               905, 905, 910, 910, 910, 910, 905, 905,
               900, 905, 910, 910, 910, 910, 905, 900
           },
           {   // King table × 0
               35,40,40,25,25,25,45,35,
               15,15,15,15,15,15,15,15,
                0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0
           }
       },
       { // Black tables
           {   // Pawn table × 1
                  0,   0,    0,    0,    0,     0,    0,    0,
               -170, -180, -190, -210, -210, -190, -180, -170,
               -125, -130, -140, -150, -150, -140, -130, -125,
               -105, -105, -120, -140, -140, -120, -105, -105,
                -90,  -90, -100, -130, -130,  -90,  -90,  -90,
                -95,  -95,  -95, -110, -110,  -90,  -90,  -95,
                -90,  -95,  -95,  -95,  -95, -100,  -95,  -90,
                  0,    0,    0,    0,    0,    0,    0,    0
           },
           {   // Knight table × 3
               -240, -280, -280, -280, -280, -280, -260, -240,
               -270, -290, -310, -315, -315, -310, -290, -270,
               -280, -310, -315, -330, -330, -315, -310, -280,
               -280, -315, -330, -335, -335, -330, -315, -280,
               -280, -315, -330, -335, -335, -330, -315, -280,
               -280, -310, -315, -330, -330, -315, -310, -280,
               -270, -290, -310, -315, -315, -310, -290, -270,
               -240, -280, -280, -280, -280, -280, -280, -240
           },
           {   // Bishop table × 3
               -260, -280, -300, -290, -290, -300, -280, -260,
               -280, -335, -300, -315, -315, -300, -335, -280,
               -300, -300, -325, -325, -325, -325, -300, -300,
               -290, -325, -325, -325, -325, -325, -325, -290,
               -315, -315, -325, -325, -325, -325, -315, -315,
               -300, -305, -325, -310, -310, -325, -305, -300,
               -280, -335, -300, -315, -315, -300, -335, -280,
               -260, -280, -300, -290, -290, -300, -280, -260
           },
           {   // Rook table × 5
               -500, -500, -500, -510, -510, -500, -500, -500,
               -515, -515, -515, -530, -530, -515, -515, -515,
               -490, -490, -490, -490, -490, -490, -490, -490,
               -490, -490, -490, -490, -490, -490, -490, -490,
               -490, -490, -490, -490, -490, -490, -490, -490,
               -490, -490, -490, -515, -515, -490, -490, -490,
               -480, -480, -490, -510, -510, -490, -480, -480,
               -480, -480, -480, -505, -505, -480, -480, -480
           },
           {   // Queen table × 9
               -900, -905, -910, -910, -910, -910, -905, -900,
               -905, -905, -910, -910, -910, -910, -905, -905,
               -900, -900, -910, -910, -910, -910, -900, -900,
               -900, -910, -910, -910, -910, -910, -910, -905,
               -905, -910, -910, -910, -910, -910, -910, -900,
               -885, -905, -910, -910, -910, -910, -905, -885,
               -870, -890, -905, -905, -905, -905, -890, -870,
               -850, -870, -890, -905, -905, -890, -870, -850     
           },
           {   // King table × 0
               0,  0,  0,  0,  0,  0,  0,  0,
               0,  0,  0,  0,  0,  0,  0,  0,
               0,  0,  0,  0,  0,  0,  0,  0,
               0,  0,  0,  0,  0,  0,  0,  0,
               0,  0,  0,  0,  0,  0,  0,  0,
               0,  0,  0,  0,  0,  0,  0,  0,
             -15,-15,-15,-15,-15,-15,-15,-15,
             -35,-40,-40,-25,-25,-25,-45,-35
           }
       }
    };
    constexpr short PST_mid[2][6][64] = {
        { // White tables
            {   // Pawn table × 1
                0,   0,   0,   0,   0,   0,   0,   0,
                90,  95,  95,  95,  95, 100,  95,  90,
                95,  95,  95, 110, 110,  90,  90,  95,
                90,  90, 100, 130, 130,  90,  90,  90,
               105, 105, 120, 140, 140, 120, 105, 105,
               125, 130, 140, 150, 150, 140, 130, 125,
               170, 180, 190, 210, 210, 190, 180, 170,
                 0,   0,   0,   0,   0,   0,   0,   0
           },
           {   // Knight table × 3
               240, 280, 280, 280, 280, 280, 260, 240,
               270, 290, 310, 315, 315, 310, 290, 270,
               280, 310, 315, 330, 330, 315, 310, 280,
               280, 315, 330, 335, 335, 330, 315, 280,
               280, 315, 330, 335, 335, 330, 315, 280,
               280, 310, 315, 330, 330, 315, 310, 280,
               270, 290, 310, 315, 315, 310, 290, 270,
               240, 280, 280, 280, 280, 280, 280, 240
           },
           {   // Bishop table × 3
               260, 280, 300, 290, 290, 300, 280, 260,
               280, 335, 300, 315, 315, 300, 335, 280,
               300, 305, 325, 310, 310, 325, 305, 300,
               315, 315, 325, 325, 325, 325, 315, 315,
               290, 325, 325, 325, 325, 325, 325, 290,
               300, 300, 325, 325, 325, 325, 300, 300,
               280, 335, 300, 315, 315, 300, 335, 280,
               260, 280, 300, 290, 290, 300, 280, 260
           },
           {   // Rook table × 5
               480, 480, 480, 505, 505, 480, 480, 480,
               480, 480, 490, 510, 510, 490, 480, 480,
               490, 490, 490, 515, 515, 490, 490, 490,
               490, 490, 490, 490, 490, 490, 490, 490,
               490, 490, 490, 490, 490, 490, 490, 490,
               490, 490, 490, 490, 490, 490, 490, 490,
               515, 515, 515, 530, 530, 515, 515, 515,
               500, 500, 500, 510, 510, 500, 500, 500
            },
           {   // Queen table × 9
               850, 870, 890, 900, 900, 890, 870, 850,
               870, 890, 900, 905, 905, 900, 890, 870,
               885, 905, 915, 915, 915, 915, 905, 885,
               905, 910, 910, 910, 910, 910, 910, 900,
               900, 910, 910, 910, 910, 910, 910, 905,
               900, 900, 910, 910, 910, 910, 900, 900,
               905, 905, 910, 910, 910, 910, 905, 905,
               900, 905, 910, 910, 910, 910, 905, 900
           },
           {   // King table × 0
               35,40,40,25,25,25,45,35,
               15,15,15,15,15,15,15,15,
               0, 0, 0, 0, 0, 0, 0, 0,
               0, 0, 0, 0, 0, 0, 0, 0,
               0, 0, 0, 0, 0, 0, 0, 0,
               0, 0, 0, 0, 0, 0, 0, 0,
               0, 0, 0, 0, 0, 0, 0, 0,
               0, 0, 0, 0, 0, 0, 0, 0
           }
       },
       { // Black tables
           {   // Pawn table × 1
                  0,   0,    0,    0,    0,     0,    0,    0,
               -170, -180, -190, -210, -210, -190, -180, -170,
               -125, -130, -140, -150, -150, -140, -130, -125,
               -105, -105, -120, -140, -140, -120, -105, -105,
                -90,  -90, -100, -130, -130,  -90,  -90,  -90,
                -95,  -95,  -95, -110, -110,  -90,  -90,  -95,
                -90,  -95,  -95,  -95,  -95, -100,  -95,  -90,
                  0,    0,    0,    0,    0,    0,    0,    0
           },
           {   // Knight table × 3
               -240, -280, -280, -280, -280, -280, -260, -240,
               -270, -290, -310, -315, -315, -310, -290, -270,
               -280, -310, -315, -330, -330, -315, -310, -280,
               -280, -315, -330, -335, -335, -330, -315, -280,
               -280, -315, -330, -335, -335, -330, -315, -280,
               -280, -310, -315, -330, -330, -315, -310, -280,
               -270, -290, -310, -315, -315, -310, -290, -270,
               -240, -280, -280, -280, -280, -280, -280, -240
           },
           {   // Bishop table × 3
               -260, -280, -300, -290, -290, -300, -280, -260,
               -280, -335, -300, -315, -315, -300, -335, -280,
               -300, -300, -325, -325, -325, -325, -300, -300,
               -290, -325, -325, -325, -325, -325, -325, -290,
               -315, -315, -325, -325, -325, -325, -315, -315,
               -300, -305, -325, -310, -310, -325, -305, -300,
               -280, -335, -300, -315, -315, -300, -335, -280,
               -260, -280, -300, -290, -290, -300, -280, -260
           },
           {   // Rook table × 5
               -500, -500, -500, -510, -510, -500, -500, -500,
               -515, -515, -515, -530, -530, -515, -515, -515,
               -490, -490, -490, -490, -490, -490, -490, -490,
               -490, -490, -490, -490, -490, -490, -490, -490,
               -490, -490, -490, -490, -490, -490, -490, -490,
               -490, -490, -490, -515, -515, -490, -490, -490,
               -480, -480, -490, -510, -510, -490, -480, -480,
               -480, -480, -480, -505, -505, -480, -480, -480
           },
           {   // Queen table × 9
               -900, -905, -910, -910, -910, -910, -905, -900,
               -905, -905, -910, -910, -910, -910, -905, -905,
               -900, -900, -910, -910, -910, -910, -900, -900,
               -900, -910, -910, -910, -910, -910, -910, -905,
               -905, -910, -910, -910, -910, -910, -910, -900,
               -885, -905, -915, -915, -915, -915, -905, -885,
               -870, -890, -900, -905, -905, -900, -890, -870,
               -850, -870, -890, -900, -900, -890, -870, -850     
           },
           {   // King table × 0
               0,  0,  0,  0,  0,  0,  0,  0,
               0,  0,  0,  0,  0,  0,  0,  0,
               0,  0,  0,  0,  0,  0,  0,  0,
               0,  0,  0,  0,  0,  0,  0,  0,
               0,  0,  0,  0,  0,  0,  0,  0,
               0,  0,  0,  0,  0,  0,  0,  0,
             -15,-15,-15,-15,-15,-15,-15,-15,
             -35,-40,-40,-25,-25,-25,-45,-35
           }
       }
    };
    constexpr short PST_end[2][6][64] = {
        { // White tables
            {   // Pawn table × 1
                0,   0,   0,   0,   0,   0,   0,   0,
                90,  95,  95,  95,  95,  95,  95,  90,
                95,  95,  95, 110, 110,  95,  95,  95,
               100, 100, 100, 130, 130, 100, 100, 100,
               105, 105, 120, 140, 140, 120, 105, 105,
               135, 140, 150, 160, 160, 150, 140, 135,
               180, 190, 200, 220, 220, 200, 190, 180,
                 0,   0,   0,   0,   0,   0,   0,   0
           },
           {   // Knight table × 3
               260, 290, 290, 290, 290, 290, 290, 260,
               290, 300, 305, 305, 305, 305, 300, 290,
               290, 305, 305, 310, 310, 305, 305, 290,
               290, 305, 310, 315, 315, 310, 305, 290,
               290, 305, 310, 315, 315, 310, 305, 290,
               290, 305, 305, 310, 310, 305, 305, 290,
               290, 300, 305, 305, 305, 305, 300, 290,
               260, 290, 290, 290, 290, 290, 290, 260
           },
           {   // Bishop table × 3
               270, 280, 300, 300, 300, 300, 280, 270,
               280, 325, 315, 315, 315, 315, 325, 280,
               300, 315, 325, 325, 325, 325, 315, 300,
               300, 325, 325, 325, 325, 325, 325, 300,
               300, 325, 325, 325, 325, 325, 325, 300,
               300, 315, 325, 325, 325, 325, 315, 300,
               300, 325, 315, 315, 315, 315, 325, 280,
               270, 280, 300, 300, 300, 300, 280, 270
           },
           {   // Rook table × 5
               510, 510, 510, 510, 510, 510, 510, 510,
               510, 510, 510, 510, 510, 510, 510, 510,
               510, 510, 510, 510, 510, 510, 510, 510,
               510, 510, 510, 510, 510, 510, 510, 510,
               510, 510, 510, 510, 510, 510, 510, 510,
               510, 510, 510, 510, 510, 510, 510, 510,
               520, 520, 520, 520, 520, 520, 520, 520,
               510, 510, 510, 510, 510, 510, 510, 510,
           },
           {   // Queen table × 9
               900, 900, 905, 910, 910, 905, 900, 900,
               900, 900, 910, 910, 910, 910, 900, 900,
               900, 900, 910, 910, 910, 910, 900, 900,
               900, 910, 910, 910, 910, 910, 910, 900,
               900, 910, 910, 910, 910, 910, 910, 900,
               900, 900, 910, 910, 910, 910, 900, 900,
               900, 900, 910, 910, 910, 910, 900, 900,
               900, 900, 905, 910, 910, 905, 900, 900
           },
           {   // King table × 0
               0, 0, 0, 0, 0, 0, 0, 0,
               0, 0, 0, 0, 0, 0, 0, 0,
               0, 0, 5, 5, 5, 5, 0, 0,
               0, 0, 5, 5, 5, 5, 0, 0,
               0, 0, 5, 5, 5, 5, 0, 0,
               0, 0, 5, 5, 5, 5, 0, 0,
               0, 0, 0, 0, 0, 0, 0, 0,
               0, 0, 0, 0, 0, 0, 0, 0
           }
       },
       { // Black tables
           {   // Pawn table × 1
                  0,   0,    0,    0,    0,     0,    0,    0,
               -180, -190, -200, -220, -220, -200, -190, -180,
               -135, -140, -150, -160, -160, -150, -140, -135,
               -105, -105, -120, -140, -140, -120, -105, -105,
               -100, -100, -100, -130, -130, -100, -100, -100,
                -95,  -95,  -95, -110, -110,  -95,  -95,  -95,
                -90,  -95,  -95,  -95,  -95,  -95,  -95,  -90,
                  0,    0,    0,    0,    0,    0,    0,    0
           },
           {   // Knight table × 3
               -260, -290, -290, -290, -290, -290, -290, -260,
               -290, -300, -305, -305, -305, -305, -300, -290,
               -290, -305, -305, -310, -310, -305, -305, -290,
               -290, -305, -310, -315, -315, -310, -305, -290,
               -290, -305, -310, -315, -315, -310, -305, -290,
               -290, -305, -305, -310, -310, -305, -305, -290,
               -290, -300, -305, -305, -305, -305, -300, -290,
               -260, -290, -290, -290, -290, -290, -290, -260
           },
           {   // Bishop table × 3
               -270, -280, -300, -300, -300, -300, -280, -270,
               -280, -325, -315, -315, -315, -315, -325, -280,
               -300, -315, -325, -325, -325, -325, -315, -300,
               -300, -325, -325, -325, -325, -325, -325, -300,
               -300, -325, -325, -325, -325, -325, -325, -300,
               -300, -315, -325, -325, -325, -325, -315, -300,
               -280, -325, -315, -315, -315, -315, -325, -280,
               -270, -280, -300, -300, -300, -300, -280, -270
           },
           {   // Rook table × 5
               -510, -510, -510, -510, -510, -510, -510, -510,
               -520, -520, -520, -520, -520, -520, -520, -520,
               -510, -510, -510, -510, -510, -510, -510, -510,
               -510, -510, -510, -510, -510, -510, -510, -510,
               -510, -510, -510, -510, -510, -510, -510, -510,
               -510, -510, -510, -510, -510, -510, -510, -510,
               -510, -510, -510, -510, -510, -510, -510, -510,
               -510, -510, -510, -510, -510, -510, -510, -510,
           },
           {   // Queen table × 9
               -900, -900, -905, -910, -910, -905, -900, -900,
               -900, -900, -910, -910, -910, -910, -900, -900,
               -900, -900, -910, -910, -910, -910, -900, -900,
               -900, -910, -910, -910, -910, -910, -910, -900,
               -900, -910, -910, -910, -910, -910, -910, -900,
               -900, -900, -910, -910, -910, -910, -900, -900,
               -900, -900, -910, -910, -910, -910, -900, -900,
               -900, -900, -905, -910, -910, -905, -900, -900     
           },
           {   // King table × 0
               0, 0, 0, 0, 0, 0, 0, 0,
               0, 0, 0, 0, 0, 0, 0, 0,
               0, 0, -5, -5, -5, -5, 0, 0,
               0, 0, -5, -5, -5, -5, 0, 0,
               0, 0, -5, -5, -5, -5, 0, 0,
               0, 0, -5, -5, -5, -5, 0, 0,
               0, 0, 0, 0, 0, 0, 0, 0,
               0, 0, 0, 0, 0, 0, 0, 0
           }
       }
    };

    vector<short>Pawn_structure = {
        0, -15, -15,   0, -15, -45,   0,   0, -15, -45, -45, -30,   0, -30,   0,   0,
      -15, -45, -45, -30, -45, -75, -30, -30,   0, -30, -30, -15,   0, -30,   0,   0,
      -15, -45, -45, -30, -45, -75, -30, -30, -45, -75, -75, -60, -30, -60, -30, -30, 
        0, -30, -30, -15, -30, -60, -15, -15,   0, -30, -30, -15,   0, -30,   0,   0, 
      -15, -45, -45, -30, -45, -75, -30, -30, -45, -75, -75, -60, -30, -60, -30, -30, 
      -45, -75, -75, -60, -75,-105, -60, -60, -30, -60, -60, -45, -30, -60, -30, -30, 
        0, -30, -30, -15, -30, -60, -15, -15, -30, -60, -60, -45, -15, -45, -15, -15, 
        0, -30, -30, -15, -30, -60, -15, -15,   0, -30, -30, -15,   0, -30,   0,   0, 
      -15, -45, -45, -30, -45, -75, -30, -30, -45, -75, -75, -60, -30, -60, -30, -30, 
      -45, -75, -75, -60, -75,-105, -60, -60, -30, -60, -60, -45, -30, -60, -30, -30, 
      -45, -75, -75, -60, -75,-105, -60, -60, -75,-105,-105, -90, -60, -90, -60, -60, 
      -30, -60, -60, -45, -60, -90, -45, -45, -30, -60, -60, -45, -30, -60, -30, -30, 
        0, -30, -30, -15, -30, -60, -15, -15, -30, -60, -60, -45, -15, -45, -15, -15, 
      -30, -60, -60, -45, -60, -90, -45, -45, -15, -45, -45, -30, -15, -45, -15, -15, 
        0, -30, -30, -15, -30, -60, -15, -15, -30, -60, -60, -45, -15, -45, -15, -15, 
        0, -30, -30, -15, -30, -60, -15, -15,   0, -30, -30, -15,   0, -30,   0,  0
  };
}

// Global variable to hold the current game stage for the whole search.
GameStage currentStage = GameStage::EARLY;

// Global incremental counters for extra bonuses.
int white_bishop_count = 0, black_bishop_count = 0;
std::vector<int> white_rooks_on_file(8, 0); // Number of white rooks on each file (0-7)
std::vector<int> black_rooks_on_file(8, 0); // Same for black

// Modified getPieceSquareValue selects the table according to the current stage.
short getPieceSquareValue(PieceType type, int colorId, int squareId) {
    int pieceIndex = static_cast<int>(type);
    switch (currentStage) {
        case GameStage::EARLY:
            return Evaluation::PST_early[colorId][pieceIndex][squareId];
        case GameStage::MID:
            return Evaluation::PST_mid[colorId][pieceIndex][squareId];
        case GameStage::END:
            return Evaluation::PST_end[colorId][pieceIndex][squareId];
    }
    return 0; // Should never happen.
}

// Helper: Compute the extra bonus based on the incremental counters.
int computeExtraBonusIncremental() {
    int bonus = 0;
    // Bishop pair bonus: +25 for White if at least 2 bishops; -25 for Black.
    if (white_bishop_count >= 2) bonus += 25;
    if (black_bishop_count >= 2) bonus -= 25;
    
    // Rook bonus per file.
    for (int file = 0; file < 8; ++file) {
        // For White: if there are rooks and no white pawn on the file.
        if (white_rooks_on_file[file] > 0 && white_pawn_counts[file] == 0) {
            if (black_pawn_counts[file] == 0)
                bonus += white_rooks_on_file[file] * 30; // open file
            else
                bonus += white_rooks_on_file[file] * 20; // semi–open file
        }
        // For Black: bonus is negative.
        if (black_rooks_on_file[file] > 0 && black_pawn_counts[file] == 0) {
            if (white_pawn_counts[file] == 0)
                bonus -= black_rooks_on_file[file] * 30;
            else
                bonus -= black_rooks_on_file[file] * 20;
        }
    }
    return bonus;
}

// Updated evaluateBoard: scan the board once and set the incremental counters.
short evaluateBoard(Board &board) {
    // Reset pawn counts and bit representations.
    std::fill(white_pawn_counts.begin(), white_pawn_counts.end(), 0);
    std::fill(black_pawn_counts.begin(), black_pawn_counts.end(), 0);
    white_pawns = 0;
    black_pawns = 0;
    
    // Reset extra bonus counters.
    white_bishop_count = 0;
    black_bishop_count = 0;
    std::fill(white_rooks_on_file.begin(), white_rooks_on_file.end(), 0);
    std::fill(black_rooks_on_file.begin(), black_rooks_on_file.end(), 0);
    
    short score = 0;
    short pieceCount = 0;

    // Process all pieces.
    for (int color : {0, 1}) {
        for (auto type : {PieceType::PAWN, PieceType::KNIGHT, PieceType::BISHOP,
                           PieceType::ROOK, PieceType::QUEEN, PieceType::KING}) {
            Bitboard pieces = board.pieces(type, Color(color));
            while (!pieces.empty()){
                int sq = pieces.pop();
                pieceCount++;
                score += getPieceSquareValue(type, color, sq);
                
                // For pawns, update structure.
                if (type == PieceType::PAWN) {
                    int col = sq % 8;
                    if (color == 0) {
                        white_pawn_counts[col]++;
                        white_pawns |= (1 << col);
                    } else {
                        black_pawn_counts[col]++;
                        black_pawns |= (1 << col);
                    }
                }
                // For bishops, update bishop count.
                if (type == PieceType::BISHOP) {
                    if (color == 0)
                        white_bishop_count++;
                    else
                        black_bishop_count++;
                }
                // For rooks, update per–file counters.
                if (type == PieceType::ROOK) {
                    int col = sq % 8;
                    if (color == 0)
                        white_rooks_on_file[col]++;
                    else
                        black_rooks_on_file[col]++;
                }
            }
        }
    }
    
    // Determine game stage.
    if (pieceCount <= 12) {
        currentStage = GameStage::END;
        EXPECTED_MOVES_LEFT = END_GAME_MOVES;
    } else if (pieceCount <= 29 || board.fullMoveNumber() >= 6) {
        currentStage = GameStage::MID;
        EXPECTED_MOVES_LEFT = MID_GAME_MOVES;
    } else {
        currentStage = GameStage::EARLY;
        EXPECTED_MOVES_LEFT = EARLY_GAME_MOVES;
    }
    
    // Pawn structure bonus.
    score += Evaluation::Pawn_structure[white_pawns];
    score -= Evaluation::Pawn_structure[black_pawns];
    
    // Add the incremental extra bonus.
    score += computeExtraBonusIncremental();
    
    return score;
}

// Updated evaluateMove that uses incremental updates for extra bonus.
short evaluateMove(const Board& board, const Move& move) {
    short delta = 0;
    int movingColorIdx = board.sideToMove();
    int oppColorIdx = (movingColorIdx == 0 ? 1 : 0);
    int from_ind = move.from().index();
    int to_ind = move.to().index();
    
    // Backup pawn structure variables are already done in your code.
    short before_pawn_structure = Evaluation::Pawn_structure[white_pawns] - Evaluation::Pawn_structure[black_pawns];
    
    // --- Begin: Backup our extra bonus counters.
    int backup_white_bishop_count = white_bishop_count;
    int backup_black_bishop_count = black_bishop_count;
    std::vector<int> backup_white_rooks = white_rooks_on_file;
    std::vector<int> backup_black_rooks = black_rooks_on_file;
    // --- End backup.
    
    // Compute extra bonus before the move using the incremental counters.
    int extraBefore = computeExtraBonusIncremental();
    
    // (Existing evaluation delta computation for move type goes here.)
    // For example:
    uint16_t mov_type = move.typeOf();
    if (mov_type == Move::CASTLING) {
        delta -= getPieceSquareValue(PieceType::KING, movingColorIdx, from_ind);
        delta += getPieceSquareValue(PieceType::KING, movingColorIdx, to_ind);
        int aux = (from_ind < to_ind ? -1 : 1);
        int rook_initial = (aux == -1 ? (movingColorIdx == 0 ? 7 : 63) : (movingColorIdx == 0 ? 0 : 56));
        delta -= getPieceSquareValue(PieceType::KING, movingColorIdx, rook_initial);
        delta += getPieceSquareValue(PieceType::KING, movingColorIdx, to_ind + aux);
        if (currentStage != GameStage::END)
            delta += 20;
        // Update rook counters for castling.
        int rook_from_col = rook_initial % 8;
        int rook_to_col = (to_ind + aux) % 8;
        if (movingColorIdx == 0) {
            white_rooks_on_file[rook_from_col]--;
            white_rooks_on_file[rook_to_col]++;
        } else {
            black_rooks_on_file[rook_from_col]--;
            black_rooks_on_file[rook_to_col]++;
        }
    }
    else if (mov_type == Move::ENPASSANT) {
        int capturedSq_ind = (movingColorIdx == 0 ? to_ind - 8 : to_ind + 8);
        int from_col = from_ind % 8;
        int to_col = to_ind % 8;
        if (movingColorIdx == 0) {
            white_pawn_counts[from_col]--;
            white_pawn_counts[to_col]++;
            black_pawn_counts[to_col]--;
            if (white_pawn_counts[from_col] == 0) white_pawns &= ~(1 << from_col);
            white_pawns |= (1 << to_col);
            if (black_pawn_counts[to_col] == 0) black_pawns &= ~(1 << to_col);
        } else {
            black_pawn_counts[from_col]--;
            black_pawn_counts[to_col]++;
            white_pawn_counts[to_col]--;
            if (black_pawn_counts[from_col] == 0) black_pawns &= ~(1 << from_col);
            black_pawns |= (1 << to_col);
            if (white_pawn_counts[to_col] == 0) white_pawns &= ~(1 << to_col);
        }
        delta -= getPieceSquareValue(PieceType::PAWN, movingColorIdx, from_ind);
        delta -= getPieceSquareValue(PieceType::PAWN, oppColorIdx, capturedSq_ind);
        delta += getPieceSquareValue(PieceType::PAWN, movingColorIdx, to_ind);
    }
    else if (mov_type == Move::PROMOTION) {
        int from_col = from_ind % 8;
        PieceType capture_type = board.at(Square(to_ind)).type();
        if (movingColorIdx == 0) {
            white_pawn_counts[from_col]--;
            if (white_pawn_counts[from_col] == 0) white_pawns &= ~(1 << from_col);
        } else {
            black_pawn_counts[from_col]--;
            if (black_pawn_counts[from_col] == 0) black_pawns &= ~(1 << from_col);
        }
        delta -= getPieceSquareValue(PieceType::PAWN, movingColorIdx, from_ind);
        delta += getPieceSquareValue(move.promotionType(), movingColorIdx, to_ind);
        if (capture_type != PieceType::NONE)
            delta -= getPieceSquareValue(capture_type, oppColorIdx, to_ind);
        // Update incremental counters for promotion.
        if (movingColorIdx == 0) {
            // A pawn promoted: if promotion is to a rook or bishop, update accordingly.
            if (move.promotionType() == PieceType::ROOK) {
                white_rooks_on_file[to_ind % 8]++;
            } else if (move.promotionType() == PieceType::BISHOP) {
                white_bishop_count++;
            }
        } else {
            if (move.promotionType() == PieceType::ROOK) {
                black_rooks_on_file[to_ind % 8]++;
            } else if (move.promotionType() == PieceType::BISHOP) {
                black_bishop_count++;
            }
        }
    }
    else { // Normal move.
        PieceType pieceType = board.at(Square(from_ind)).type();
        PieceType capture_type = board.at(Square(to_ind)).type();
        int from_col = from_ind % 8;
        int to_col = to_ind % 8;
        
        if (pieceType == PieceType::PAWN) {
            if (movingColorIdx == 0) {
                white_pawn_counts[from_col]--;
                white_pawn_counts[to_col]++;
                if (white_pawn_counts[from_col] == 0) white_pawns &= ~(1 << from_col);
                white_pawns |= (1 << to_col);
            } else {
                black_pawn_counts[from_col]--;
                black_pawn_counts[to_col]++;
                if (black_pawn_counts[from_col] == 0) black_pawns &= ~(1 << from_col);
                black_pawns |= (1 << to_col);
            }
        }
        if (capture_type == PieceType::PAWN) {
            if (movingColorIdx == 0) {
                black_pawn_counts[to_col]--;
                if (black_pawn_counts[to_col] == 0) black_pawns &= ~(1 << to_col);
            } else {
                white_pawn_counts[to_col]--;
                if (white_pawn_counts[to_col] == 0) white_pawns &= ~(1 << to_col);
            }
        }
        delta -= getPieceSquareValue(pieceType, movingColorIdx, from_ind);
        delta += getPieceSquareValue(pieceType, movingColorIdx, to_ind);
        if (capture_type != PieceType::NONE)
            delta -= getPieceSquareValue(capture_type, oppColorIdx, to_ind);
        
        // Incremental updates for moving pieces:
        if (pieceType == PieceType::ROOK) {
            // Update rook file counters.
            if (movingColorIdx == 0) {
                white_rooks_on_file[from_col]--;
                white_rooks_on_file[to_col]++;
            } else {
                black_rooks_on_file[from_col]--;
                black_rooks_on_file[to_col]++;
            }
        }
        if (pieceType == PieceType::BISHOP) {
            // For a bishop move, the count remains unchanged.
            // However, if capturing an opponent bishop, update that side’s count.
            if (capture_type == PieceType::BISHOP) {
                if (movingColorIdx == 0)
                    black_bishop_count--;
                else
                    white_bishop_count--;
            }
        }
    }
    
    // Compute extra bonus after updating our incremental counters.
    int extraAfter = computeExtraBonusIncremental();
    // Add the difference to the delta.
    delta += (extraAfter - extraBefore);
    
    // Also adjust for pawn structure delta.
    short after_pawn_structure = Evaluation::Pawn_structure[white_pawns] - Evaluation::Pawn_structure[black_pawns];
    delta += (after_pawn_structure - before_pawn_structure);
    
    // Restore our extra bonus incremental counters.
    white_bishop_count = backup_white_bishop_count;
    black_bishop_count = backup_black_bishop_count;
    white_rooks_on_file = backup_white_rooks;
    black_rooks_on_file = backup_black_rooks;
    
    return delta;
}


short black(Board &board, short depth, short alpha, short beta, Move &bestMove, short currentEval, vector<uint8_t> &positionCounts, short min_depth,  short max_depth);
short white(Board &board, short depth, short alpha, short beta, Move &bestMove, short currentEval, vector<uint8_t> &positionCounts, short min_depth,  short max_depth) {
    #ifdef DEBUG
    ++nodesAnalyzed; // Debug: count node analyzed for white search
    #endif

    // Terminal condition 1: 50 moves rule
    if (board.isHalfMoveDraw()){
        GameResult result = board.getHalfMoveDrawType().second;
        if (result == GameResult::LOSE) return -INFINITY_VAL;
        else return 0;
    }

    // Terminal condition 2 : triple repetition
    uint64_t zobrist_w = board.zobrist() & (HASH_TABLE_SIZE - 1);
    uint8_t rep = positionCounts[zobrist_w];
    if (rep == 2) return 0;

    Movelist unordered_moves;
    movegen::legalmoves(unordered_moves, board);

    // Terminal condition 3: no moves allowed (received checkmate or stalemate)
    if (unordered_moves.empty()){
        GameResult result = board.isGameOver().second;
        if (result == GameResult::LOSE) return -INFINITY_VAL;
        else return 0; 
    }

    bestMove = unordered_moves[0];

    // Terminal condition 4: max depth reached
    if (depth == max_depth) return currentEval+10;

    bool in_check = board.inCheck();

    // Null move pruning (inactive in endgames)
    if (currentStage != GameStage::END && currentEval - 100 > beta && !in_check && min_depth - depth > 2) {   
        board.makeNullMove();
        Movelist moves_aux;                                               
        movegen::legalmoves(moves_aux, board);
        if (!moves_aux.empty()){
            short new_min_depth = min_depth - depth - 2;
            short new_max_depth = min_depth - depth;
            short null_move_score = black(board, 0, beta-1, beta, bestMove, currentEval, positionCounts, new_min_depth, new_max_depth); // Give turn away, small window for efficiency
            if (null_move_score >= beta){
                board.unmakeNullMove();
                return null_move_score;                                                                                                 // Return null_move score
            }
        }
        bestMove = unordered_moves[0];
        board.unmakeNullMove();
    }

    // Move ordering
    Movelist queen_promotion;
    Movelist check_and_capture;
    Movelist check;
    Movelist good_capture;
    Movelist bad_capture;
    Movelist quiet;

    for (const auto& move : unordered_moves){ 
        uint16_t mov_type = move.typeOf();
        board.makeMove(move);
        bool is_check = board.inCheck();
        board.unmakeMove(move);
    
        bool is_capture = board.isCapture(move) && mov_type != Move::CASTLING;
        bool is_queen_promotion = (mov_type == Move::PROMOTION && move.promotionType() == PieceType::QUEEN);
    
        if (is_queen_promotion) queen_promotion.add(move);
        else if (is_check && is_capture) check_and_capture.add(move);
        else if (is_capture) {
            if (mov_type != Move::ENPASSANT && board.at(Square(move.to())).type() > board.at(Square(move.from())).type()){
                good_capture.add(move);
            }
            else bad_capture.add(move);
        }
        else if (is_check) check.add(move);
        else quiet.add(move);
    }

    Movelist moves;
    for (const auto& m : queen_promotion) moves.add(m);
    for (const auto& m : check_and_capture) moves.add(m);
    for (const auto& m : good_capture) moves.add(m);
    if (depth < min_depth + 3){
        for (const auto& m : check) moves.add(m);
    }
    for (const auto& m : bad_capture) moves.add(m);

    short best = -INFINITY_VAL;
    
    if (depth < min_depth || in_check) {
        for (const auto& m : quiet) moves.add(m); // If in check or min_depth not reached, analyze all movements
    }
    else { // Not in check, min_depth reached 
        if (currentEval-10 >= beta) return currentEval-10;
        else best = currentEval-10; // Standing pat
    }

    Move dummy;
    for (auto &move : moves) {
        short evalDelta = 0;

        // Backup pawn structure
        unsigned char backup_white_pawns = white_pawns;
        unsigned char backup_black_pawns = black_pawns;
        vector<short> backup_white_pawn_counts = white_pawn_counts;
        vector<short> backup_black_pawn_counts = black_pawn_counts;

        // Calculate the change in evaluation caused by the move
        evalDelta = evaluateMove(board, move);
        currentEval += evalDelta;

        board.makeMove(move);
        positionCounts[zobrist_w] += 1;
        short score = black(board, depth + 1, alpha, beta, dummy, currentEval, positionCounts, min_depth, max_depth);
        positionCounts[zobrist_w] -= 1;
        board.unmakeMove(move);

        // Restore pawn structure
        white_pawns = backup_white_pawns;
        black_pawns = backup_black_pawns;
        white_pawn_counts = backup_white_pawn_counts;
        black_pawn_counts = backup_black_pawn_counts;

        // Restore the evaluation to its previous state
        currentEval -= evalDelta;

        if (score > best) {
            best = score;
            bestMove = move;
        }
        alpha = max(alpha, score);
        if (alpha >= beta) {
            break;
        }
    }
    return best;
}


short black(Board &board, short depth, short alpha, short beta, Move &bestMove, short currentEval, vector<uint8_t> &positionCounts, short min_depth,  short max_depth){
    #ifdef DEBUG
        ++nodesAnalyzed; // Debug: count node analyzed for black search
    #endif
    
    // Terminal condition 1: 50 moves rule
    if (board.isHalfMoveDraw()){
        GameResult result = board.getHalfMoveDrawType().second;
        if (result == GameResult::LOSE) return INFINITY_VAL;
        else return 0;
    }

    // Terminal condition 2 : triple repetition
    uint64_t zobrist_w = board.zobrist() & (HASH_TABLE_SIZE - 1);
    uint8_t rep = positionCounts[zobrist_w];
    if (rep == 2) return 0;

    Movelist unordered_moves;
    movegen::legalmoves(unordered_moves, board);

    // Terminal condition 3: no moves allowed (received checkmate or stalemate)
    if (unordered_moves.empty()){
        GameResult result = board.isGameOver().second;
        if (result == GameResult::LOSE) return INFINITY_VAL;
        else return 0; 
    }

    bestMove = unordered_moves[0];

    if (depth == max_depth) return currentEval - 10; //Max depth

    bool in_check = board.inCheck();

    // Null move pruning (inactive in endgames)
    if (currentStage != GameStage::END && currentEval + 100 < alpha && !in_check && min_depth - depth > 2) {
        board.makeNullMove();
        Movelist moves_aux;                                               
        movegen::legalmoves(moves_aux, board);
        if (!moves_aux.empty()){
            short new_min_depth = min_depth - depth - 2;
            short new_max_depth = min_depth - depth;
            short null_move_score = white(board, 0, alpha, alpha+1, bestMove, currentEval, positionCounts, new_min_depth, new_max_depth); // Give turn away, small window for efficiency
            if (null_move_score <= alpha){
                board.unmakeNullMove();
                return null_move_score;                                                                       // Return null_move score
            }
        }
        bestMove = unordered_moves[0];
        board.unmakeNullMove();
    }

    // Move ordering
    Movelist queen_promotion;
    Movelist check_and_capture;
    Movelist check;
    Movelist good_capture;
    Movelist bad_capture;
    Movelist quiet;

    for (const auto& move : unordered_moves){ 
        uint16_t mov_type = move.typeOf();
        board.makeMove(move);
        bool is_check = board.inCheck();
        board.unmakeMove(move);
    
        bool is_capture = board.isCapture(move) && mov_type != Move::CASTLING;
        bool is_queen_promotion = (mov_type == Move::PROMOTION && move.promotionType() == PieceType::QUEEN);
    
        if (is_queen_promotion) queen_promotion.add(move);
        else if (is_check && is_capture) check_and_capture.add(move);
        else if (is_capture) {
            if (mov_type != Move::ENPASSANT && board.at(Square(move.to())).type() > board.at(Square(move.from())).type()){
                good_capture.add(move);
            }
            else bad_capture.add(move);
        }
        else if (is_check) check.add(move);
        else quiet.add(move);
    }

    Movelist moves;
    for (const auto& m : queen_promotion) moves.add(m);
    for (const auto& m : check_and_capture) moves.add(m);
    for (const auto& m : good_capture) moves.add(m);
    if (depth < min_depth + 3) for (const auto& m : check) moves.add(m);
    for (const auto& m : bad_capture) moves.add(m);

    short best = +INFINITY_VAL;
    
    if (depth < min_depth || in_check) {
        for (const auto& m : quiet) moves.add(m); // If in check or min_depth not reached, analyze all movements
    }
    else { // Not in check, min_depth reached 
        if (currentEval+10 <= alpha) return currentEval+10;
        else best = currentEval+10; // Standing pat
    }

    Move dummy;
    for (auto &move : moves) {
        short evalDelta = 0;

        // Backup pawn structure
        unsigned char backup_white_pawns = white_pawns;
        unsigned char backup_black_pawns = black_pawns;
        vector<short> backup_white_pawn_counts = white_pawn_counts;
        vector<short> backup_black_pawn_counts = black_pawn_counts;

        // Calculate the change in evaluation caused by the move
        evalDelta = evaluateMove(board, move);
        currentEval += evalDelta;

        board.makeMove(move);
        positionCounts[zobrist_w] += 1;
        short score = white(board, depth + 1, alpha, beta, dummy, currentEval, positionCounts, min_depth, max_depth);
        positionCounts[zobrist_w] -= 1;
        board.unmakeMove(move);

        // Restore pawn structure
        white_pawns = backup_white_pawns;
        black_pawns = backup_black_pawns;
        white_pawn_counts = backup_white_pawn_counts;
        black_pawn_counts = backup_black_pawn_counts;

        // Restore the evaluation to its previous state
        currentEval -= evalDelta;

        if (score < best) {
            best = score;
            bestMove = move;
        }
        beta = min(beta, score);
        if (alpha >= beta) {
            break;
        }
    }
    return best;
}


struct SearchParameters {
    int wtime = 0;
    int btime = 0;
    int winc = 0;
    int binc = 0;
    int depth = 0;
    int nodes = 0;
    int mate = 0;
    int movetime = 0;
    bool infinite = false;
    vector<string> searchmoves;
    bool ponder = false;
};

//-------------------------------------------------------------
// Opening Database Structures and Functions
//-------------------------------------------------------------

// Structure to represent a node in the openings database
struct OpeningNode {
    int id;
    std::string fen;
    int evaluation;
    std::string moves;  // Space-separated moves with evaluations
    std::vector<std::string> pv;  // PV moves stored as a space-delimited string in the database
};

// Helper: tokenize a space-delimited string into a vector of strings.
std::vector<std::string> tokenize(const std::string& str) {
    std::istringstream iss(str);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

// Query the opening database for a specific FEN using the new schema.
OpeningNode queryOpeningDatabase(sqlite3* db, const std::string& fen) {
    OpeningNode node;
    sqlite3_stmt* stmt;
    
    // New query: we now select id, fen, evaluation, moves, and pv from the nodes table.
    const char* node_sql = 
        "SELECT id, fen, evaluation, moves, pv "
        "FROM nodes "
        "WHERE fen = ?;";
    
    if (sqlite3_prepare_v2(db, node_sql, -1, &stmt, nullptr) != SQLITE_OK) {
        DEBUG_PRINT("[DEBUG] Failed to prepare node SQL statement: " << sqlite3_errmsg(db));
        return node;
    }
    
    sqlite3_bind_text(stmt, 1, fen.c_str(), -1, SQLITE_STATIC);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        node.id = sqlite3_column_int(stmt, 0);
        node.fen = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        node.evaluation = sqlite3_column_int(stmt, 2);
        node.moves = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        std::string pvStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        node.pv = tokenize(pvStr);
    }
    
    sqlite3_finalize(stmt);
    return node;
}

// Given a position, look up the corresponding node in the openings database
const OpeningNode* lookupOpening(const std::vector<OpeningNode>& nodes, const std::string& fen) {
    for (const auto& node : nodes) {
        if (node.fen == fen) {
            return &node;
        }
    }
    return nullptr;
}

//-------------------------------------------------------------
// UCIHandler: Modified to use the openings database.
//-------------------------------------------------------------
class UCIHandler {
private:
    Board board;
    atomic<bool> searching{false};
    thread search_thread;
    mutex board_mutex;
    
    // Add random number generator as a class member
    std::mt19937 rng;
    
    // Search parameters structure (same as before)
    struct SearchParameters {
        int wtime = 0;
        int btime = 0;
        int winc = 0;
        int binc = 0;
        int movetime = 0;
        bool ponder = false;
    } search_params;
    
    vector<uint8_t> positionCounts = vector<uint8_t>(HASH_TABLE_SIZE, 0);
    bool uciChess960 = false;
    
    // Engine options
    unordered_map<string, string> options;
    
    // --- Modified members for the openings database ---
    sqlite3* opening_db;
    vector<string> playedMoves;     // Move history in UCI notation.
    vector<string> openingPV;       // If a leaf is reached in the book, store the rest of the PV.
    bool hitLeaf = false;           // Track if we've hit a leaf in the book
    Board previous_board;
    
public:
    UCIHandler() {
        // Initialize with a random seed at construction time
        unsigned int seed = std::random_device()();
        rng.seed(seed);
        DEBUG_PRINT("[DEBUG] Random seed initialized to: " << seed);
    }
    
    // Destructor: join any running search thread.
    ~UCIHandler() {
        if (search_thread.joinable())
            search_thread.join();
        if (opening_db) {
            sqlite3_close(opening_db);
        }
    }
    
    void run() {
        string line;
        while (getline(cin, line))
            process_command(line);
    }
    
private:
    void process_command(const string& input) {
        istringstream iss(input);
        string token;
        iss >> token;
        
        if (token == "uci")
            handle_uci();
        else if (token == "isready")
            handle_isready();
        else if (token == "setoption")
            handle_setoption(iss);
        else if (token == "ucinewgame")
            handle_ucinewgame();
        else if (token == "position")
            handle_position(iss);
        else if (token == "go")
            handle_go(iss);
        else if (token == "stop")
            handle_stop();
        else if (token == "quit")
            handle_quit();
    }
    
    void handle_uci() {
        cout << "id name Chessape_1.2" << endl;
        cout << "id author Bernabé Iturralde Jara" << endl;
        cout << "option name UCI_Chess960 type check default false" << endl;
        cout << "option name RandomSeed type spin default 0 min 0 max 2147483647" << endl;
        cout << "uciok" << endl;
    }
    
    void handle_setoption(istringstream& iss) {
        string word, name, value;
        while (iss >> word) {
            if (word == "name")
                iss >> name;
            else if (word == "value")
                iss >> value;
        }
        if (name == "UCI_Chess960") {
            uciChess960 = (value == "true" || value == "1");
            DEBUG_PRINT("[DEBUG] UCI_Chess960 set to " << (uciChess960 ? "true" : "false"));
        }
        else if (name == "RandomSeed") {
            options["RandomSeed"] = value;
            DEBUG_PRINT("[DEBUG] Random seed set to " << value);
        }
    }
    
    void handle_isready() {
        try {
            int rc = sqlite3_open("../../Openings/openings.db", &opening_db);
            if (rc) {
                DEBUG_PRINT("[DEBUG] Can't open openings database: " << sqlite3_errmsg(opening_db));
            } else {
                DEBUG_PRINT("[DEBUG] Opening database opened successfully");
            }
        } catch (const std::exception& e) {
            DEBUG_PRINT("[DEBUG] Error opening database: " << e.what());
        }
        if (search_thread.joinable())
            search_thread.join();
        cout << "readyok" << endl;
    }
    
    void handle_ucinewgame() {
        lock_guard<mutex> lock(board_mutex);
        board = Board(); // Reset board.
        positionCounts = vector<uint8_t>(HASH_TABLE_SIZE, 0);
        playedMoves.clear();   // Reset move history.
        openingPV.clear();
        hitLeaf = false;      // Reset leaf flag
        
        // Generate a new random seed for this game
        unsigned int seed = std::random_device()();
        rng.seed(seed);
        DEBUG_PRINT("[DEBUG] New random seed initialized for this game: " << seed);
    }
    
    void handle_position(istringstream& iss) {
        lock_guard<mutex> lock(board_mutex);
        string token;
        iss >> token;
        
        if (token == "startpos") {
            board = Board();
            playedMoves.clear();
            iss >> token; // Consume "moves" if present.
        } else if (token == "fen") {
            string fen;
            while (iss >> token && token != "moves")
                fen += token + " ";
            board = Board(fen);
            playedMoves.clear();
        }
        
        // Process moves.
        while (iss >> token) {
            DEBUG_PRINT("[DEBUG] Applying move: " + token);
            playedMoves.push_back(token); // Record move in UCI.
            Move move = uci::uciToMove(board, token);
            board.makeMove(move);
        }
        DEBUG_PRINT("[DEBUG] Final FEN: " + board.getFen());
        DEBUG_PRINT("[DEBUG] sideToMove: " + string((board.sideToMove() == Color::WHITE) ? "WHITE" : "BLACK"));
    }
    
    void handle_go(istringstream& iss) {
        if (search_thread.joinable())
            search_thread.join();
        
        string token;
        while (iss >> token) {
            if (token == "wtime")
                iss >> search_params.wtime;
            else if (token == "btime")
                iss >> search_params.btime;
            else if (token == "winc")
                iss >> search_params.winc;
            else if (token == "binc")
                iss >> search_params.binc;
            else if (token == "movetime")
                iss >> search_params.movetime;
            else if (token == "ponder")
                search_params.ponder = true;
        }
        
        if (searching)
            return;
        
        searching = true;
        search_thread = thread(&UCIHandler::start_search, this);
    }
    
    void start_search() {
        lock_guard<mutex> lock(board_mutex);
        short bestEval; 
        uint64_t zobrist = board.zobrist() & (HASH_TABLE_SIZE - 1);

        // Check opening book first (only if not hit a leaf)
        if (!hitLeaf) {
            sqlite3* current_db = opening_db;
            OpeningNode dbNode = queryOpeningDatabase(current_db, board.getFen());
            
            if (!dbNode.fen.empty()) {
                string chosenMove;
                if (!dbNode.moves.empty()) {
                    // Split moves string into vector of move:evaluation pairs
                    std::vector<std::pair<std::string, int>> available_moves;
                    std::istringstream iss(dbNode.moves);
                    std::string moveEval;
                    while (iss >> moveEval) {
                        size_t colonPos = moveEval.find(':');
                        if (colonPos != std::string::npos) {
                            std::string move = moveEval.substr(0, colonPos);
                            int eval = std::stoi(moveEval.substr(colonPos + 1));
                            available_moves.push_back({move, eval});
                        }
                    }
                    
                    // Print all available moves
                    DEBUG_PRINT("[DEBUG] Opening database found a branch. Available moves:");
                    for (const auto& [move, eval] : available_moves) {
                        DEBUG_PRINT("[DEBUG]   " << move << ":" << eval);
                    }
                    
                    // Choose move based on probabilities using modern random number generator
                    std::uniform_real_distribution<double> dist(0.0, 1.0);
                    double r = dist(rng);
                    bestEval = available_moves[0].second;

                    // Calculate probabilities based on distance from best evaluation
                    std::vector<double> probabilities;
                    double totalProb = 0;
                    for (const auto& [_, eval] : available_moves) {
                        double diff = std::abs(eval - bestEval);
                        double prob = pow((2.0 - RANDOM_COEFF),-diff);
                        probabilities.push_back(prob);
                        totalProb += prob;
                    }
                    
                    // Normalize probabilities
                    for (double& prob : probabilities) {
                        prob /= totalProb;
                    }
                    
                    // Print probabilities for each move
                    DEBUG_PRINT("[DEBUG] Move probabilities:");
                    for (size_t i = 0; i < available_moves.size(); ++i) {
                        DEBUG_PRINT("[DEBUG]   " << available_moves[i].first << ": " 
                                    << std::fixed << std::setprecision(1) 
                                    << (probabilities[i] * 100.0) << "%");
                    }
                    
                    // Choose move based on probabilities
                    double cumsum = 0;
                    for (size_t i = 0; i < probabilities.size(); ++i) {
                        cumsum += probabilities[i];
                        if (r <= cumsum) {
                            chosenMove = available_moves[i].first;
                            break;
                        }
                    }
                    
                    DEBUG_PRINT("[DEBUG] Chose move: " << chosenMove);
                    board.makeMove(uci::uciToMove(board, chosenMove));

                    // Check for PV in new position after applying the move
                    dbNode = queryOpeningDatabase(current_db, board.getFen());
                    if (!dbNode.pv.empty()){
                        openingPV.assign(dbNode.pv.begin(), dbNode.pv.end());
                        DEBUG_PRINT("[DEBUG] Stored PV sequence:");
                        for (const auto& move : openingPV) {
                            DEBUG_PRINT("[DEBUG]   " << move);
                        }
                    }
                }
                else if (!dbNode.pv.empty()) {
                    chosenMove = dbNode.pv[0];
                    openingPV.assign(dbNode.pv.begin() + 1, dbNode.pv.end());
                    hitLeaf = true;
                    DEBUG_PRINT("[DEBUG] Opening database hit a leaf. Returning move: " 
                                << chosenMove << " with score " << dbNode.evaluation);
                    DEBUG_PRINT("[DEBUG] Stored PV sequence:");
                    for (const auto& move : openingPV) {
                        DEBUG_PRINT("[DEBUG]   " << move);
                    }
                    board.makeMove(uci::uciToMove(board, chosenMove));
                }
                
                if (!chosenMove.empty()) {
                    std::cout << "bestmove " << chosenMove << endl;
                    searching = false;
                    positionCounts[zobrist] += 1;
                    uint64_t new_zobrist = board.zobrist() & (HASH_TABLE_SIZE - 1);
                    positionCounts[new_zobrist] += 1;
                    previous_board = board;
                    return;
                }
            }
            else{
                hitLeaf = true;
            }
        }
        
        DEBUG_PRINT("[DEBUG] Starting search on FEN: " + board.getFen());
        // If no book move found or we've hit a leaf, continue with normal search
        auto go_beg = Clock::now();
            
        // Get initial time values from current search parameters
        int my_time = (board.sideToMove() == Color::WHITE) ? search_params.wtime : search_params.btime;
        int my_inc = (board.sideToMove() == Color::WHITE) ? search_params.winc : search_params.binc;
            
        // Calculate allocated time for this move
        int moveTime = my_inc + (my_time / EXPECTED_MOVES_LEFT);
        
        DEBUG_PRINT("[DEBUG] Initial time: " + to_string(my_time) + "ms");
        DEBUG_PRINT("[DEBUG] Estimated move time: " + to_string(moveTime) + "ms");
        
        Move bestMove;
        short currentEval = evaluateBoard(board);
        short score;
        
        int currentMinDepth = FIRST_MIN_DEPTH;
        int currentMaxDepth = FIRST_MAX_DEPTH;
        long elapsed = 0;
        
        do {
            #ifdef DEBUG
            nodesAnalyzed = 0;
            auto searchStart = Clock::now();
            #endif
        
            if (board.sideToMove() == Color::WHITE) {
                score = white(board, 0, -INFINITY_VAL, INFINITY_VAL, bestMove,
                                currentEval, positionCounts, currentMinDepth, currentMaxDepth);
            } else {
                score = black(board, 0, -INFINITY_VAL, INFINITY_VAL, bestMove,
                                currentEval, positionCounts, currentMinDepth, currentMaxDepth);
            }
        
            #ifdef DEBUG
            auto searchEnd = Clock::now();
            elapsed = chrono::duration_cast<chrono::milliseconds>(searchEnd - searchStart).count();
            DEBUG_PRINT("[DEBUG] Depth [" + to_string(currentMinDepth) + ", " + 
                            to_string(currentMaxDepth) + "] - Nodes analyzed: " + to_string(nodesAnalyzed));
            DEBUG_PRINT("[DEBUG] Time of execution: " + to_string(elapsed) + "ms");
            if (elapsed > 0) DEBUG_PRINT("[DEBUG] Speed: " + to_string(nodesAnalyzed / elapsed) + "knps");
            DEBUG_PRINT("[DEBUG] Move: " + uci::moveToUci(bestMove));
            DEBUG_PRINT("[DEBUG] Score: " + to_string(score));
            #endif
        
            // Calculate remaining time
            auto now = Clock::now();
            long total_elapsed = chrono::duration_cast<chrono::milliseconds>(now - go_beg).count();
            int remaining_time = my_time - total_elapsed;
        
            DEBUG_PRINT("[DEBUG] Total elapsed: " + to_string(total_elapsed) + "ms");
            DEBUG_PRINT("[DEBUG] Remaining time: " + to_string(remaining_time) + "ms");
        
            bool terminalScore = false;
            if (score == INFINITY_VAL || score == -INFINITY_VAL) {
                terminalScore = true;
             }
        
            // Time management logic
            if (!terminalScore) {
                if (total_elapsed < moveTime * MOVETIME_MINIMUM) {
                    currentMinDepth += INCR_MIN_DEPTH;
                    currentMaxDepth += INCR_MAX_DEPTH;
                    DEBUG_PRINT("[DEBUG] Increasing depth to MIN_DEPTH=" +
                                  to_string(currentMinDepth) + ", MAX_DEPTH=" + to_string(currentMaxDepth));
                }
                else break;
            } 
            else break;
        } while (true);
        
        auto go_end = Clock::now();
        long total_elapsed = chrono::duration_cast<chrono::milliseconds>(go_end - go_beg).count();
            
        // Update position count for threefold repetition
        positionCounts[zobrist] += 1;
        
        board.makeMove(bestMove);
        uint64_t new_zobrist = board.zobrist() & (HASH_TABLE_SIZE - 1);
        positionCounts[new_zobrist] += 1;
        
        searching = false;
            
        // Send final best move
        cout << "bestmove " << uci::moveToUci(bestMove) << endl;
    }
    
    void send_info(const string& message) {
        cout << "info " << message << endl;
    }
    
    void handle_stop() {
        if (searching) {
            searching = false;
            if (search_thread.joinable())
                search_thread.join();
        }
    }
    
    void handle_quit() {
        handle_stop();
        exit(0);
    }
};

int main() {
    loadConfig();  // Load configuration at startup
    UCIHandler handler;
    handler.run();
    return 0;
}
