#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <algorithm>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include "movelib.h"


// Afterstates for Temporal Difference Learning.
static std::vector<std::vector<double> > learned_afterstates;
// Learning rate.
static const double ALPHA = 0.0025;
// n-tuples, it's a vector containing vectors, each representing 
// one n-tuple. Fields (tiles) are ordered from 0 to 15.
static std::vector<std::vector<int> > ntuples;
// How many training games.
static int num_iterations;
// Number of tuples, use this instead of ntuples.size() for performance.
static int num_ntuples;
// Sizes of tuples, stored for faster performance.
static std::vector<int> ntuples_size;
// Keeps track of scores during training
static std::vector<int> performance;

// Reads information from config.txt and values.txt, 
// if provided. If value.txt isn't provided, it 
// initializes learned_afterstates to all zero. 
void init_config() {
    std::ifstream inFile;
    inFile.open("../config.txt");
    if (!inFile) {
        std::cerr << "No configuration file!" << std::endl;
        exit(1);
    }
    std::string line;
    std::getline(inFile, line);
    inFile >> num_iterations;
    std::getline(inFile, line);
    std::getline(inFile, line);
    int num;
    while (!inFile.eof()) {
        std::getline(inFile, line);
        std::vector<int> temp;
        std::istringstream iss(line);
        while(iss >> num) {
            if (num < 16 && num >= 0) temp.push_back(num);
        }
        ntuples.push_back(temp);
        ntuples_size.push_back(temp.size());
    }
    inFile.close();

    inFile.open("../data/invalues.txt");
    if (!inFile) {
        for (int i = 0; i < ntuples.size(); i++) {
            std::vector<double> temp;
            for (int j = 0; j < (1 << (4 * ntuples[i].size())); j++) {
                temp.push_back(0);
            }
            learned_afterstates.push_back(temp);
        }
    }
    else {
        while (!inFile.eof()) {
            std::getline(inFile, line);
            std::istringstream iss(line);
            std::vector<double> temp;
            double val;
            while(iss >> val) {
                temp.push_back(val);
            }
            learned_afterstates.push_back(temp);
        }
        inFile.close();
    }
}

int get_tuple (int tuple, uint64_t board) {
    if (tuple < 0 || tuple >= num_ntuples) return -1;
    uint64_t mask = 0x0000000000000000ULL;
    int ret = 0;
    for (int i = 0; i < ntuples_size[tuple]; i++) {
        mask |= (0x000000000000000FULL << (4 * ntuples[tuple][i]));
        ret += ((mask & board) >> (4 * ntuples[tuple][i])) << (4 * i);
    }
    return ret;
}

// Value of the board is a sum of n-tuple values.
double board_value (uint64_t board) {
    double soln = 0;
	for (int i = 0; i < num_ntuples; i++) {
        soln += learned_afterstates[i][get_tuple(i,board)];
    }
    return soln;
}

int get_greedy_move (uint64_t board) {
  int move = -1;
  double maxvalue = -100000000.0f;
  for (int i = 0; i < 4; i++) {
    if (board != execute_move(i, board)) {
    	double reward = score_board(execute_move(i,board)) - score_board(board);
      if (reward + board_value(execute_move(i,board)) > maxvalue) {
        maxvalue = reward + board_value(execute_move(i,board));
        move = i;
      }
      else if (reward + board_value(execute_move(i,board)) == maxvalue) {
        int random = rand()%2;
        if (random == 0) {
          move = i;
        }
      }
    }
  }
  return move;
}

void learn_eval (uint64_t prevboard, uint64_t newboard, int penalty) {
	double reward = score_board(newboard) - score_board(prevboard) - penalty;
	for (int i = 0; i < num_ntuples; i++) {
		// double prevb = learned_afterstates[i][get_tuple(i,prevboard)];
		// double newb = learned_afterstates[i][get_tuple(i,newboard)];
		double prevb = board_value(prevboard);
		double newb = board_value(newboard);
		learned_afterstates[i][get_tuple(i,prevboard)] += ALPHA * (reward/((double)num_ntuples) + newb - prevb);
		// printf("Tuple: %d, Number of the tuple: %d, Afterstate of prevboard: %.4f \n",i,get_tuple(i,prevboard),learned_afterstates[i][get_tuple(i,prevboard)] );
	}
}

void play_game() {
    uint64_t board = initial_board();
    int moveno = 0;
    int reward = 0;
    uint64_t prevboard = 0x0000000000000000ULL;
    int temp_penalty = 0;
    int scorepenalty = 0; // "penalty" for obtaining free 4 tiles

    while(1) {
        int move;
        uint64_t newboard;

        for(move = 0; move < 4; move++) {
            if(execute_move(move, board) != board)
                break;
        }
        if(move == 4)
            break; // no legal moves

        move = get_greedy_move(board);
        if(move < 0)
            break;

        newboard = execute_move(move, board);
        if(newboard == board) {
            printf("Illegal move!\n");
            moveno--;
            continue;
        }
        // prevboard is afterstate from previous loop, newboard from current
        if (prevboard != 0x0000000000000000ULL) learn_eval(prevboard,newboard,temp_penalty);

        prevboard = newboard;
        uint64_t tile = draw_tile();
        temp_penalty = 0;
        if (tile == 2) {
            scorepenalty += 4;
            temp_penalty = 4;
        }
        board = insert_tile_rand(newboard, tile);
    }
    performance.push_back(score_board(board) - scorepenalty);
    //printf("%.0f %d \n",score_board(board)-scorepenalty,get_max_rank(board));
}

int main() {
    std::cout << "Initializing..." << std::endl;
    init_move_tables();
    init_config();
    num_ntuples = ntuples.size();

    // clock_t t1, t2;
    // t1 = clock();
    std::cout << "Training..." << std::endl;
    for (int i = 0; i < num_iterations; i++) {
        play_game();
        if ((100*i)/num_iterations > (100*(i-1))/num_iterations) {
            std::cout << (100*i)/num_iterations << "%.." << std::flush;
        }
    } 
    std::cout << std::endl << "Storing results..." << std::endl;
    std::ofstream outFile("../data/outvalues.txt");
    for (int i = 0; i < num_ntuples; i++) {
        for (int j = 0; j < learned_afterstates[i].size(); j++) {
            outFile << learned_afterstates[i][j] << " ";
        }
        outFile << std::endl;
    }
    outFile.close();
    outFile.open("../data/perform.txt");
    for (int i = 0; i < performance.size(); i++) {
        outFile << performance[i] << " ";
    }
    outFile << std::endl;
    outFile.close();

    std::cout << "Done." << std::endl;
    // t2 = clock();
    // printf("Time taken: %.2fs\n", (float)(t2 - t1)/CLOCKS_PER_SEC);
    return 0;
}






