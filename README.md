**How To Use**
========================================
In my project3/chess directory, enter:
make
make u 
make l
This will create my driver, unload the driver module and then load the module. 

Now go to my project3/chess_driver, enter:
make 
make run
Now you can directly enter the commands and it with echo the command and cat the output, type in "exit" to exit. 
========================================
**Data Structures**
========================================
__Chessboard Representation: Describe how the chessboard is represented in memory. Explain the choice of data structure (e.g., a two-dimensional array, a single array with 64 elements, or a more complex data structure like a hash map for pieces and their positions).__
-------------------------------------------------------

I represented the chessboard using a 2D array called game_board. Each element of this array represents a square on the chessboard and holds a string indicating the piece occupying that square. The size of each square is three bytes, one for the color, one for the peice,and the last one for the null terminator. This choice simplifies accessing specific squares and makes the program much simplier and avoid dynamically allocated memory. In addition, these elements should also be sequential, making accessing elementing through a loop much faster. 

__Piece Encoding: Detail how pieces are encoded within your data structure, including both type (pawn, knight, bishop, rook, queen, king) and color (white, black).__
-------------------------------------------------------
In my implementation, each piece is encoded using a two-character string within the game_board array. The first character represents the color of the piece ('W' for white and 'B' for black), while the second character represents the type of the piece. 

Below is a list of pieces and their corresponding characters. 

    Pawn: "P"
    Knight: "N"
    Bishop: "B"
    Rook: "R"
    Queen: "Q"
    King: "K"

For example, 'WP' denotes a white pawn, 'BN' denotes a black knight, 'WR' denotes a white rook, and so on.

__Game State Management: Explain the data structure used to keep track of the game state, including the turn, check status, and history of moves for potential "undo" functionality or move validation.__
-------------------------------------------------------
The game_board array represents the chessboard's current configuration.

Boolean variables like game_started indicate the game's active status.

player_turn tracks whose turn it is.

output_message holds information for display.

Move history is implicitly tracked by updating game_board with each move, enabling undo functionality for checkmate tracking and move validation.

========================================
**Move Validation**
========================================
__General Approach: Outline the algorithm or method used to validate moves. Include how you handle different pieces' move sets, including special moves like castling and en passant.__
-------------------------------------------------------
Here is a outline of how my verification algorithm works. 

General Validations: 
Ensure the move string length matches expected formats (7, 10, or 13 characters).

Obtain source and destination coordinates from the move string.

Verify that the piece color is either 'W' (white) or 'B' (black).

Ensure that the source and destination coordinates are within the bounds of the chessboard (8x8 grid).

Validate the marker between source and destination coordinates. 

Confirm that a piece exists at the source square.

Piece-Specific Validations:
    Pawn ('P'): Check pawn moves including initial two-square advance, regular advances, captures, and promotion.

    Knight ('N'): Verify valid knight moves in L-shape.

    Bishop ('B'): Validate bishop moves along diagonals.

    Rook ('R'): Ensure valid rook moves along horizonals and verticals.

    Queen ('Q'): Check queen moves along horizonals, verticles, and diagonals.

    King ('K'): Verify valid king moves one square in any direction.

    Obstacle Detection: Ensure pieces cannot move through other pieces (calls a helper function).

Move-Specific Validations: 
    Check that the destination square is empty for non-capture moves.
    
    Handle capturing moves, ensuring the captured piece's color is different from the moving piece.

    Handle pawn promotion when reaching the opposite end of the board.

    Allow promotion after a pawn captures a piece and reaches the opposite end of the board.

If everything is alright, it returns true and allows the move to be updated. 

__Collision Detection: Describe how the module checks for obstructions in a piece's path and how it handles captures.__
-------------------------------------------------------
To ensure pieces cannot hop over obstacles, I implemented the obstacles function. Here's how it works:

I calculate the row and column displacement between the source and destination squares.

Based on this, I determine the direction of movement for rows and columns.

For horizontal moves, I check each column in the path.

For vertical moves, I check each row in the path.

For diagonal moves, I traverse each square in the diagonal path.

At each step, I verify if the square is occupied. If so, I detect an obstacle.

This function is only for Pawns, Bishops, Rooks, and Queens. 

Knights and Kings do not have to worry above obstacles being in the way. 

Captures are handled by my validate moves function and it checks for a capture that the specified square holds the peice that the user states is to be captured. 

In addition for more special captures like for the Pawn, it checks that the capture is diagonal one square instead of vertical like the normal move. 

Another special case is the Pawn promotion with the capture. 

__Special Cases: Detail the handling of special moves and conditions, such as pawn promotion, check, and checkmate.__
-------------------------------------------------------

Pawn Promotion:
    When a pawn reaches the opposite end of the board, it is promoted to a queen by default.
    In the validate_move function, I check if a pawn reaches the 8th (for white) or 1st (for black) rank.
    If so, and the move is valid, I update the pawn to a queen of the same color.

Check:
    I use the is_opponent_in_check function to determine if the opponent's king is in check after each move.

    If the opponent's king is in check, I set the output message to "CHECK\n".

Checkmate:
    Checkmate occurs when the opponent's king is in check, and there are no legal moves that can be made to escape the check.
    I check for checkmate using the is_opponent_in_checkmate function after each move.

    If the opponent is in checkmate, I set the output message to "MATE\n[COLOR] WINS\n", where [COLOR] represents the winning side.

========================================
**Check and Checkmate Detection**
========================================
__Check Detection: Explain the algorithm for determining if a king is in check, including how threats are identified from each piece type.__
-------------------------------------------------------

The is_opponent_in_check function determines if the opponent's king is in check through these stages:

Find the Opponent's King:
    The function iterates over the entire game board to find the position of the opponent's king.
    It checks each square on the board, looking for a king ('K') that belongs to the opponent.
    
Identify Threats to the King:
    After locating the opponent's king, the function iterates over the entire board again.
    For each piece of the current player's color, it generates potential moves that capture the opponent's king.
    It does this by calling the generate_move_capture function, which generates a move from the piece's position to the king's position, assuming it's a capturing move.
    The validate_move function is then called to check if the generated move is valid. If it is, it means that the opponent's king is under threat.
    
Check Result:
    If any piece of the current player's color can legally capture the opponent's king, the function returns true, indicating that the opponent's king is in check.
    If no such threat exists, the function returns false, indicating that the opponent's king is not in check.

__Checkmate Detection: Outline the logic used to determine if a player is in checkmate, considering the king's inability to escape check through any means.__
-------------------------------------------------------

The is_opponent_in_checkmate function determines if the opponent is in checkmate through these stages:

Check if Opponent is in Check:
    The function first checks if the opponent's king is currently in check. If not, it returns false immediately because checkmate cannot occur if the king is not in check.

Search for Legal Moves:
    If the opponent's king is in check, the function iterates over the entire board to find all pieces belonging to the opponent.
    For each piece found, it generates all possible legal moves (both capturing and non-capturing) for that piece.

Attempt and Undo Moves:
    For each legal move generated, the function simulates making the move on a temporary board.
    It then checks if the opponent's king is still in check after making that move.
    To do this, it calls the try_and_undo function, which temporarily makes the move on the board, checks if the opponent's king is still in check, and then undoes the move.
    If the move successfully gets the opponent's king out of check, the function returns false, indicating that the opponent is not in checkmate.

Checkmate Determination:
    If no legal move can get the opponent's king out of check, the function returns true, indicating that the opponent is in checkmate.

========================================
**CPU Move Determination**
========================================
__Strategy Implementation: Describe how the CPU decides on moves. Discuss if you're implementing a specific algorithm (like Minimax with Alpha-Beta pruning) or a simpler heuristic approach.__
-------------------------------------------------------
The CPU move generation algorithm follows a two-step process: first, it searches for capturing moves, and if none are found, it selects a non-capturing move. Here's a simplified overview:

Capturing Move Selection:
    The CPU scans the board for opponent pieces and then its own pieces.
    It generates capturing moves for each pair of positions and checks their validity.
    If a valid capturing move is found, it executes it and returns.

Non-Capturing Move Selection:
    If no capturing move is available, the CPU explores non-capturing moves.
    Valid non-capturing moves are stored, and one is randomly selected for execution.

__Difficulty Levels: If applicable, explain how different difficulty levels are managed and how they affect the CPU's decision-making process.__
-------------------------------------------------------
For the purposes of this base project, there was nothing on different difficulty levels described, so they were not implemented. 

========================================
**Extra Credit Implementations**
========================================
__Description of Features: For any extra credit features (such as support for castling, en passant, advanced AI strategies), provide detailed descriptions of how these features are implemented and integrated into the overall module.__
-------------------------------------------------------

Though saving and loading was not done, my I just want to mention that my AI does much better than just choosing random moves; 
if it is in a check state, it does everything it can to get out of the state by moving peices to protect the king or moving the king itself.

In addition, it first tries to do a capture before it does a non capture moves. This makes the AI have a sort of greedy algorithm, that makes a decent move every turn. 

__Impact on Game Play: Discuss how these features enhance the gameplay or the user experience.__
-------------------------------------------------------
It allows users to play against a decently competent opponent, giving them some level of challenge as they play the game. 

========================================
**Design Rationale**
========================================
__Decision Justifications: For each major design decision (e.g., data structures, algorithms, extra features), explain why this choice was made over the alternatives.__
-------------------------------------------------------
Data Structures:
2D Array for Game Board: 
    This choice was made because it represents the chessboard naturally. Each cell in the array corresponds to a square on the board, making it intuitive to implement movement and piece interactions.
Algorithms:
    Brute-Force Move Generation: The decision to use a brute-force approach for move generation simplifies implementation. While more sophisticated algorithms could improve efficiency, they also increase complexity significantly, which might make it much harder to debug and understand. 
    
    Additionally since there is only a maximum of 32 pieces on the board at a time, its not really such a big deal if the time compelxity is bad. 

__Complexity Considerations: Address any considerations made regarding the complexity of operations, particularly move validation and game state evaluation, to ensure efficiency.__
-------------------------------------------------------
Move Validation Complexity:
    Approach: I used a straightforward method to check the validity of each move, even though it might not be the most efficient. This approach ensures simplicity and completeness, considering the manageable complexity due to the limited number of possible moves on the chessboard.

    Efficiency: Despite its simplicity, this method remains effective for validating moves. 

Game State Evaluation Complexity:
    Approach: Like said in previous sections I used a brute force method to check the state of my game for check and checkmate by iterating over each of the pieces. 

    Efficiency: The decision to use a brute-force approach for move generation simplifies implementation. While more sophisticated algorithms could improve efficiency, they also increase complexity significantly, which might make it much harder to debug and understand. 
    
    Additionally since there is only a maximum of 32 pieces on the board at a time, its not really such a big deal if the time compelxity is bad. 

========================================
**Other**
========================================
When you have any errors in input reading, it outputs UNKCMD

When you have the right starting command with a newline at the end but has extra characters at the end, it outputs INVFMT

========================================
**Sources**
========================================
https://embetronicx.com/tutorials/linux/device-drivers/misc-device-driver/

https://docs.kernel.org/5.10/driver-api/misc_devices.html

https://www.chess.com/learn-how-to-play-chess
