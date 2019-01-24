/* The majority of this library was implemented by:
        
        https://github.com/nneonneo/2048-ai

This library contains only game logic, no AI.

============================================================ */

#ifndef MOVELIB_H
#define MOVELIB_H

#include <stdint.h>
// Move tables and board stuff.
void init_move_tables();
uint64_t initial_board();
uint64_t unpack_col(uint16_t row);
uint16_t reverse_row(uint16_t row);
uint64_t transpose(uint64_t x);
uint64_t rotate_right(uint64_t board);
uint64_t find_board_perm (int num, uint64_t board);
void print_board(uint64_t board);


// Individual tiles.
uint64_t draw_tile();
uint64_t insert_tile_rand(uint64_t board, uint64_t tile);
int count_empty(uint64_t x);
int get_max_rank(uint64_t board);
int count_distinct_tiles(uint64_t board);


// 0 -> Up, 1 -> Down, 2 -> Left, 3 -> Right
uint64_t execute_move_0(uint64_t board);
uint64_t execute_move_1(uint64_t board);
uint64_t execute_move_2(uint64_t board);
uint64_t execute_move_3(uint64_t board);
uint64_t execute_move(int move, uint64_t board);


// Functions for the score.
float score_helper(uint64_t board, const float* table);
float score_board(uint64_t board);

#endif // MOVELIB_H