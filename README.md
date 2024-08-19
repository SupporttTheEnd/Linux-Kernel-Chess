# Chess Game

## **How To Use**

1. Navigate to the `project3/chess` directory and execute the following commands:
   ```bash
   make
   make u
   make l
   ```
   These commands will create the driver, unload the driver module, and then load the module.

2. Next, go to the `project3/chess_driver` directory and execute:
   ```bash
   make
   make run
   ```
   You can then directly enter commands. The system will echo the command and display the output. Type "exit" to exit.

## **Data Structures**

### **Chessboard Representation**

The chessboard is represented using a 2D array named `game_board`. Each element represents a square on the chessboard and holds a string indicating the piece occupying that square. Each square is three bytes: one for color, one for piece type, and one for the null terminator. This simplifies accessing specific squares and avoids dynamically allocated memory, enhancing speed due to sequential access.

### **Piece Encoding**

Pieces are encoded with a two-character string in the `game_board` array:
- **Color**: 'W' for white and 'B' for black
- **Piece Type**:
  - Pawn: "P"
  - Knight: "N"
  - Bishop: "B"
  - Rook: "R"
  - Queen: "Q"
  - King: "K"

For example:
- 'WP' denotes a white pawn
- 'BN' denotes a black knight

### **Game State Management**

- `game_board` represents the chessboard's current configuration.
- `game_started` indicates whether the game is active.
- `player_turn` tracks whose turn it is.
- `output_message` holds information for display.
- Move history is implicitly managed by updating `game_board` with each move.

## **Move Validation**

### **General Approach**

1. Validate the move string length (7, 10, or 13 characters).
2. Obtain source and destination coordinates from the move string.
3. Verify piece color and coordinate bounds.
4. Confirm the presence of a piece at the source square.

### **Piece-Specific Validations**

- **Pawn ('P')**: Validate moves, including initial two-square advance, captures, and promotion.
- **Knight ('N')**: Verify L-shaped moves.
- **Bishop ('B')**: Validate diagonal moves.
- **Rook ('R')**: Validate horizontal and vertical moves.
- **Queen ('Q')**: Validate horizontal, vertical, and diagonal moves.
- **King ('K')**: Validate one-square moves in any direction.

### **Collision Detection**

- **Pawns, Bishops, Rooks, Queens**: Check for obstructions in the path using direction-specific checks.
- **Knights and Kings**: Do not need obstacle detection.

Captures are handled by checking if the destination square contains a piece of the opposite color. Special cases like Pawn promotion are also managed.

## **Check and Checkmate Detection**

### **Check Detection**

1. Locate the opponent's king on the board.
2. Check if any piece of the current player can legally capture the opponent's king by simulating potential moves.
3. If a valid capture move exists, the opponent's king is in check.

### **Checkmate Detection**

1. Verify if the opponent's king is in check.
2. Generate all possible legal moves for the opponent's pieces.
3. Simulate each move to see if it gets the king out of check.
4. If no move can get the king out of check, it is checkmate.

## **CPU Move Determination**

### **Strategy Implementation**

The CPU first searches for capturing moves. If none are available, it selects a non-capturing move from valid options.

### **Difficulty Levels**

Currently, difficulty levels are not implemented.

### **Extra Features**

- The AI prioritizes captures over non-capturing moves, employing a greedy algorithm for decent gameplay.
- The AI tries to escape check states by moving pieces to protect the king or moving the king itself.

## **Impact on Gameplay**

These features ensure a challenging opponent, enhancing the user experience by providing a more engaging game.

## **Design Rationale**

### **Decision Justifications**

- **2D Array for Game Board**: Provides a natural and intuitive representation of the chessboard.
- **Brute-Force Move Generation**: Simplifies implementation and debugging, suitable for the limited number of pieces on the board.

### **Complexity Considerations**

- **Move Validation**: A straightforward approach ensures simplicity and completeness.
- **Game State Evaluation**: Brute-force methods are manageable given the number of pieces and ensure accurate results.

## **Error Handling**

- **Unknown Command**: Outputs `UNKCMD` for unrecognized commands.
- **Invalid Format**: Outputs `INVFMT` for commands with extra characters after the newline.

## **Sources**

- [Embetronicx - Misc Device Driver Tutorial](https://embetronicx.com/tutorials/linux/device-drivers/misc-device-driver/)
- [Linux Kernel Documentation - Misc Devices](https://docs.kernel.org/5.10/driver-api/misc_devices.html)
- [Chess.com - Learn How to Play Chess](https://www.chess.com/learn-how-to-play-chess)

---

Feel free to adjust any details or add any additional information as needed!
