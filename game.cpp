#include <iostream>
#include <vector>
#include <conio.h>
#include <Windows.h>
#include <ctime>
#include <random>
#include <string>
#include <limits.h>
#include <fstream>

// Constants
const int GRID_WIDTH = 10;
const int GRID_HEIGHT = 20;
const int SCREEN_WIDTH = 80;
const int SCREEN_HEIGHT = 30;

// Colors for different tetromino pieces
const int COLOR_O = 14; // Yellow
const int COLOR_I = 11; // Light Cyan
const int COLOR_S = 12; // Light Red
const int COLOR_Z = 10; // Light Green
const int COLOR_L = 6;  // Brown/Orange
const int COLOR_J = 13; // Light Magenta
const int COLOR_T = 5;  // Purple

// Game state
enum class GameState {
    PLAYING,
    GAME_OVER,
    RESTART
};

// Tetromino shapes
class Tetromino {
public:
    enum class Type {
        O, I, S, Z, L, J, T
    };

    struct Position {
        int x;
        int y;
    };

    // Default constructor
    Tetromino() : type(Type::O) {
        initShape();
    }

    Tetromino(Type type) : type(type) {
        initShape();
    }

    void initShape() {
        switch (type) {
        case Type::O:
            // O shape (2x2 square)
            shape = {
                {0, 0}, {1, 0},
                {0, 1}, {1, 1}
            };
            color = COLOR_O;
            break;
        case Type::I:
            // I shape (vertical)
            shape = {
                {0, 0},
                {0, 1},
                {0, 2},
                {0, 3}
            };
            color = COLOR_I;
            break;
        case Type::S:
            // S shape
            shape = {
                {1, 0}, {2, 0},
                {0, 1}, {1, 1}
            };
            color = COLOR_S;
            break;
        case Type::Z:
            // Z shape
            shape = {
                {0, 0}, {1, 0},
                {1, 1}, {2, 1}
            };
            color = COLOR_Z;
            break;
        case Type::L:
            // L shape
            shape = {
                {0, 0},
                {0, 1},
                {0, 2}, {1, 2}
            };
            color = COLOR_L;
            break;
        case Type::J:
            // J shape
            shape = {
                        {1, 0},
                        {1, 1},
                {0, 2}, {1, 2}
            };
            color = COLOR_J;
            break;
        case Type::T:
            // T shape
            shape = {
                {0, 0}, {1, 0}, {2, 0},
                        {1, 1}
            };
            color = COLOR_T;
            break;
        }
    }

    void rotate() {
        // Skip rotation for O piece (square) since it looks the same
        if (type == Type::O) return;

        std::vector<Position> rotated;
        
        // Find center of rotation
        int minX = INT_MAX, maxX = INT_MIN;
        int minY = INT_MAX, maxY = INT_MIN;
        
        for (const auto& block : shape) {
            minX = std::min(minX, block.x);
            maxX = std::max(maxX, block.x);
            minY = std::min(minY, block.y);
            maxY = std::max(maxY, block.y);
        }
        
        int centerX = (minX + maxX) / 2;
        int centerY = (minY + maxY) / 2;
        
         // Special case for I piece (long bar) - it has an offset rotation center
    if (type == Type::I) {
        // For I piece, use a fixed rotation point 
        // This gives better results than calculating center
        for (const auto& block : shape) {
            int x = block.x - 1;  // Use 1 as the center X
            int y = block.y - 1;  // Use 1 as the center Y
            
            // (x, y) -> (y, -x) is a 90-degree clockwise rotation
            rotated.push_back({1 + y, 1 - x});
        }
    } else {
        // Rotate 90 degrees clockwise around the center
        for (const auto& block : shape) {
            int x = block.x - centerX;
            int y = block.y - centerY;
            
            // (x, y) -> (y, -x) is a 90-degree clockwise rotation
            rotated.push_back({centerX + y, centerY - x});
        }
    }
    
    // Update shape with rotated coordinates
    shape = rotated;
}

    std::vector<Position> getGlobalPositions(int offsetX, int offsetY) const {
        std::vector<Position> global;
        for (const auto& block : shape) {
            global.push_back({block.x + offsetX, block.y + offsetY});
        }
        return global;
    }

    Type type;
    std::vector<Position> shape;
    int color;
};

// The game class
class TetrisGame {
public:
    TetrisGame() : currentPiece(getRandomPiece()), nextPiece(getRandomPiece()) {
        // Initialize console
        initConsole();
        resetGame();
    }
   
    ~TetrisGame() {
        // Restore console settings
        SetConsoleTextAttribute(consoleHandle, 7); // Reset to default color
    }

    void run() {
        bool exitGame = false;
        
        while (!exitGame) {
            gameState = GameState::PLAYING;
            lastFallTime = GetTickCount();
            
            while (gameState == GameState::PLAYING) {
                if (_kbhit()) {
                    int key = _getch();
                    if (key == 27) { // ESC key
                        gameState = GameState::GAME_OVER;
                        exitGame = true;
                        break;
                    }
                    handleInput();
                }

                // Check if it's time for the piece to fall
                DWORD currentTime = GetTickCount();
                if (currentTime - lastFallTime >= fallSpeed) {
                    movePieceDown();
                    lastFallTime = currentTime;
                }

                render();
                Sleep(50);
            }

            if (gameState == GameState::GAME_OVER) {
                // Check if we have a new high score BEFORE saving it
                 bool isNewHighScore = (score > highScore);

                // Save high score
                saveHighScore();
                
              // Game over screen with restart option - pass the highscore flag
              renderGameOver(isNewHighScore);
    
                
                bool waitingForInput = true;
                while (waitingForInput) {
                    if (_kbhit()) {
                        int key = _getch();
                        if (key == 'r' || key == 'R') {
                            waitingForInput = false;
                            resetGame(); // Restart the game
                            system("cls"); // Clear screen before new game
                        }
                        else if (key == 27) { // ESC to exit
                            waitingForInput = false;
                            exitGame = true;
                        }
                        else if (key == 'q' || key == 'Q') { // Q to quit
                            waitingForInput = false;
                            exitGame = true;
                        }
                    }
                    Sleep(50);
                }
            }
        }
    }

private:
    // Game grid (0 = empty, other values = filled with color)
    std::vector<std::vector<int>> grid;
    
    // Current falling piece
    Tetromino currentPiece;
    int pieceX, pieceY;
    
    // Next piece to appear
    Tetromino nextPiece;
    
    // Game state variables
    int score;
    int level;
    int linesCleared;
    int highScore;
    GameState gameState;
    int fallSpeed;
    DWORD lastFallTime;
    
    // Console handle
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    void initConsole() {
        // Hide cursor
        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(consoleHandle, &cursorInfo);
        cursorInfo.bVisible = false;
        SetConsoleCursorInfo(consoleHandle, &cursorInfo);
        
        // Set text attributes to default
        SetConsoleTextAttribute(consoleHandle, 7);
        
        // Load high score
        loadHighScore();
    }
    void resetGame() {
        // Clear the grid
        grid = std::vector<std::vector<int>>(GRID_HEIGHT, std::vector<int>(GRID_WIDTH, 0));
        
        // Reset game variables
        currentPiece = getRandomPiece();
        nextPiece = getRandomPiece();
        pieceX = GRID_WIDTH / 2 - 1;
        pieceY = 0;
        score = 0;
        level = 1;
        linesCleared = 0;
        gameState = GameState::PLAYING;
        fallSpeed = 1000; // Initial falling speed in milliseconds
        lastFallTime = GetTickCount();
        
        // Initialize random number generator
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        
        // Clear screen
        system("cls");
    }

    Tetromino getRandomPiece() {
        // Create a random tetromino
        int randPiece = std::rand() % 7;
        return Tetromino(static_cast<Tetromino::Type>(randPiece));
    }

    void handleInput() {
        int key = _getch();
        
        switch (key) {
        case 224: // Special key prefix
           
            key = _getch(); // Get the actual key code
            switch (key) {
            case 75: // Left arrow
                movePieceLeft();
                break;
            case 77: // Right arrow
                movePieceRight();
                break;
            case 72: // Up arrow
                rotatePiece();
                break;
            case 80: // Down arrow
                movePieceDown(); // Soft drop
                break;
            }
        
            break;
            case 72: // Additional check for Up arrow without prefix
            rotatePiece();
            break;
        case 75: // Additional check for Left arrow without prefix
            movePieceLeft();
            break;
        case 77: // Additional check for Right arrow without prefix
            movePieceRight();
            break;
        case 80: // Additional check for Down arrow without prefix
            movePieceDown(); 
            break;
        
        case 32: // Spacebar - Hard drop
            hardDrop();
            break;
        case 27: // ESC key - Pause/Quit
            // Could implement pause menu here
            gameState = GameState::GAME_OVER;
            break;
        }
    }

    void movePieceLeft() {
        pieceX--;
        if (collision()) {
            pieceX++; // Move back if collision
        }
    }

    void movePieceRight() {
        pieceX++;
        if (collision()) {
            pieceX--; // Move back if collision
        }
    }

    void rotatePiece() {
        // Save the original shape in case we need to revert
        auto originalShape = currentPiece.shape;
        
        // Try to rotate
        currentPiece.rotate();
        
        // Check if the rotated piece collides
        if (collision()) {
            // Wall kick attempts - try to shift the piece to make the rotation work
            const int kicks[4] = {1, -1, 2, -2}; // Right, left, 2 right, 2 left
            bool kickSucceeded = false;
            
            for (int kick : kicks) {
                pieceX += kick;
                if (!collision()) {
                    kickSucceeded = true;
                    break;
                }
                pieceX -= kick; // Revert the kick
            }
            
            // If all kicks failed, revert the rotation
            if (!kickSucceeded) {
                currentPiece.shape = originalShape;
            }
        }
    }

    void movePieceDown() {
        pieceY++;
        if (collision()) {
            pieceY--; // Move back up if collision
            lockPiece(); // Lock the piece in place
            clearLines(); // Check and clear any full lines
            spawnNewPiece(); // Spawn a new piece
        }
    }

    void hardDrop() {
        // Keep moving down until collision
        while (!collision()) {
            pieceY++;
        }
        pieceY--; // Move back up from the collision position
        lockPiece(); // Lock the piece in place
        clearLines(); // Check and clear any full lines
        spawnNewPiece(); // Spawn a new piece
    }

    bool collision() {
        for (const auto& pos : currentPiece.getGlobalPositions(pieceX, pieceY)) {
            // Check boundaries
            if (pos.x < 0 || pos.x >= GRID_WIDTH || pos.y < 0 || pos.y >= GRID_HEIGHT) {
                return true;
            }
            
            // Check collision with locked pieces
            // Only check for collision with blocks if the position is within the grid
            if (pos.y >= 0 && pos.y < GRID_HEIGHT && pos.x >= 0 && pos.x < GRID_WIDTH) {
                if (grid[pos.y][pos.x] != 0) {
                return true;
            }
        }
    }
        return false;
}

    void lockPiece() {
        for (const auto& pos : currentPiece.getGlobalPositions(pieceX, pieceY)) {
            if (pos.y >= 0 && pos.y < GRID_HEIGHT && pos.x >= 0 && pos.x < GRID_WIDTH) {
                grid[pos.y][pos.x] = currentPiece.color;
            }
        }
    }

    void clearLines() {
        int linesCleared = 0;
        
        for (int y = GRID_HEIGHT - 1; y >= 0; --y) {
            bool lineFilled = true;
            
            // Check if this line is completely filled
            for (int x = 0; x < GRID_WIDTH; ++x) {
                if (grid[y][x] == 0) {
                    lineFilled = false;
                    break;
                }
            }
            
            if (lineFilled) {
                linesCleared++;
                
                // Move all lines above this one down
                for (int moveY = y; moveY > 0; --moveY) {
                    for (int x = 0; x < GRID_WIDTH; ++x) {
                        grid[moveY][x] = grid[moveY - 1][x];
                    }
                }
                
                // Clear the top line
                for (int x = 0; x < GRID_WIDTH; ++x) {
                    grid[0][x] = 0;
                }
                
                // Since we've moved all lines down, we need to check this y-index again
                y++;
            }
        }
        
        // Update score and level
        if (linesCleared > 0) {
            updateScoreAndLevel(linesCleared);
        }
    }

    void updateScoreAndLevel(int lines) {
        // Scoring system: more points for clearing multiple lines at once
        const int pointsForLines[4] = {40, 100, 300, 1200};
        
        if (lines > 0 && lines <= 4) {
            // Add points based on the number of lines and current level
            score += pointsForLines[lines - 1] * level;
        }
        
        // Update total lines cleared
        this->linesCleared += lines;
        
        // Every 5 lines, increase level
        level = 1 + (this->linesCleared / 5);
        
        // Increase speed as level increases
        fallSpeed = 1000 / level;
    }

    void spawnNewPiece() {
        // Set the current piece to the next piece
        currentPiece = nextPiece;
        
        // Generate a new next piece
        nextPiece = getRandomPiece();
        
        // Reset position
        pieceX = GRID_WIDTH / 2 - 1;
        pieceY = 0;
        
        // Check for game over
        if (collision()) {
            gameState = GameState::GAME_OVER;
        }
    }

    void render() {
        // Clear the screen
        COORD topLeft = {0, 0};
        SetConsoleCursorPosition(consoleHandle, topLeft);
                // Draw the border and grid
                drawBorder();
        
                // Draw the current piece
                drawCurrentPiece();
                
                // Draw next piece preview
                drawNextPiece();
                
                // Draw score and level information
                drawInfo();
                
                // Ensure cursor is in a safe position after all drawing
                COORD safePos = {0, GRID_HEIGHT + 3};
                SetConsoleCursorPosition(consoleHandle, safePos);
            }

    void drawBorder() {
        // Top border
        SetConsoleTextAttribute(consoleHandle, 7); // Reset color
        setCursorPosition(0, 0);
        std::cout << "*";
        for (int i = 0; i < GRID_WIDTH * 2; i++) {
            std::cout << "*";
        }
        std::cout << "*";
        
        // Side borders and grid
        for (int y = 0; y < GRID_HEIGHT; y++) {
            setCursorPosition(0, y + 1);
            std::cout << "*";
            
            for (int x = 0; x < GRID_WIDTH; x++) {
                // Draw cell based on grid value
                int cellValue = grid[y][x];
                if (cellValue != 0) {
                    SetConsoleTextAttribute(consoleHandle, cellValue);
                    std::cout <<char(219) << char(219); 
                } else {
                    SetConsoleTextAttribute(consoleHandle, 8); // Dark gray for empty cells
                    std::cout << "  ";
                }
            }
            
            SetConsoleTextAttribute(consoleHandle, 7); // Reset color
            std::cout << "*";
        }
        

        // Bottom border
    setCursorPosition(0, GRID_HEIGHT + 1);
    std::cout << "*";
    for (int i = 0; i < GRID_WIDTH * 2; i++) {
        std::cout << "*";
    }
    std::cout << "*";
}

void drawCurrentPiece() {
    SetConsoleTextAttribute(consoleHandle, currentPiece.color);
    
    for (const auto& pos : currentPiece.getGlobalPositions(pieceX, pieceY)) {
        if (pos.y >= 0 && pos.y < GRID_HEIGHT && pos.x >= 0 && pos.x < GRID_WIDTH) {
            setCursorPosition(pos.x * 2 + 1, pos.y + 1);
            std::cout <<char(219) << char(219); 
        }
    }
        // Important: Reset cursor position after drawing the piece
        SetConsoleCursorPosition(consoleHandle, {0, 0});
        SetConsoleTextAttribute(consoleHandle, 7); // Reset color
    
}

void drawNextPiece() {
      // Draw the next piece preview box
      int previewX = GRID_WIDTH * 2 + 5;
      int previewY = 3;
      
      SetConsoleTextAttribute(consoleHandle, 13);
      setCursorPosition(previewX, previewY - 2);
      std::cout << "Next Piece:";
      setCursorPosition(previewX, previewY);
      std::cout << "*";
      for (int y = 0; y < 4; y++) {
          setCursorPosition(previewX, previewY + y + 1);
          std::cout << "*           *";
      }
      setCursorPosition(previewX, previewY + 5);
      std::cout << "*";
      
      // Draw the next piece in the preview box
      SetConsoleTextAttribute(consoleHandle, nextPiece.color);
      
      // Center the piece in the preview box
      int centerX = previewX + 6; // Adjusted from 5 to 6
      int centerY = previewY + 3;
      
      // Special adjustment for the I piece
      int offsetX = 0;
      int offsetY = 0;
      
      if (nextPiece.type == Tetromino::Type::I) {
          // Check if I piece is vertical (has height > width)
          int minX = INT_MAX, maxX = INT_MIN;
          int minY = INT_MAX, maxY = INT_MIN;
          
          for (const auto& block : nextPiece.shape) {
              minX = std::min(minX, block.x);
              maxX = std::max(maxX, block.x);
              minY = std::min(minY, block.y);
              maxY = std::max(maxY, block.y);
          }
          
          int width = maxX - minX + 1;
          int height = maxY - minY + 1;
          
          if (height > width) { // Vertical I piece
              offsetY = -1; // Move it up a bit
          } else { // Horizontal I piece
              offsetX = -1; // Move it left a bit
          }
      }
      
      for (const auto& pos : nextPiece.shape) {
          setCursorPosition(centerX + pos.x * 2 - 3 + offsetX * 2, centerY + pos.y - 1 + offsetY);
          std::cout << char(219) << char(219); 
      }
      
      SetConsoleTextAttribute(consoleHandle, 7); // Reset color
}

void drawInfo() {
    int infoX = GRID_WIDTH * 2 + 5;
    int infoY = 10;
    
    SetConsoleTextAttribute(consoleHandle, 11);
    setCursorPosition(infoX, infoY);
    std::cout << "CURRENT SCORE: " << score;

    setCursorPosition(infoX, infoY + 1);
    std::cout << "HIGH SCORE: " << highScore;
    
    setCursorPosition(infoX, infoY + 2);
    std::cout << "LEVEL: " << level;
    
    setCursorPosition(infoX, infoY + 3);
    std::cout << "LINES: " << linesCleared;
    
    // Draw controls
    setCursorPosition(infoX, infoY + 5);
    SetConsoleTextAttribute(consoleHandle, 2);
    std::cout << "CONTROLS:---";
    std::cout << "\n";
    setCursorPosition(infoX, infoY + 6);
    SetConsoleTextAttribute(consoleHandle, 6);
    std::cout << "<-- / -->  :- ";
    SetConsoleTextAttribute(consoleHandle, 12); 
    std::cout << "To Move Left and Right";
    setCursorPosition(infoX, infoY + 7);
    SetConsoleTextAttribute(consoleHandle, 6);
    std::cout << "Up Key :- ";
    SetConsoleTextAttribute(consoleHandle, 12);
    std::cout << "To Rotate";
    setCursorPosition(infoX, infoY + 8);
    SetConsoleTextAttribute(consoleHandle, 6);
    std::cout << "Down Key :- ";
    SetConsoleTextAttribute(consoleHandle, 12);
    std::cout<< "Soft Drop";
    setCursorPosition(infoX, infoY + 9);
    SetConsoleTextAttribute(consoleHandle, 6);
    std::cout << "Double-Spacebar Key :- ";
    SetConsoleTextAttribute(consoleHandle, 12);
    std::cout << "To Hard Drop";
    setCursorPosition(infoX, infoY + 10);
    SetConsoleTextAttribute(consoleHandle, 6);
    std::cout << "Spacebar Key :- ";
    SetConsoleTextAttribute(consoleHandle, 12);
    std::cout << "To Pause the Game";
    setCursorPosition(infoX, infoY + 11);
    SetConsoleTextAttribute(consoleHandle, 6);
    std::cout << "ESC Key/Press 'Q/q':- ";
    SetConsoleTextAttribute(consoleHandle, 12);
    std::cout << "To Quit";
}

void renderGameOver(bool isNewHighScore) {
     // Position game over messages at the bottom of the grid
     int messageX = 1;  // Starting from the left edge of the grid
     int messageY = GRID_HEIGHT + 2;  // Just below the grid's bottom border
     
     // Game Over message
     setCursorPosition(messageX, messageY);
     SetConsoleTextAttribute(consoleHandle, 12); // Light red
     std::cout << "Game Over!";
     
     // Final Score message
     setCursorPosition(messageX, messageY + 1);
     SetConsoleTextAttribute(consoleHandle, 13); // Reset color
     std::cout << "Final Score: " << score;
     
     // High Score message
     setCursorPosition(messageX, messageY + 2);
     SetConsoleTextAttribute(consoleHandle, 14); // Yellow
     if (isNewHighScore) {
         std::cout << "NEW HIGH SCORE!";
     } else {
         std::cout << "High Score: " << highScore;
     }
     std::cout << "\n\n";
    // Restart option
    setCursorPosition(messageX, messageY + 3);
    SetConsoleTextAttribute(consoleHandle, 10); // Light green
    std::cout << "Press 'R' to restart\n";
    SetConsoleTextAttribute(consoleHandle, 12);//red
    std::cout << "'ESC' / 'Q' to quit...";
    }

   void setCursorPosition(int x, int y) {
    COORD coord = {static_cast<SHORT>(x), static_cast<SHORT>(y)};
    SetConsoleCursorPosition(consoleHandle, coord);
   }

   void saveHighScore() {
    // Update high score if current score is higher
    if (score > highScore) {
        highScore = score;
        
        // Save to file
        std::ofstream file("tetris_highscore.txt");
        if (file.is_open()) {
            file << highScore;
            file.close();
        }
    }
  }

  void loadHighScore() {
    // Default high score is 0
    highScore = 0;
    
    // Try to load from file
    std::ifstream file("tetris_highscore.txt");
    if (file.is_open()) {
        file >> highScore;
        file.close();
      }
    }
};
int main() {
    // Set console handle for the title screen
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    // Clear screen first
    system("cls");
    
    // Define a solid block character
    char block = 219; // ASCII code for solid block
    
    // Get console screen buffer info to determine window width
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(consoleHandle, &csbi);
    int consoleWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    
    // Calculate width of the TETRIS text (each letter is 5*2 chars wide with 2 spaces between)
    const int tetrisWidth = (5 * 2 * 6) + (5 * 2); // 6 letters, each 5*2 wide, with 2 spaces between
    int leftPadding = (consoleWidth - tetrisWidth) / 2;
    if (leftPadding < 0) leftPadding = 0;
    
    // Define the letters
    int R[5][5] = {
        {1,1,1,0,0},
        {1,0,0,1,0},
        {1,1,1,0,0},
        {1,0,1,0,0},
        {1,0,0,1,0}
    };
    
    int S[5][5] = {
        {0,1,1,1,0},
        {1,0,0,0,0},
        {0,1,1,0,0},
        {0,0,0,1,0},
        {1,1,1,0,0}
    };
    
    int T[5][5] = {
        {1,1,1,1,1},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0}
    };
    
    int E[5][5] = {
        {1,1,1,1,1},
        {1,0,0,0,0},
        {1,1,1,0,0},
        {1,0,0,0,0},
        {1,1,1,1,1}
    };
    
    int I[5][5] = {
        {0,1,1,0,0},
        {0,1,1,0,0},
        {0,1,1,0,0},
        {0,1,1,0,0},
        {0,1,1,0,0}
    };
    
    // Print the TETRIS title
    SetConsoleTextAttribute(consoleHandle, 1); // Dark Blue
    std::cout << "\n\n";
    
    // Print each row of the letters
    for (int row = 0; row < 5; row++) {
        // Print left padding to center the text
        std::string padding(leftPadding, ' ');
        std::cout << padding;
        
        // Print T
        for (int col = 0; col < 5; col++) {
            if (T[row][col]) std::cout << block << block;
            else std::cout << "  ";
        }
        std::cout << "  ";
        
        // Print E
        for (int col = 0; col < 5; col++) {
            if (E[row][col]) std::cout << block << block;
            else std::cout << "  ";
        }
        std::cout << "  ";
        
        // Print T
        for (int col = 0; col < 5; col++) {
            if (T[row][col]) std::cout << block << block;
            else std::cout << "  ";
        }
        std::cout << "  ";
        
        // Print R
        for (int col = 0; col < 5; col++) {
            if (R[row][col]) std::cout << block << block;
            else std::cout << "  ";
        }
        std::cout << "  ";
        
        // Print I
        for (int col = 0; col < 5; col++) {
            if (I[row][col]) std::cout << block << block;
            else std::cout << "  ";
        }
        std::cout << "  ";
        
        // Print S
        for (int col = 0; col < 5; col++) {
            if (S[row][col]) std::cout << block << block;
            else std::cout << "  ";
        }
        std::cout << "\n";
    }
    
    std::cout << "\n\n";
    
    // Calculate width of instruction box
    const int instructionBoxWidth = 44; // Based on your longest line of text
    leftPadding = (consoleWidth - instructionBoxWidth) / 2;
    if (leftPadding < 0) leftPadding = 0;
    
    std::string instructionPadding(leftPadding, ' ');
    
    // Border for instructions
    SetConsoleTextAttribute(consoleHandle, 13); // Bright Pink/Magenta
    std::cout << instructionPadding << "+------------------------------------------+\n";
    std::cout << instructionPadding << "|              HOW TO PLAY   let's play the game              |\n";
    std::cout << instructionPadding << "+------------------------------------------+\n\n";
    SetConsoleTextAttribute(consoleHandle, 2);
    std::cout << instructionPadding << "Controls:\n";
    SetConsoleTextAttribute(consoleHandle, 11);
    std::cout << instructionPadding << "  Left/Right Arrow: ";
    SetConsoleTextAttribute(consoleHandle, 12);
    std::cout<<"Move piece\n";
    SetConsoleTextAttribute(consoleHandle, 11);
    std::cout << instructionPadding << "  Up Arrow: ";
    SetConsoleTextAttribute(consoleHandle, 12);
    std::cout<<"Rotate piece\n";
    SetConsoleTextAttribute(consoleHandle, 11);
    std::cout << instructionPadding << "  Down Arrow: ";
    SetConsoleTextAttribute(consoleHandle, 12);
    std::cout << "Soft drop\n";
    SetConsoleTextAttribute(consoleHandle, 11);
    std::cout << instructionPadding << "  Double-Spacebar key: ";
    SetConsoleTextAttribute(consoleHandle, 12);
    std::cout << "Hard drop\n";
    SetConsoleTextAttribute(consoleHandle, 11);
    std::cout << instructionPadding << "  Spacebar key: ";
    SetConsoleTextAttribute(consoleHandle, 12);
    std::cout << "To Pause the Game\n";
    SetConsoleTextAttribute(consoleHandle, 11);
    std::cout << instructionPadding << "  ESC: ";
    SetConsoleTextAttribute(consoleHandle, 12);
    std::cout << "Quit game\n";
    std::cout << "\n";
    SetConsoleTextAttribute(consoleHandle, 13);
    std::cout << instructionPadding <<"LEVEL WILL INCREASE AFTER EVERY 5 LINES.....";

    SetConsoleTextAttribute(consoleHandle, 2);
    std::cout << "\n";std::cout << "\n";
    std::cout << instructionPadding << "Press any key to start...\n";
    
    _getch(); // Wait for a key press to start
    
    TetrisGame game;
    game.run();
    
    return 0;
}