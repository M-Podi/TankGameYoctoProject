#include <ncurses.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <locale.h>

// Constants
#define MAX_TANKS 2
#define MAX_PROJECTILES 100
#define BOARD_HEIGHT 20
#define BOARD_WIDTH 60
#define WALL_CHAR '#'
#define PROJECTILE_CHAR '.'
#define HEART_STR "<3"  // Using text emoticon heart
#define MAX_HEALTH 10
#define PROJECTILE_SPEED 100000 // microseconds

// Color pairs
#define COLOR_PLAYER1 1
#define COLOR_PLAYER2 2
#define COLOR_WALL 3
#define COLOR_PROJECTILE 4
#define COLOR_HEART 5

// Direction enum
typedef enum {
    UP, DOWN, LEFT, RIGHT
} Direction;

// Tank structure
typedef struct {
    char symbol;
    int x, y;
    int health;
    Direction dir;
    pthread_t thread;
    char up_key, down_key, left_key, right_key, fire_key;
} Tank;

// Projectile structure
typedef struct {
    int x, y;
    Direction dir;
    pthread_t thread;
    int active;
    char owner; // Tank symbol that fired this projectile
} Projectile;

// Game state structure
typedef struct {
    char board[BOARD_HEIGHT][BOARD_WIDTH];
    Tank tanks[MAX_TANKS];
    Projectile projectiles[MAX_PROJECTILES];
    int num_projectiles;
    int game_over;
    int winner; // Index of winning tank (-1 if no winner yet)
    pthread_mutex_t board_mutex;
} GameState;

// Global game state
GameState game;

// Function to initialize the board with walls
void init_board() {
    pthread_mutex_lock(&game.board_mutex);
    
    // Clear the board
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (y == 0 || y == BOARD_HEIGHT - 1 || x == 0 || x == BOARD_WIDTH - 1) {
                game.board[y][x] = WALL_CHAR; // Border walls
            } else {
                game.board[y][x] = ' '; // Empty space
            }
        }
    }
    
    // Add some random walls (simple maze)
    srand(time(NULL));
    for (int i = 0; i < (BOARD_HEIGHT * BOARD_WIDTH) / 20; i++) {
        int x = rand() % (BOARD_WIDTH - 2) + 1;
        int y = rand() % (BOARD_HEIGHT - 2) + 1;
        
        // Avoid placing walls near tank starting positions
        if ((x < 5 && y < 5) || (x > BOARD_WIDTH - 6 && y > BOARD_HEIGHT - 6)) {
            continue;
        }
        
        game.board[y][x] = WALL_CHAR;
    }
    
    pthread_mutex_unlock(&game.board_mutex);
}

// Function to place tanks on the board
void place_tanks() {
    pthread_mutex_lock(&game.board_mutex);
    
    // Place tank 1 in the top-left corner
    game.tanks[0].x = 2;
    game.tanks[0].y = 2;
    game.board[game.tanks[0].y][game.tanks[0].x] = game.tanks[0].symbol;
    
    // Place tank 2 in the bottom-right corner
    game.tanks[1].x = BOARD_WIDTH - 3;
    game.tanks[1].y = BOARD_HEIGHT - 3;
    game.board[game.tanks[1].y][game.tanks[1].x] = game.tanks[1].symbol;
    
    pthread_mutex_unlock(&game.board_mutex);
}

// Function to render the game
void render_game() {
    clear();
    
    pthread_mutex_lock(&game.board_mutex);
    
    // Display the board
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            char cell = game.board[y][x];
            
            // Set appropriate colors based on cell content
            if (cell == game.tanks[0].symbol) {
                attron(COLOR_PAIR(COLOR_PLAYER1));
                mvaddch(y, x, cell);
                attroff(COLOR_PAIR(COLOR_PLAYER1));
            } 
            else if (cell == game.tanks[1].symbol) {
                attron(COLOR_PAIR(COLOR_PLAYER2));
                mvaddch(y, x, cell);
                attroff(COLOR_PAIR(COLOR_PLAYER2));
            }
            else if (cell == WALL_CHAR) {
                attron(COLOR_PAIR(COLOR_WALL));
                mvaddch(y, x, cell);
                attroff(COLOR_PAIR(COLOR_WALL));
            }
            else if (cell == PROJECTILE_CHAR) {
                attron(COLOR_PAIR(COLOR_PROJECTILE));
                mvaddch(y, x, cell);
                attroff(COLOR_PAIR(COLOR_PROJECTILE));
            }
            else {
                mvaddch(y, x, cell);
            }
        }
    }
    
    // Display player health as hearts
    mvprintw(0, BOARD_WIDTH + 2, "Player %c: ", game.tanks[0].symbol);
    attron(COLOR_PAIR(COLOR_HEART));
    for (int i = 0; i < game.tanks[0].health; i++) {
        mvaddstr(0, BOARD_WIDTH + 11 + i*2, HEART_STR);  // Multiplying by 2 to account for the width of "<3"
    }
    attroff(COLOR_PAIR(COLOR_HEART));
    
    mvprintw(1, BOARD_WIDTH + 2, "Player %c: ", game.tanks[1].symbol);
    attron(COLOR_PAIR(COLOR_HEART));
    for (int i = 0; i < game.tanks[1].health; i++) {
        mvaddstr(1, BOARD_WIDTH + 11 + i*2, HEART_STR);  // Multiplying by 2 to account for the width of "<3"
    }
    attroff(COLOR_PAIR(COLOR_HEART));
    
    // Display controls
    attron(COLOR_PAIR(COLOR_PLAYER1));
    mvprintw(3, BOARD_WIDTH + 2, "Player %c Controls:", game.tanks[0].symbol);
    attroff(COLOR_PAIR(COLOR_PLAYER1));
    mvprintw(4, BOARD_WIDTH + 2, "  Up: %c", game.tanks[0].up_key);
    mvprintw(5, BOARD_WIDTH + 2, "  Down: %c", game.tanks[0].down_key);
    mvprintw(6, BOARD_WIDTH + 2, "  Left: %c", game.tanks[0].left_key);
    mvprintw(7, BOARD_WIDTH + 2, "  Right: %c", game.tanks[0].right_key);
    mvprintw(8, BOARD_WIDTH + 2, "  Fire: %c", game.tanks[0].fire_key);
    
    attron(COLOR_PAIR(COLOR_PLAYER2));
    mvprintw(10, BOARD_WIDTH + 2, "Player %c Controls:", game.tanks[1].symbol);
    attroff(COLOR_PAIR(COLOR_PLAYER2));
    mvprintw(11, BOARD_WIDTH + 2, "  Up: %c", game.tanks[1].up_key);
    mvprintw(12, BOARD_WIDTH + 2, "  Down: %c", game.tanks[1].down_key);
    mvprintw(13, BOARD_WIDTH + 2, "  Left: %c", game.tanks[1].left_key);
    mvprintw(14, BOARD_WIDTH + 2, "  Right: %c", game.tanks[1].right_key);
    mvprintw(15, BOARD_WIDTH + 2, "  Fire: %c", game.tanks[1].fire_key);
    
    mvprintw(17, BOARD_WIDTH + 2, "Press 'q' to quit");
    
    pthread_mutex_unlock(&game.board_mutex);
    
    refresh();
}

// Function to check if a position is valid for movement
int is_valid_position(int x, int y) {
    if (x < 0 || x >= BOARD_WIDTH || y < 0 || y >= BOARD_HEIGHT) {
        return 0; // Out of bounds
    }
    
    pthread_mutex_lock(&game.board_mutex);
    char cell = game.board[y][x];
    pthread_mutex_unlock(&game.board_mutex);
    
    if (cell == WALL_CHAR || (cell != ' ' && cell != PROJECTILE_CHAR)) {
        return 0; // Wall or another tank
    }
    
    return 1; // Valid position
}

// Function for tank thread
void* tank_thread(void* arg) {
    Tank* tank = (Tank*)arg;
    
    while (!game.game_over) {
        // The tank waits for input in the main loop
        usleep(100000); // Sleep to prevent high CPU usage
    }
    
    return NULL;
}

// Function to move a tank
void move_tank(Tank* tank, Direction dir) {
    // Always update the direction for aiming
    tank->dir = dir;
    
    int new_x = tank->x;
    int new_y = tank->y;
    
    // Calculate new position based on direction
    switch (dir) {
        case UP:
            new_y--;
            break;
        case DOWN:
            new_y++;
            break;
        case LEFT:
            new_x--;
            break;
        case RIGHT:
            new_x++;
            break;
    }
    
    // Check if the new position is valid
    if (is_valid_position(new_x, new_y)) {
        pthread_mutex_lock(&game.board_mutex);
        
        // Clear old position
        game.board[tank->y][tank->x] = ' ';
        
        // Update tank position
        tank->x = new_x;
        tank->y = new_y;
        
        // Place tank at new position
        game.board[tank->y][tank->x] = tank->symbol;
        
        pthread_mutex_unlock(&game.board_mutex);
    }
}

// Function for projectile thread
void* projectile_thread(void* arg) {
    Projectile* proj = (Projectile*)arg;
    
    while (proj->active && !game.game_over) {
        // Calculate new position based on direction
        int new_x = proj->x;
        int new_y = proj->y;
        
        switch (proj->dir) {
            case UP:
                new_y--;
                break;
            case DOWN:
                new_y++;
                break;
            case LEFT:
                new_x--;
                break;
            case RIGHT:
                new_x++;
                break;
        }
        
        pthread_mutex_lock(&game.board_mutex);
        
        // Check if new position is valid
        if (new_x < 0 || new_x >= BOARD_WIDTH || new_y < 0 || new_y >= BOARD_HEIGHT) {
            // Hit the board edge
            if (game.board[proj->y][proj->x] == PROJECTILE_CHAR) {
                game.board[proj->y][proj->x] = ' '; // Clear old position
            }
            proj->active = 0;
            pthread_mutex_unlock(&game.board_mutex);
            break;
        }
        
        char target = game.board[new_y][new_x];
        
        if (target == WALL_CHAR) {
            // Hit a wall
            if (game.board[proj->y][proj->x] == PROJECTILE_CHAR) {
                game.board[proj->y][proj->x] = ' '; // Clear old position
            }
            proj->active = 0;
            pthread_mutex_unlock(&game.board_mutex);
            break;
        } else if (target != ' ' && target != PROJECTILE_CHAR) {
            // Hit something else (likely a tank)
            for (int i = 0; i < MAX_TANKS; i++) {
                if (game.tanks[i].symbol == target && proj->owner != target) {
                    // Reduce tank health
                    game.tanks[i].health--;
                    if (game.tanks[i].health <= 0) {
                        game.game_over = 1;
                        // Set the winner to the other player
                        game.winner = (i == 0) ? 1 : 0;
                    }
                }
            }
            
            if (game.board[proj->y][proj->x] == PROJECTILE_CHAR) {
                game.board[proj->y][proj->x] = ' '; // Clear old position
            }
            proj->active = 0;
            pthread_mutex_unlock(&game.board_mutex);
            break;
        } else {
            // Move the projectile
            if (game.board[proj->y][proj->x] == PROJECTILE_CHAR) {
                game.board[proj->y][proj->x] = ' '; // Clear old position
            }
            
            proj->x = new_x;
            proj->y = new_y;
            game.board[proj->y][proj->x] = PROJECTILE_CHAR; // Place at new position
        }
        
        pthread_mutex_unlock(&game.board_mutex);
        
        // Sleep to control projectile speed
        usleep(PROJECTILE_SPEED);
    }
    
    return NULL;
}

// Function to fire a projectile
void fire_projectile(Tank* tank) {
    if (game.num_projectiles >= MAX_PROJECTILES) {
        return; // Too many projectiles
    }
    
    pthread_mutex_lock(&game.board_mutex);
    
    // Find the starting position for the projectile
    int proj_x = tank->x;
    int proj_y = tank->y;
    
    // Move the projectile one step in the tank's direction to prevent hitting the tank
    switch (tank->dir) {
        case UP:
            proj_y--;
            break;
        case DOWN:
            proj_y++;
            break;
        case LEFT:
            proj_x--;
            break;
        case RIGHT:
            proj_x++;
            break;
    }
    
    // Check if the position is valid
    if (proj_x < 0 || proj_x >= BOARD_WIDTH || proj_y < 0 || proj_y >= BOARD_HEIGHT ||
        game.board[proj_y][proj_x] == WALL_CHAR || 
        (game.board[proj_y][proj_x] != ' ' && game.board[proj_y][proj_x] != PROJECTILE_CHAR)) {
        pthread_mutex_unlock(&game.board_mutex);
        return; // Can't fire
    }
    
    // Create a new projectile
    Projectile* proj = &game.projectiles[game.num_projectiles++];
    proj->x = proj_x;
    proj->y = proj_y;
    proj->dir = tank->dir;
    proj->active = 1;
    proj->owner = tank->symbol;
    
    // Place projectile on board
    game.board[proj->y][proj->x] = PROJECTILE_CHAR;
    
    pthread_mutex_unlock(&game.board_mutex);
    
    // Create a thread for the projectile (now we don't detach it)
    pthread_create(&proj->thread, NULL, projectile_thread, proj);
}

// Function to handle keyboard input
void handle_input(int ch) {
    for (int i = 0; i < MAX_TANKS; i++) {
        Tank* tank = &game.tanks[i];
        
        if (ch == tank->up_key) {
            move_tank(tank, UP);
        } else if (ch == tank->down_key) {
            move_tank(tank, DOWN);
        } else if (ch == tank->left_key) {
            move_tank(tank, LEFT);
        } else if (ch == tank->right_key) {
            move_tank(tank, RIGHT);
        } else if (ch == tank->fire_key) {
            fire_projectile(tank);
        }
    }
}

// Function to reset game state for a new game
void reset_game() {
    // Reset game state variables
    game.num_projectiles = 0;
    game.game_over = 0;
    game.winner = -1;
    
    // Reset tank health (positions will be reset by place_tanks)
    game.tanks[0].health = MAX_HEALTH;
    game.tanks[1].health = MAX_HEALTH;
    
    // Re-initialize board and place tanks
    init_board();
    place_tanks();
}

// Function to display the main menu and get player settings
void display_menu() {
    clear();
    
    // Display title
    attron(A_BOLD);
    mvprintw(2, BOARD_WIDTH / 2 - 5, "TANK GAME");
    attroff(A_BOLD);
    
    // Get player 1 character with input validation
    mvprintw(5, 2, "Introdu caracterul pentru Jucatorul 1: ");
    refresh();
    
    timeout(-1); // Blocking input
    echo(); // Enable echo for input
    char p1_char;
    while (1) {
        p1_char = getch();
        if (p1_char != '\n' && p1_char != '\r' && p1_char != ' ' && p1_char != WALL_CHAR && p1_char != PROJECTILE_CHAR) {
            break; // Valid character
        }
        mvprintw(5, 40, "Invalid! Incearca din nou: ");
        clrtoeol();
        refresh();
    }
    noecho(); // Disable echo
    mvprintw(5, 40, "\n%c", p1_char);
    
    // Get player 2 character with input validation
    mvprintw(7, 2, "Introdu caracterul pentru Jucatorul 2: ");
    refresh();
    
    echo(); // Enable echo for input
    char p2_char;
    while (1) {
        p2_char = getch();
        if (p2_char != '\n' && p2_char != '\r' && p2_char != ' ' && 
            p2_char != WALL_CHAR && p2_char != PROJECTILE_CHAR && p2_char != p1_char) {
            break; // Valid character and different from player 1
        }
        mvprintw(7, 40, "Invalid! Incearca din nou: ");
        clrtoeol();
        refresh();
    }
    noecho(); // Disable echo
    mvprintw(7, 40, "\n%c", p2_char);
    
    // Set default controls
    game.tanks[0].symbol = p1_char;
    game.tanks[0].up_key = 'w';
    game.tanks[0].down_key = 's';
    game.tanks[0].left_key = 'a';
    game.tanks[0].right_key = 'd';
    game.tanks[0].fire_key = ' '; // Space
    game.tanks[0].health = MAX_HEALTH;
    game.tanks[0].dir = RIGHT;
    
    game.tanks[1].symbol = p2_char;
    game.tanks[1].up_key = 'i';
    game.tanks[1].down_key = 'k';
    game.tanks[1].left_key = 'j';
    game.tanks[1].right_key = 'l';
    game.tanks[1].fire_key = 'm';
    game.tanks[1].health = MAX_HEALTH;
    game.tanks[1].dir = LEFT;
    
    // Display controls
    attron(COLOR_PAIR(COLOR_PLAYER1));
    mvprintw(10, 2, "Controale Jucator 1 (%c):", p1_char);
    attroff(COLOR_PAIR(COLOR_PLAYER1));
    mvprintw(11, 4, "W (sus), S (jos), A (stanga), D (dreapta), SPACE (foc)");
    
    attron(COLOR_PAIR(COLOR_PLAYER2));
    mvprintw(13, 2, "Controale Jucator 2 (%c):", p2_char);
    attroff(COLOR_PAIR(COLOR_PLAYER2));
    mvprintw(14, 4, "I (sus), K (jos), J (stanga), L (dreapta), M (foc)");
    
    // Prompt to start game
    mvprintw(17, 2, "Apasa ENTER pentru a incepe jocul sau Q pentru a iesi");
    refresh();
    
    // Wait for key
    int ch;
    while (1) {
        ch = getch();
        if (ch == 'q' || ch == 'Q') {
            game.game_over = 1; // Set to exit program
            return;
        }
        if (ch == '\n' || ch == KEY_ENTER || ch == '\r') {
            return; // Return to start game
        }
    }
}

int main() {
    // Initialize game state
    memset(&game, 0, sizeof(GameState));
    pthread_mutex_init(&game.board_mutex, NULL);
    
    // Initialize ncurses
    setlocale(LC_ALL, ""); // Set locale for UTF-8
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0); // Hide cursor
    
    // Set up colors if terminal supports them
    if (has_colors()) {
        start_color();
        
        // Define color pairs
        init_pair(COLOR_PLAYER1, COLOR_RED, COLOR_BLACK);       // Player 1 is red
        init_pair(COLOR_PLAYER2, COLOR_BLUE, COLOR_BLACK);      // Player 2 is blue
        init_pair(COLOR_WALL, COLOR_WHITE, COLOR_BLACK);        // Walls are white
        init_pair(COLOR_PROJECTILE, COLOR_YELLOW, COLOR_BLACK); // Projectiles are yellow
        init_pair(COLOR_HEART, COLOR_RED, COLOR_BLACK);         // Hearts are red
    }
    
    int quit_program = 0;
    
    while (!quit_program) {
        // Reset game state for a new session
        game.game_over = 0;
        game.winner = -1;
        game.num_projectiles = 0;
        
        // Display menu and get player settings
        display_menu();
        
        if (game.game_over) {
            quit_program = 1;
            break; // User quit from menu
        }
        
        // Reset game for a new round
        reset_game();
        
        // Create tank threads
        for (int i = 0; i < MAX_TANKS; i++) {
            pthread_create(&game.tanks[i].thread, NULL, tank_thread, &game.tanks[i]);
        }
        
        // Set non-blocking getch for the game
        timeout(100);
        
        // Game loop
        while (!game.game_over) {
            // Handle input
            int ch = getch();
            if (ch != ERR) {
                if (ch == 'q') {
                    game.game_over = 1; // Exit game loop
                    quit_program = 1;  // Exit program
                    break;
                }
                handle_input(ch);
            }
            
            // Clean up inactive projectiles
            pthread_mutex_lock(&game.board_mutex);
            for (int i = 0; i < game.num_projectiles; i++) {
                if (!game.projectiles[i].active) {
                    // Join the thread
                    pthread_join(game.projectiles[i].thread, NULL);
                    
                    // Remove this projectile by swapping with the last one
                    game.projectiles[i] = game.projectiles[game.num_projectiles - 1];
                    game.num_projectiles--;
                    i--; // Recheck this index again
                }
            }
            pthread_mutex_unlock(&game.board_mutex);
            
            // Render game
            render_game();
            
            // Sleep to control game speed
            usleep(50000);
        }
        
        // Display game over message
        clear();
        if (game.winner >= 0 && game.winner < MAX_TANKS && !quit_program) {
            int color = (game.winner == 0) ? COLOR_PLAYER1 : COLOR_PLAYER2;
            attron(COLOR_PAIR(color) | A_BOLD);
            mvprintw(BOARD_HEIGHT / 2, (BOARD_WIDTH - 25) / 2, "Jucatorul %c a castigat!", game.tanks[game.winner].symbol);
            attroff(COLOR_PAIR(color) | A_BOLD);
        } else if (!quit_program) {
            mvprintw(BOARD_HEIGHT / 2, (BOARD_WIDTH - 10) / 2, "Joc incheiat!");
        }
        
        if (!quit_program) {
            mvprintw(BOARD_HEIGHT / 2 + 2, (BOARD_WIDTH - 40) / 2, "Apasa orice tasta pentru a reveni la meniu...");
            refresh();
            
            // Wait for key press (blocking)
            timeout(-1);
            getch();
        }
        
        // Wait for tank threads to finish
        for (int i = 0; i < MAX_TANKS; i++) {
            pthread_cancel(game.tanks[i].thread); // Cancel thread in case it's stuck
            pthread_join(game.tanks[i].thread, NULL);
        }
        
        // Cancel and join any active projectile threads
        for (int i = 0; i < game.num_projectiles; i++) {
            if (game.projectiles[i].active) {
                pthread_cancel(game.projectiles[i].thread);
                pthread_join(game.projectiles[i].thread, NULL);
            }
        }
    }
    
    // Clean up
    pthread_mutex_destroy(&game.board_mutex);
    endwin();
    
    return 0;
}
