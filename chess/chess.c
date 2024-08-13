/*
author: Andrew Tang
email: andrew73@umbc.edu
description: a implementation of chess in a kernel module
*/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/random.h>

MODULE_LICENSE("GPL");

// define constants for board dimension
#define BOARD_SIZE 8

// define characters for pieces
#define EMPTY "**"
#define PAWN_WP "WP"
#define KNIGHT_WN "WN"
#define BISHOP_WB "WB"
#define ROOK_WR "WR"
#define QUEEN_WQ "WQ"
#define KING_WK "WK"
#define PAWN_BP "BP"
#define KNIGHT_BN "BN"
#define BISHOP_BB "BB"
#define ROOK_BR "BR"
#define QUEEN_BQ "BQ"
#define KING_BK "BK"

#define DEV_NAME "chess"

// variables
static char game_board[BOARD_SIZE][BOARD_SIZE][3]; 
static char player_color;
static char cpu_color;
static bool game_started = false;
static bool player_turn = false;
static bool cpu_in_check = false;
static char output_message[256] = "";

// function prototypes
static ssize_t chess_read(struct file *filp, char __user *buf, size_t len, loff_t *off);
static ssize_t chess_write(struct file *filp, const char __user *buf, size_t len, loff_t *off);
static void initialize_board(void);
static void generate_cpu_move(void);
static void handle_cpu_turn(void);
static void handle_resign_game(void);

// file operations structure
static const struct file_operations chess_fops = {
    .owner = THIS_MODULE,
    .read = chess_read,
    .write = chess_write,
};

// misc device structure
static struct miscdevice chess_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEV_NAME,
    .fops = &chess_fops,
};

// random number generation
int random_number(int min, int max) {
    int num;
    get_random_bytes(&num, sizeof(num));
    return min + (abs(num) % (max - min));
}

// initialize the chess board
static void initialize_board() {
    int i, j;
    // fill with empty pieces
    for (i = 0; i < BOARD_SIZE; i++) {
        for (j = 0; j < BOARD_SIZE; j++) {
            strcpy(game_board[i][j], EMPTY);
        }
    }

    // place white pieces
    strcpy(game_board[0][0], ROOK_WR);
    strcpy(game_board[0][1], KNIGHT_WN);
    strcpy(game_board[0][2], BISHOP_WB);
    strcpy(game_board[0][3], QUEEN_WQ);
    strcpy(game_board[0][4], KING_WK);
    strcpy(game_board[0][5], BISHOP_WB);
    strcpy(game_board[0][6], KNIGHT_WN);
    strcpy(game_board[0][7], ROOK_WR);

    // place black pieces
    strcpy(game_board[7][0], ROOK_BR);
    strcpy(game_board[7][1], KNIGHT_BN);
    strcpy(game_board[7][2], BISHOP_BB);
    strcpy(game_board[7][3], QUEEN_BQ);
    strcpy(game_board[7][4], KING_BK);
    strcpy(game_board[7][5], BISHOP_BB);
    strcpy(game_board[7][6], KNIGHT_BN);
    strcpy(game_board[7][7], ROOK_BR);

    // place pawns of both colors
    for (i = 0; i < BOARD_SIZE; i++) {
        strcpy(game_board[1][i], PAWN_WP);
        strcpy(game_board[6][i], PAWN_BP);
    }
    
}

// display the current state of the board
static ssize_t display_board(char *buf, size_t len, loff_t *off) {
    int i, j;
    char result[1536] = ""; 
    ssize_t ret;

    // append game board state to the result string
    for (i = 0; i < BOARD_SIZE; i++) {
        char row_number[4];
        sprintf(row_number, "%d ", i + 1); // convert row number to string
        strcat(result, row_number);

        for (j = 0; j < BOARD_SIZE; j++) {
            char* piece = game_board[i][j];
            // color the piece based on player color
            if (piece[0] == 'W') {
                strcat(result, "\033[1;31m"); // white piece color
            } else if (piece[0] == 'B') {
                strcat(result, "\033[0;34m"); // black piece color
            }
            strcat(result, piece);
            strcat(result, "\033[0m "); // reset color
        }
        strcat(result, "\n");
    }
    // append column numbers to the result string
    strcat(result, "  a  b  c  d  e  f  g  h\n");

    // print it out to the device file
    ret = simple_read_from_buffer(buf, len, off, result, strlen(result));
    return ret;
}

// helper function that determines if there is a obstacle in the way for pawn, bishop, rook, and queen moves
static bool obstacles(int from_row, int from_col, int to_row, int to_col) {
    // calculation of col/ row displacement
    int row_step, col_step, row, col;
    int row_diff = to_row - from_row;
    int col_diff = to_col - from_col;

    // determines which way steps are taken
    if (row_diff > 0) {
        row_step = 1;
    } else {
        row_step = -1;
    }
    if (col_diff > 0) {
        col_step = 1;
    } else {
        col_step = -1;
    }

    // horizontal move (rook or queen)
    if (row_diff == 0) {
        for (col = from_col + col_step; col != to_col; col += col_step) {
            if (strcmp(game_board[from_row][col], EMPTY) != 0) {
                return true; // obstacle found
            }
        }
    }
    // vertical move (pawn or rook or queen)
    else if (col_diff == 0) {
        for (row = from_row + row_step; row != to_row; row += row_step) {
            if (strcmp(game_board[row][from_col], EMPTY) != 0) {
                return true; // obstacle found
            }
        }
    }
    // diagonal move (bishop or queen)
    else if (abs(row_diff) == abs(col_diff)) {
        row = from_row + row_step;
        col = from_col + col_step;

        // check for obstacles in the diagonal path
        while (row != to_row && col != to_col) {
            if (strcmp(game_board[row][col], EMPTY) != 0) {
                return true; // obstacle found
            }
            row += row_step;
            col += col_step;
        }
    }

    return false;
}

static bool validate_move(const char *move) {
    int from_col, from_row, to_col, to_row;

    if (strlen(move) != 7 && strlen(move) != 10 && strlen(move) != 13) {
        return false; // invalid move format
    }

    from_col = move[2] - 'a';
    from_row = move[3] - '1';
    to_col = move[5] - 'a';
    to_row = move[6] - '1';

    if (move[0] != 'W' && move[0] != 'B') {
        return false; // invalid piece color
    }
    if (from_row < 0 || from_row >= 8 || from_col < 0 || from_col >= 8 ||
        to_row < 0 || to_row >= 8 || to_col < 0 || to_col >= 8) {
        return false; // out of bounds
    }
    if (move[4] != '-') {
        return false; // bad marker
    }
    if (strncmp(game_board[from_row][from_col], move, 2) != 0) {
        return false; // piece is not present at the source square
    }

    // check that the movement of the piece is valid
    if (move[1] == 'P') {
        if (strlen(move) == 7) {
            if (move[0] == 'W') {
                if (!(from_col == to_col && (from_row + 1 == to_row || (from_row == 1 && to_row == 3)))) {
                    return false; // invalid pawn move for White
                }
                if (to_row == 7) {
                    return false; // need to promote
                }
            } else {
                if (!(from_col == to_col && (from_row - 1 == to_row || (from_row == 6 && to_row == 4)))) {
                    return false; // invalid pawn move for Black
                }
                if (to_row == 0) {
                    return false; // need to promote
                }
            }
            if (obstacles(from_row, from_col, to_row, to_col)) {
                return false; // pieces cannot move through other pieces
            }
        }
    } else if (move[1] == 'N') {
        int row_diff = abs(to_row - from_row);
        int col_diff = abs(to_col - from_col);
        if (!((row_diff == 2 && col_diff == 1) || (row_diff == 1 && col_diff == 2))) {
            return false; // bad knight move
        }
    } else if (move[1] == 'B') {
        if (!(abs(to_row - from_row) == abs(to_col - from_col))) {
            return false; // bad bishop move
        }
        if (obstacles(from_row, from_col, to_row, to_col)) {
            return false; // pieces cannot move through other pieces
        }
    } else if (move[1] == 'R') {
        if (!(from_row == to_row || from_col == to_col)) {
            return false; // bad rook move
        }
        if (obstacles(from_row, from_col, to_row, to_col)) {
            return false; // pieces cannot move through other pieces
        }
    } else if (move[1] == 'Q') {
        if (!(from_row == to_row || from_col == to_col || abs(to_row - from_row) == abs(to_col - from_col))) {
            return false; // bad queen move
        }
        if (obstacles(from_row, from_col, to_row, to_col)) {
            return false; // pieces cannot move through other pieces
        }
    } else if (move[1] == 'K') {
        if (!((abs(to_row - from_row) <= 1) && (abs(to_col - from_col) <= 1))) {
            return false; // bad king move
        }
    } else {
        return false; // bad piece type
    }

    // check that the destination is empty
    if (strlen(move) == 7) {
        if (strncmp(game_board[to_row][to_col], EMPTY, 2) != 0) {
            return false; // no empty space
        }
    }

    if (strlen(move) == 10) {
        if (move[7] == 'x') {
            if (move[8] != 'W' && move[8] != 'B') {
                return false; // invalid capturing piece color
            }
            if (move[8] == move[0]) {
                return false; // capturing own piece
            }
            if (strncmp(game_board[to_row][to_col], move + 8, 2) != 0) {
                return false; // piece to be captured isn't present
            }
            if (move[1] == 'P') {
                if (move[0] == 'W') {
                    if ((to_row != from_row + 1 || abs(to_col - from_col) != 1)) {
                        return false; // invalid pawn capture for white
                    }
                    if (to_row == 7) {
                        return false; // need to promote
                    }
                } else {
                    if ((to_row != from_row - 1 || abs(to_col - from_col) != 1)) {
                        return false; // invalid pawn capture for black
                    }
                    if (to_row == 0) {
                        return false; // need to promote
                    }
                }
            }
        } else if (move[7] == 'y') {
            if (move[1] != 'P') {
                return false; // promoted piece is not pawn 
            }
            if (move[0] != move[8]) {
                return false; // color of promotion is invalid
            }
            if (move[9] != 'Q' && move[9] != 'R' && move[9] != 'B' && move[9] != 'N') {
                return false; // type of peice to be promoted is invalid
            }
            if (move[0] == 'W') {
                if (from_row != 6 || to_row != 7 || from_col != to_col) {
                    return false; // invalid move
                }
            } else {
                if (from_row != 1 || to_row != 0 || from_col != to_col) {
                    return false; // invalid move
                }
            }
            if (strncmp(game_board[to_row][to_col], EMPTY, 2) != 0) {
                return false; // make sure that the tile is empty
            }
            game_board[from_row][from_col][1] = move[9]; // do the promotion
        }
        else {
            // bad marker
            return false; 
        }
    }

    if (strlen(move) == 13) {
        if (move[1] != 'P') {
            return false;
        }
        if (move[0] != move[11]) {
            return false; // promoted piece color doesn't match the pawn color
        }
        if (move[7] != 'x') {
            return false; // invalid marker for capture move
        }
        if (move[10] != 'y') {
            return false; // bad marker for promotion after capture
        }
        if (move[8] != 'W' && move[8] != 'B') {
            return false; // invalid capturing piece color
        }
        if (move[8] == move[0]) {
            return false; // capturing own piece
        }
        if (strncmp(game_board[to_row][to_col], move + 8, 2) != 0) {
            return false; // piece to be captured isn't present
        }
        if (move[0] == 'W') {
            if ((from_row != 6 || to_row != 7 || abs(to_col - from_col) != 1 )) {
                return false; // invalid pawn capture for White
            }
        } else {
            if ((from_row != 1 || to_row != 0|| abs(to_col - from_col) != 1)) {
                return false; // invalid pawn capture for Black
            }
        }

        if (move[12] != 'Q' && move[12] != 'R' && move[12] != 'B' && move[12] != 'N') {
            return false; // wrong type of promotion
        }
        game_board[from_row][from_col][1] = move[12]; // do the promotion
    }

    return true;
}

// function to update the game state based on the player's move
static void update_game_state(const char *move) {
    int from_col = move[2] - 'a';
    int from_row = move[3] - '1';
    int to_col = move[5] - 'a';
    int to_row = move[6] - '1';

    // perform the move
    strcpy(game_board[to_row][to_col], game_board[from_row][from_col]);
    strcpy(game_board[from_row][from_col], EMPTY);
}

// function to generate a move string
char* generate_move_capture(int from_row, int from_col, int to_row, int to_col) {
    static char move_string[11]; // Format is in "BNb8-c6xWP\0"
    // convert row and column indices to integers
    char from_col_char = 'a' + from_col;
    char to_col_char = 'a' + to_col;
    int from_row_num = from_row + 1; 
    int to_row_num = to_row + 1; 
    char current_color = game_board[from_row][from_col][0]; 
    char current_piece = game_board[from_row][from_col][1]; 
    char opponent_color = game_board[to_row][to_col][0]; 
    char opponent_piece = game_board[to_row][to_col][1]; 
    // construct the move string
    sprintf(move_string, "%c%c%c%d-%c%dx%c%c", current_color, current_piece, from_col_char, from_row_num, to_col_char, to_row_num, opponent_color, opponent_piece);

    return move_string;
}

// function to generate a move string without considering captures
char* generate_move_non_capture(int from_row, int from_col, int to_row, int to_col) {
    static char move_string[8]; // Format is in "BNb8-c6\0"
    // convert row and column indices to characters
    char from_col_char = 'a' + from_col;
    char to_col_char = 'a' + to_col;
    int from_row_num = from_row + 1; 
    int to_row_num = to_row + 1; 
    char current_color = game_board[from_row][from_col][0]; 
    char current_piece = game_board[from_row][from_col][1]; 
    // construct the move string without considering captures
    sprintf(move_string, "%c%c%c%d-%c%d", current_color, current_piece, from_col_char, from_row_num, to_col_char, to_row_num);

    return move_string;
}

static bool is_opponent_in_check(char curr_color) {
    int king_row, king_col;
    int row, col; 
    king_row = 0; 
    king_col = 0;
    // find the position of the opponent's king
    for (row = 0; row < BOARD_SIZE; row++) {
        for (col = 0; col < BOARD_SIZE; col++) {
            if (game_board[row][col][1] == 'K' && game_board[row][col][0] != curr_color) {
                king_row = row;
                king_col = col;
            }
        }
    }

    // check if any of the pieces of the given color can attack the opponent's king
    for (row = 0; row < BOARD_SIZE; row++) {
        for (col = 0; col < BOARD_SIZE; col++) {
            if (game_board[row][col][0] == curr_color) {
                if (validate_move(generate_move_capture(row, col, king_row, king_col))) {
                    // the opponent's king is in check
                    return true;
                }
            }
        }
    }

    // the opponent's king is not in check
    return false;
}

// helper that simulates the move and checks if the peice is still in check
static bool try_and_undo(int from_row, int from_col, int to_row, int to_col, char curr_color) {
    bool out_of_check; 
    char piece[3];
    char captured_piece[3];
    strcpy(piece, game_board[from_row][from_col]);
    strcpy(captured_piece, game_board[to_row][to_col]);

    // try making the move
    strcpy(game_board[to_row][to_col], piece);
    strcpy(game_board[from_row][from_col], EMPTY);

    // check if the move gets the opponent's king out of check
    out_of_check = !is_opponent_in_check(curr_color);

    // undo the move
    strcpy(game_board[from_row][from_col], piece);
    strcpy(game_board[to_row][to_col], captured_piece);

    return out_of_check;
}

// checks if opponent is in checkmate
static bool is_opponent_in_checkmate(char curr_color) {
    int row, col, new_row, new_col;
    if (!is_opponent_in_check(curr_color)) {
        // if the opponent's king is not in check, so it is not in checkmate
        return false;
    }

    // check if there are any legal moves the opponent can make to get out of check
    for (row = 0; row < BOARD_SIZE; row++) {
        for (col = 0; col < BOARD_SIZE; col++) {
            if (game_board[row][col][0] != curr_color) {
                // try moving each piece to every possible square and check if it gets out of check
                for (new_row = 0; new_row < BOARD_SIZE; new_row++) {
                    for (new_col = 0; new_col < BOARD_SIZE; new_col++) {
                        if (validate_move(generate_move_non_capture(row, col, new_row, new_col)) || 
                            validate_move(generate_move_capture(row, col, new_row, new_col))) {
                            // if the move is valid, check if it gets the king out of check
                            if (try_and_undo(row, col, new_row, new_col, curr_color)) {
                                return false;
                            }
                        }
                    }
                }
            }
        }
    }

    // if no legal moves can get the opponent's king out of check, it's checkmate
    return true;
}

// function to handle the player's move
static void handle_player_move(const char *buf, size_t len, loff_t *off, const char *move) {
    // check if there's an active game
    if (!game_started) {
        strcpy(output_message, "NOGAME\n");
        return;
    }

    if (!player_turn) {
        strcpy(output_message, "OOT\n");
        return;
    }

    if (move[0] != player_color) {
        strcpy(output_message, "ILLMOVE\n");
        return;
    }

    // validate the move
    if (!validate_move(move)) {
        strcpy(output_message, "ILLMOVE\n");
        return;
    }
    
    // check if the move puts the player's own king in check
    if (!try_and_undo(move[3] - '1', move[2] - 'a', move[6] - '1', move[5] - 'a', cpu_color)) {
        strcpy(output_message, "ILLMOVE\n");
        return;
    }

    // update the game state with the player's move
    update_game_state(move);

    // respond based on the move's validity and game state
    if (is_opponent_in_checkmate(player_color)) {
        if (player_color == 'W') {
            strcpy(output_message, "MATE\nWHITE WINS\n");
        } 
        else {
            strcpy(output_message, "MATE\nBLACK WINS\n");
        }
        game_started = false;
    } 
    else if (is_opponent_in_check(player_color)) {
        strcpy(output_message, "CHECK\n");
        cpu_in_check = true;
    } 
    else {
        strcpy(output_message, "OK\n");
    }

    // set player's turn
    player_turn = false;
}

// function to generate a CPU move
static void generate_cpu_move() {
    int to_row, to_col, from_row, from_col;
    char move[20]; // buffer to hold the move string
    int num_non_capture_moves = 0;
    char non_capture_moves[BOARD_SIZE * BOARD_SIZE][20]; // array to store non-capture moves
    
    // when cpu is in check, get out of check 
    if (cpu_in_check) {
        // iterate over all pieces
        for (from_row = 0; from_row < BOARD_SIZE; from_row++) {
            for (from_col = 0; from_col < BOARD_SIZE; from_col++) {
                if (game_board[from_row][from_col][0] != player_color) {
                    // try moving each piece to every possible square and check if it gets out of check
                    for (to_row = 0; to_row < BOARD_SIZE; to_row++) {
                        for (to_col = 0; to_col < BOARD_SIZE; to_col++) {
                            if (validate_move(generate_move_non_capture(from_row, from_col, to_row, to_col)) || 
                                validate_move(generate_move_capture(from_row, from_col, to_row, to_col))) {
                                // if the move is valid, check if it gets the king out of check
                                if (try_and_undo(from_row, from_col, to_row, to_col, player_color)) {
                                    // execute the move and update game state
                                    update_game_state(generate_move_non_capture(from_row, from_col, to_row, to_col));
                                    cpu_in_check = false; 
                                    return; 
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // get it out of the check state if it exited the loop
    cpu_in_check = false; 

    // CPU always selects the first valid capture move it finds
    for (to_row = 0; to_row < BOARD_SIZE; to_row++) {
        for (to_col = 0; to_col < BOARD_SIZE; to_col++) {
            if (game_board[to_row][to_col][0] == player_color) { 
                for (from_row = 0; from_row < BOARD_SIZE; from_row++) {
                    for (from_col = 0; from_col < BOARD_SIZE; from_col++) {
                        if (game_board[from_row][from_col][0] != player_color) { 
                            strcpy(move, generate_move_capture(from_row, from_col, to_row, to_col)); 
                            if (validate_move(move)) {
                                // execute the CPU move
                                update_game_state(move);
                                return;
                            }
                        }
                    }
                }
            }
        }
    }

    // generate all valid non-capture moves and store them in the array
    for (to_row = 0; to_row < BOARD_SIZE; to_row++) {
        for (to_col = 0; to_col < BOARD_SIZE; to_col++) { 
            for (from_row = 0; from_row < BOARD_SIZE; from_row++) {
                for (from_col = 0; from_col < BOARD_SIZE; from_col++) {
                    if (game_board[from_row][from_col][0] != player_color) { 
                        strcpy(move, generate_move_non_capture(from_row, from_col, to_row, to_col)); 
                        if (validate_move(move)) {
                            // check if there is space in the array to store the move
                            if (num_non_capture_moves < BOARD_SIZE * BOARD_SIZE) {
                                strcpy(non_capture_moves[num_non_capture_moves], move);
                                num_non_capture_moves++;
                            } else {
                                // array is full, so break out of the loop
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    
    // if there are no valid non-capture moves, return
    if (num_non_capture_moves == 0) {
        return;
    }

    strcpy(move, non_capture_moves[random_number(0, num_non_capture_moves - 1)]);
    // execute the CPU move
    update_game_state(move);
}

// function to handle the CPU's turn
static void handle_cpu_turn() {
    if (!game_started) {
        strcpy(output_message, "NOGAME\n");
        return;
    }

    if (player_turn) {
        strcpy(output_message, "OOT\n");
        return;
    }

    // generate CPU move
    generate_cpu_move();

    // check game state after CPU move
    if (is_opponent_in_checkmate(cpu_color)) {
        if (player_color == 'W') {
            strcpy(output_message, "MATE\nBLACK WINS\n");
        } 
        else {
            strcpy(output_message, "MATE\nWHITE WINS\n");
        }
        game_started = false;
    } 
    else if (is_opponent_in_check(cpu_color)) {
        strcpy(output_message, "CHECK\n");
    } 
    else {
        strcpy(output_message, "OK\n");
    }

    // set player's turn
    player_turn = true;
}

// function to handle the player resigning the game
static void handle_resign_game() {
    // check if there's an active game
    if (!game_started) {
        strcpy(output_message, "NOGAME\n");
        return;
    }

    // check if it's the player's turn
    if (!player_turn) {
        strcpy(output_message, "OOT\n");
        return;
    }

    // if check state is active
    if (is_opponent_in_check(cpu_color) || (is_opponent_in_check(player_color))) {
        strcpy(output_message, "CHECK\n");
    }

    // the player resigns, so CPU wins
    if (player_color == 'W') {
        strcpy(output_message, "OK\nBLACK WINS\n");
    } 
    else {
        strcpy(output_message, "OK\nWHITE WINS\n");
    }
    game_started = false;
    player_turn = false;
}

// read from the device
static ssize_t chess_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {
    ssize_t ret = 0; 
    // process the command
    if (strcmp(output_message, "DISPLAY\n") == 0) {
        ret = display_board(buf, len, off);
    } 
    else {
        // copy output_message to user buffer
        ret = simple_read_from_buffer(buf, len, off, output_message, strlen(output_message));
    }

    return ret;
}

static ssize_t chess_write(struct file *filp, const char __user *buf, size_t len, loff_t *off) {
    char command[21]; // Fixed-size buffer to hold the command string (maximum length is 20 characters)
    
    // command cannot be larger than this many characters
    if (len > 20) {
        strcpy(output_message, "UNKCMD\n");
        return len; 
    }

    // copy command from user space
    if (copy_from_user(command, buf, len)) {
        strcpy(output_message, "UNKCMD\n");
        return len;
    }

    // check if the last character is a newline
    if (command[len - 1] != '\n') {
        strcpy(output_message, "UNKCMD\n");
        return len;
    }

    // ensure the command string is null-terminated
    command[len - 1] = '\0';

    if (strncmp(command, "00 W", 4) == 0) { // start new game as white
        if (len != 5) { 
            // command length must be exactly 4 characters + newline
            strcpy(output_message, "INVFMT\n");
        } else {
            player_color = 'W';
            cpu_color = 'B';
            initialize_board();
            game_started = true;
            player_turn = true;
            strcpy(output_message, "OK\n");
        }
    } 
    else if (strncmp(command, "00 B", 4) == 0) { // start new game as black
        if (len != 5) { 
            // command length must be exactly 4 characters + newline
            strcpy(output_message, "INVFMT\n");
        } else {
            player_color = 'B';
            cpu_color = 'W';
            initialize_board();
            game_started = true;
            player_turn = false;
            strcpy(output_message, "OK\n");
        }
    } 
    else if (strncmp(command, "01", 2) == 0) { // gets the current state of the game
        if (len != 3) { 
            // command length must be exactly 2 characters + newline
            strcpy(output_message, "INVFMT\n");
        } else if (game_started) {
            // when game has been started
            strcpy(output_message, "DISPLAY\n");
        } else {
            // when no game has been started
            strcpy(output_message, "NOGAME\n");
        }
    } 
    else if (strncmp(command, "02 ", 3) == 0) { // player move
        handle_player_move(buf, len, off, command + 3); 
    } 
    else if (strncmp(command, "03", 2) == 0) { // CPU move
        if (len != 3) { 
            // command length must be exactly 2 characters + newline
            strcpy(output_message, "INVFMT\n");
        }
        else {
            handle_cpu_turn();
        }
    } 
    else if (strncmp(command, "04", 2) == 0) { // ends game
        if (len != 3) { 
            // command length must be exactly 2 characters + newline
            strcpy(output_message, "INVFMT\n");
        }
        else {
            handle_resign_game();
        }
    } 
    else { // When none of the commands matched
        strcpy(output_message, "UNKCMD\n");
    }
    return len;
}

// module initialization function
static int __init chess_init(void) {
    int ret;

    ret = misc_register(&chess_misc_device);
    if (ret) {
        printk(KERN_ALERT "Could not register misc device\n");
        return ret;
    }

    return 0;
}

// module exit function
static void __exit chess_exit(void) {
    misc_deregister(&chess_misc_device);
}

// calls initialization and exit
module_init(chess_init);
module_exit(chess_exit);
