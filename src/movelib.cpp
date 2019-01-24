/* The majority of this library was implemented by:
        
        https://github.com/nneonneo/2048-ai

This library contains only game logic, no AI.

============================================================ */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <algorithm>
#include "movelib.h"

static const uint64_t ROW_MASK = 0xFFFFULL;
static const uint64_t COL_MASK = 0x000F000F000F000FULL;
static uint16_t row_left_table [65536];
static uint16_t row_right_table[65536];
static uint64_t col_up_table[65536];
static uint64_t col_down_table[65536];
static float score_table[65536];


uint64_t unpack_col(uint16_t row) {
    uint64_t tmp = row;
    return (tmp | (tmp << 12ULL) | (tmp << 24ULL) | (tmp << 36ULL)) & COL_MASK;
}

uint16_t reverse_row(uint16_t row) {
    return (row >> 12) | ((row >> 4) & 0x00F0)  | ((row << 4) & 0x0F00) | (row << 12);
}

void print_board(uint64_t board) {
    int i,j;
    for(i=0; i<4; i++) {
        for(j=0; j<4; j++) {
            uint8_t powerVal = (board) & 0xf;
            printf("%6u", (powerVal == 0) ? 0 : 1 << powerVal);
            board >>= 4;
        }
        printf("\n");
    }
    printf("\n");
}

uint64_t transpose(uint64_t x) {
    uint64_t a1 = x & 0xF0F00F0FF0F00F0FULL;
    uint64_t a2 = x & 0x0000F0F00000F0F0ULL;
    uint64_t a3 = x & 0x0F0F00000F0F0000ULL;
    uint64_t a = a1 | (a2 << 12) | (a3 >> 12);
    uint64_t b1 = a & 0xFF00FF0000FF00FFULL;
    uint64_t b2 = a & 0x00FF00FF00000000ULL;
    uint64_t b3 = a & 0x00000000FF00FF00ULL;
    return b1 | (b2 >> 24) | (b3 << 24);
}

uint64_t rotate_right(uint64_t board) {
	uint64_t soln = 0x0000000000000000ULL;
	soln |= (board & 0x000000000000000FULL) << 12;
	soln |= (board & 0x00000000000000F0ULL) << 24;
	soln |= (board & 0x0000000000000F00ULL) << 36;
	soln |= (board & 0x000000000000F000ULL) << 48;
	soln |= (board & 0x00000000000F0000ULL) >> 8;
	soln |= (board & 0x0000000000F00000ULL) << 4;
	soln |= (board & 0x000000000F000000ULL) << 16;
	soln |= (board & 0x00000000F0000000ULL) << 28;
	soln |= (board & 0x0000000F00000000ULL) >> 28;
	soln |= (board & 0x000000F000000000ULL) >> 16;
	soln |= (board & 0x00000F0000000000ULL) >> 4;
	soln |= (board & 0x0000F00000000000ULL) << 8;
	soln |= (board & 0x000F000000000000ULL) >> 48;
	soln |= (board & 0x00F0000000000000ULL) >> 36;
	soln |= (board & 0x0F00000000000000ULL) >> 24;
	soln |= (board & 0xF000000000000000ULL) >> 12; 
	return soln;
}

// Count the number of empty positions (= zero nibbles) in a board.
// Precondition: the board cannot be fully empty.
int count_empty(uint64_t x) {
    x |= (x >> 2) & 0x3333333333333333ULL;
    x |= (x >> 1);
    x = ~x & 0x1111111111111111ULL;
    // At this point each nibble is:
    //  0 if the original nibble was non-zero
    //  1 if the original nibble was zero
    // Next sum them all
    x += x >> 32;
    x += x >> 16;
    x += x >>  8;
    x += x >>  4; // this can overflow to the next nibble if there were 16 empty positions
    return x & 0xf;
}

void init_move_tables() {

    for (unsigned row = 0; row < 65536; ++row) {
        unsigned line[4] = {
                (row >>  0) & 0xf,
                (row >>  4) & 0xf,
                (row >>  8) & 0xf,
                (row >> 12) & 0xf
        };

        // Score
        float score = 0.0f;
        for (int i = 0; i < 4; ++i) {
            int rank = line[i];
            if (rank >= 2) {
                // the score is the total sum of the tile and all intermediate merged tiles
                score += (rank - 1) * (1 << rank);
            }
        }
        score_table[row] = score;

        // execute a move to the left
        for (int i = 0; i < 3; ++i) {
            int j;
            for (j = i + 1; j < 4; ++j) {
                if (line[j] != 0) break;
            }
            if (j == 4) break; // no more tiles to the right

            if (line[i] == 0) {
                line[i] = line[j];
                line[j] = 0;
                i--; // retry this entry
            } else if (line[i] == line[j]) {
                if(line[i] != 0xf) {
                    /* Pretend that 32768 + 32768 = 32768 (representational limit). */
                    line[i]++;
                }
                line[j] = 0;
            }
        }

        uint16_t result = (line[0] <<  0) |
                       (line[1] <<  4) |
                       (line[2] <<  8) |
                       (line[3] << 12);
        uint16_t rev_result = reverse_row(result);
        unsigned rev_row = reverse_row(row);

        row_left_table [    row] =                row  ^                result;
        row_right_table[rev_row] =            rev_row  ^            rev_result;
        col_up_table   [    row] = unpack_col(    row) ^ unpack_col(    result);
        col_down_table [rev_row] = unpack_col(rev_row) ^ unpack_col(rev_result);
    }
}

uint64_t execute_move_0(uint64_t board) {
    uint64_t ret = board;
    uint64_t t = transpose(board);
    ret ^= col_up_table[(t >>  0) & ROW_MASK] <<  0;
    ret ^= col_up_table[(t >> 16) & ROW_MASK] <<  4;
    ret ^= col_up_table[(t >> 32) & ROW_MASK] <<  8;
    ret ^= col_up_table[(t >> 48) & ROW_MASK] << 12;
    return ret;
}

uint64_t execute_move_1(uint64_t board) {
    uint64_t ret = board;
    uint64_t t = transpose(board);
    ret ^= col_down_table[(t >>  0) & ROW_MASK] <<  0;
    ret ^= col_down_table[(t >> 16) & ROW_MASK] <<  4;
    ret ^= col_down_table[(t >> 32) & ROW_MASK] <<  8;
    ret ^= col_down_table[(t >> 48) & ROW_MASK] << 12;
    return ret;
}

uint64_t execute_move_2(uint64_t board) {
    uint64_t ret = board;
    ret ^= uint64_t(row_left_table[(board >>  0) & ROW_MASK]) <<  0;
    ret ^= uint64_t(row_left_table[(board >> 16) & ROW_MASK]) << 16;
    ret ^= uint64_t(row_left_table[(board >> 32) & ROW_MASK]) << 32;
    ret ^= uint64_t(row_left_table[(board >> 48) & ROW_MASK]) << 48;
    return ret;
}

uint64_t execute_move_3(uint64_t board) {
    uint64_t ret = board;
    ret ^= uint64_t(row_right_table[(board >>  0) & ROW_MASK]) <<  0;
    ret ^= uint64_t(row_right_table[(board >> 16) & ROW_MASK]) << 16;
    ret ^= uint64_t(row_right_table[(board >> 32) & ROW_MASK]) << 32;
    ret ^= uint64_t(row_right_table[(board >> 48) & ROW_MASK]) << 48;
    return ret;
}

uint64_t execute_move(int move, uint64_t board) {
    switch(move) {
    case 0: // up
        return execute_move_0(board);
    case 1: // down
        return execute_move_1(board);
    case 2: // left
        return execute_move_2(board);
    case 3: // right
        return execute_move_3(board);
    default:
        return ~0ULL;
    }
}

uint64_t initial_board() {
    uint64_t board = draw_tile() << (4 * rand()%16);
    return insert_tile_rand(board, draw_tile());
}

uint64_t insert_tile_rand(uint64_t board, uint64_t tile) {
    // don't forget to init the seed 
    int index = rand()%count_empty(board);
    uint64_t tmp = board;
    while (true) {
        while ((tmp & 0xf) != 0) {
            tmp >>= 4;
            tile <<= 4;
        }
        if (index == 0) break;
        --index;
        tmp >>= 4;
        tile <<= 4;
    }
    return board | tile;
}

int get_max_rank(uint64_t board) {
    int maxrank = 0;
    while (board) {
        maxrank = std::max(maxrank, int(board & 0xf));
        board >>= 4;
    }
    return maxrank;
}

int count_distinct_tiles(uint64_t board) {
    uint16_t bitset = 0;
    while (board) {
        bitset |= 1<<(board & 0xf);
        board >>= 4;
    }

    // Don't count empty tiles.
    bitset >>= 1;

    int count = 0;
    while (bitset) {
        bitset &= bitset - 1;
        count++;
    }
    return count;
}

uint64_t find_board_perm (int num, uint64_t board) {
	switch (num) {
		case 0:
			return board;
		case 1:
			return rotate_right(board);
		case 2:
			return rotate_right(rotate_right(board));
		case 3:
			return rotate_right(rotate_right(rotate_right(board)));
		case 4:
			return transpose(board);
		case 5:
		 	return transpose(rotate_right(board));
		case 6:
		 	return transpose(rotate_right(rotate_right(board)));
		case 7:
			return transpose(rotate_right(rotate_right(rotate_right(board))));

	}
}

uint64_t draw_tile() {
    return (rand()%10 < 9) ? 1 : 2;
}

float score_helper(uint64_t board, const float* table) {
    return table[(board >>  0) & ROW_MASK] +
           table[(board >> 16) & ROW_MASK] +
           table[(board >> 32) & ROW_MASK] +
           table[(board >> 48) & ROW_MASK];
}

float score_board(uint64_t board) {
    return score_helper(board, score_table);
}


