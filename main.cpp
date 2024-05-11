#include <iostream>
#include <cstdlib>
#include <ctime>
#include <pthread.h>
#include <semaphore.h>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <unistd.h>
#include <queue>
#include <utility>
#include <vector>
#include <cmath>
#include <stack>
#include <climits>

// Define game board size
#define ROWS 25
#define COLS 25
#define CELL_SIZE 32 // Size of each cell in pixels

using namespace std;
using namespace sf;
//semaphores for key and permit for ghosts
int key = 2;
int permit = 2;
int countG = 0; //for number of ghost out of ghost house
pthread_mutex_t keyMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t permitMutex = PTHREAD_MUTEX_INITIALIZER;
//semaphore for speed boosts
sem_t speed;
// Define game entities
// Define game grid
int gameMap[ROWS][COLS] = {0};
int score = 0;
//bool afraid to check if ghost is afraid
bool afraid = false;
//check if game closed
bool closed = false;
//bool flag aqcuisition
bool acquired = true;
//bool for speed boost
int boosts = 2;
bool timeOut[] = {false,false,false,false};
priority_queue<pair<int,int>> speedQueue; // slower ghosts get more priority or else the game would be too difficult
// Mutex to protect user input
pthread_mutex_t inputMutex = PTHREAD_MUTEX_INITIALIZER;
// Mutex to protect pacman position
pthread_mutex_t pacmanMutex = PTHREAD_MUTEX_INITIALIZER;
// Mutex for game map
pthread_mutex_t gameMapMutex = PTHREAD_MUTEX_INITIALIZER;
//mutex for closed
pthread_mutex_t closedMutex = PTHREAD_MUTEX_INITIALIZER;
//mutex for speed boost
pthread_mutex_t speedMutex = PTHREAD_MUTEX_INITIALIZER;
//mutex for afraid
pthread_mutex_t afraidMutex = PTHREAD_MUTEX_INITIALIZER;
//mutex for acquired
pthread_mutex_t acquiredMutex = PTHREAD_MUTEX_INITIALIZER;
//mutex for count
pthread_mutex_t countMutex = PTHREAD_MUTEX_INITIALIZER;
//mutex for score
pthread_mutex_t scoreMutex = PTHREAD_MUTEX_INITIALIZER;
/////////////////////////////////////////////////////////////////////
//lives reset flag
bool lives_reset = false;
int allReset = 0;
int lives=3;
//current level
int currentLevel=1;
// Mutex for lives reset
pthread_mutex_t livesResetMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t livesMutex = PTHREAD_MUTEX_INITIALIZER;

// Constants for initial positions
const float INITIAL_PACMAN_X = CELL_SIZE + 25 / 8.0;
const float INITIAL_PACMAN_Y = CELL_SIZE + 25 / 4.0;

const float INITIAL_GHOST1_X = CELL_SIZE * 11;
const float INITIAL_GHOST2_X = CELL_SIZE * 12;
const float INITIAL_GHOST3_X = CELL_SIZE * 13;
const float INITIAL_GHOST4_X = CELL_SIZE * 12;
const float INITIAL_GHOST1_Y = CELL_SIZE * 11;
const float INITIAL_GHOST2_Y = CELL_SIZE * 11;
const float INITIAL_GHOST3_Y = CELL_SIZE * 11;
const float INITIAL_GHOST4_Y = CELL_SIZE * 12;


// Mutex for ghost positions
pthread_mutex_t ghost1Mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ghost2Mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ghost3Mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ghost4Mutex = PTHREAD_MUTEX_INITIALIZER;
////////////////////////////////////////////////////////////////////////////////////

// Shared variable for user input
Keyboard::Key userInputKey = Keyboard::Unknown;

//pacman coordinates
float pacman_x = CELL_SIZE + 25 / 8.0;
float pacman_y = CELL_SIZE + 25 / 4.0;
//ghost coordinates
float ghost1X = CELL_SIZE * 11;
float ghost2X = CELL_SIZE * 12;
float ghost3X = CELL_SIZE * 13;
float ghost4X = CELL_SIZE * 12;
float ghost1Y = CELL_SIZE * 11;
float ghost2Y = CELL_SIZE * 11;
float ghost3Y = CELL_SIZE * 11;
float ghost4Y = CELL_SIZE * 12;
// Function to check if a cell is valid
bool isValid(float x, float y, int gameMap[ROWS][COLS]) {
    int x_int = int(x);
    int y_int = int(y);
    return (x_int >= 0 && x_int < ROWS && y_int >= 0 && y_int < COLS && gameMap[y_int][x_int] != -2 && gameMap[y_int][x_int] != 1 && gameMap[y_int][x_int] != -1);
}

bool isValidP(float x, float y, int gameMap[ROWS][COLS],int direction,int d2) {
    if(direction == 0)
    {
        if((y != ((25/4.0)/CELL_SIZE) + (int)y ))
        {
            return 0;
        }
        else
        {
            x=x-((25/8.0)/CELL_SIZE);
            y=y-((25/4.0)/CELL_SIZE);
            if(d2 == 1)
                return (x >= 0 && x < ROWS && y >= 0 && y < COLS && gameMap[(int)ceil(y)][(int)ceil(x)] != -2 && gameMap[(int)ceil(y)][(int)ceil(x)] != 1 && gameMap[(int)ceil(y)][(int)ceil(x)] != -1);
            else
                return (x >= 0 && x < ROWS && y >= 0 && y < COLS && gameMap[(int)ceil(y)][(int)floor(x)] != -2 && gameMap[(int)ceil(y)][(int)floor(x)] != 1 && gameMap[(int)ceil(y)][(int)floor(x)] != -1);
        }

    }
    else
    {
        if((x != ((25/8.0)/CELL_SIZE) + (int)x)) 
        {
            return 0;
        }
        else 
        {
            x=x-((25/8.0)/CELL_SIZE);
            y=y-((25/4.0)/CELL_SIZE);
            if(d2 == 1)
                return (x >= 0 && x < ROWS && y >= 0 && y < COLS && gameMap[(int)ceil(y)][(int)ceil(x)] != -2 && gameMap[(int)ceil(y)][(int)ceil(x)] != 1 && gameMap[(int)ceil(y)][(int)ceil(x)] != -1);
            else
                return (x >= 0 && x < ROWS && y >= 0 && y < COLS && gameMap[(int)floor(y)][(int)ceil(x)] != -2 && gameMap[(int)floor(y)][(int)ceil(x)] != 1 && gameMap[(int)floor(y)][(int)ceil(x)] != -1);
        }
    }
}
// Function to draw the grid with appropriate shapes for pellets, power-ups, and walls
void drawGrid(sf::RenderWindow& window)
{
    sf::CircleShape pelletShape(5);
    sf::CircleShape powerUpShape(5);
    sf::RectangleShape wallShape(sf::Vector2f(CELL_SIZE - 1, CELL_SIZE - 1));
    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLS; j++)
        {
            // Draw pellets, power-ups, and walls
            pthread_mutex_lock(&gameMapMutex);
            int value = gameMap[i][j];
            pthread_mutex_unlock(&gameMapMutex);
            switch (value)
            {
            case 2:
                // Draw small white circle for pellet
                pelletShape.setFillColor(sf::Color::White);
                pelletShape.setPosition(j * CELL_SIZE + CELL_SIZE / 2 - 5, i * CELL_SIZE + CELL_SIZE / 2 - 5);
                window.draw(pelletShape);
                break;
            case 3:
                // Draw slightly bigger red circle for power-up
                powerUpShape.setFillColor(sf::Color::Red);
                powerUpShape.setPosition(j * CELL_SIZE + CELL_SIZE / 2 - 5, i * CELL_SIZE + CELL_SIZE / 2 - 5);
                window.draw(powerUpShape);
                break;
            case 4:
                // Draw slightly bigger white circle for power-up
                powerUpShape.setFillColor(sf::Color::Blue);
                powerUpShape.setPosition(j * CELL_SIZE + CELL_SIZE / 2 - 5, i * CELL_SIZE + CELL_SIZE / 2 - 5);
                window.draw(powerUpShape);
                break;
            case 1:
                // Draw blue rectangle for wall
                wallShape.setFillColor(sf::Color::Blue);
                wallShape.setPosition(j * CELL_SIZE, i * CELL_SIZE);
                window.draw(wallShape);
                break;
            case -1:
                // Draw blue rectangle for ghost house
                wallShape.setFillColor(sf::Color::Yellow);
                wallShape.setPosition(j * CELL_SIZE, i * CELL_SIZE);
                window.draw(wallShape);
                break;
            case -2:
                // Draw blue rectangle for fixed wall
                wallShape.setFillColor(sf::Color(100, 10, 255));
                wallShape.setPosition(j * CELL_SIZE, i * CELL_SIZE);
                window.draw(wallShape);
                break;
            }
        }
    }
}

// Function to initialize the game board with values
void initializeGameBoard()
{
    int powerUps = 0;
    bool powerUp_in_row = false;
    int powerUp_row = 0;
    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLS; j++)
        {
            gameMap[i][j] = 2;
        }
    }
    for (int i = 0; i < ROWS; i++)
    {
        if(powerUp_in_row)
            powerUp_row++;
        for (int j = 0; j < COLS; j++)
        {
            if ((i == 0 || j == 0) || (i == ROWS - 1 || j == COLS - 1))
            {
                // Fixed walls
                gameMap[i][j] = -2;
            }
            // Ghost house
            else if ((i == 10 && j >= 10) && (i == 10 && j <= 14))
            {
                if (i != 10 || j != 12)
                    gameMap[i][j] = -1;
            }
            else if ((i == 14 && j >= 10) && (i == 14 && j <= 14))
            {
                gameMap[i][j] = -1;
            }
            else if ((j == 10 && i >= 10) && (j == 10 && i <= 14))
            {
                gameMap[i][j] = -1;
            }
            else if ((j == 14 && i >= 10) && (j == 14 && i <= 14))
            {
                gameMap[i][j] = -1;
            }
            //ghost house empty
            else if((i == 11 && j >= 11) && (i == 11 && j <= 13))
            {
                gameMap[i][j] = 0;
            }
            else if((i == 13 && j >= 11) && (i == 13 && j <= 13))
            {
                gameMap[i][j] = 0;
            }
            else if((j == 11 && i >= 11) && (j == 11 && i <= 13))
            {
                gameMap[i][j] = 0;
            }
            else if((j == 13 && i >= 11) && (j == 13 && i <= 13))
            {
                gameMap[i][j] = 0;
            }
            // Openings
            else if (i == 12 || j == 12)
            {
                gameMap[i][j] = 2;
            }
            // Small square
            else if ((i == 6 && j >= 6) && (i == 6 && j <= 18))
            {
                gameMap[i][j] = 1;
            }
            else if ((i == 18 && j >= 6) && (i == 18 && j <= 18))
            {
                gameMap[i][j] = 1;
            }
            else if ((j == 6 && i >= 6) && (j == 6 && i <= 18))
            {
                gameMap[i][j] = 1;
            }
            else if ((j == 18 && i >= 6) && (j == 18 && i <= 18))
            {
                gameMap[i][j] = 1;
            }
            // Bigger square
            else if ((i == 4 && j >= 4) && (i == 4 && j <= 20))
            {
                gameMap[i][j] = 1;
            }
            else if ((i == 20 && j >= 4) && (i == 20 && j <= 20))
            {
                gameMap[i][j] = 1;
            }
            else if ((j == 4 && i >= 4) && (j == 4 && i <= 20))
            {
                gameMap[i][j] = 1;
            }
            else if ((j == 20 && i >= 4) && (j == 20 && i <= 20))
            {
                gameMap[i][j] = 1;
            }
            // Bigger square
            else if ((i == 2 && j >= 2) && (i == 2 && j <= 22))
            {
                gameMap[i][j] = 1;
            }
            else if ((i == 22 && j >= 2) && (i == 22 && j <= 22))
            {
                gameMap[i][j] = 1;
            }
            else if ((j == 2 && i >= 2) && (j == 2 && i <= 22))
            {
                gameMap[i][j] = 1;
            }
            else if ((j == 22 && i >= 2) && (j == 22 && i <= 22))
            {
                gameMap[i][j] = 1;
            }
            else
            {
                // Randomly place pellets and power-ups
                int randNum = rand() % 5; // Generate random number from 0 to 4
                if (randNum == 2)
                {
                    int num = 3 + rand() % 2;
                    if(powerUp_row == 3)
                    {
                        powerUp_in_row = false;
                        powerUp_row = 0;
                    }
                    if(powerUps > 5 || powerUp_in_row)
                        gameMap[i][j] = 3;
                    else if(num == 4)
                    {
                        powerUp_in_row = true;
                        powerUps++;
                        gameMap[i][j] = num;
                    }
                    else
                        gameMap[i][j] = num; // Randomly assign power-up value (2, 3, or 4)
                }
                else
                {
                    gameMap[i][j] = 2;
                }
            }
        }
    }
}
//lives reset function
// Function to reset positions of Pacman and the ghosts
void resetPositions(int gNum) {
    switch(gNum)
    {
    case 0:
        pthread_mutex_lock(&pacmanMutex);
        pacman_x = INITIAL_PACMAN_X;
        pacman_y = INITIAL_PACMAN_Y;
        pthread_mutex_unlock(&pacmanMutex);
        break;
    case 1:
        pthread_mutex_lock(&ghost1Mutex);
        ghost1X = INITIAL_GHOST1_X;
        ghost1Y = INITIAL_GHOST1_Y;
        pthread_mutex_unlock(&ghost1Mutex);
        break;
    case 2:
        pthread_mutex_lock(&ghost2Mutex);
        ghost2X = INITIAL_GHOST2_X;
        ghost2Y = INITIAL_GHOST2_Y;
        pthread_mutex_unlock(&ghost2Mutex);
        break;
    case 3:
        pthread_mutex_lock(&ghost3Mutex);
        ghost3X = INITIAL_GHOST3_X;
        ghost3Y = INITIAL_GHOST3_Y;
        pthread_mutex_unlock(&ghost3Mutex);
        break;
    case 4:
        pthread_mutex_lock(&ghost4Mutex);
        ghost4X = INITIAL_GHOST4_X;
        ghost4Y = INITIAL_GHOST4_Y;
        pthread_mutex_unlock(&ghost4Mutex);
        break;
    }
}


// Function to handle user input
void* userInput(void* arg)
{
    // Unpack arguments
    sf::RenderWindow* window = (sf::RenderWindow*)arg;
    while (window->isOpen())
    {
        Event event;
        // Process SFML events
        while (window->pollEvent(event))
        {
            if (event.type == Event::Closed)
            {
                //window->close();
                pthread_mutex_lock(&closedMutex);
                closed = true;
                pthread_mutex_unlock(&closedMutex);
                pthread_exit(NULL);
            }
            else if (event.type == Event::KeyPressed)
            {
                if(event.key.code == Keyboard::Escape)
                {
                    pthread_mutex_lock(&closedMutex);
                    closed = true;
                    pthread_mutex_unlock(&closedMutex);
                    pthread_exit(NULL);
                }
                // Lock mutex before accessing shared variable
                pthread_mutex_lock(&inputMutex);
                userInputKey = event.key.code;
                // Unlock mutex after updating shared variable
                pthread_mutex_unlock(&inputMutex);
            }
        }
    }
    pthread_exit(NULL);
}
// Function to handle movement
void movePacman(void* arg)
{
    sf::Sprite* pacman_shape = (sf::Sprite*)arg;
    sf::Clock afraidClock;
    //pacman rotation
    int rotation = 0;
    //pacman mouth closing
    int mouth = 0;
    // Initial movement direction
    int pacman_direction_x = 0;
    int pacman_direction_y = 0;
    //pacman previous values
    int prevX = 0;
    int prevY = 0;
    int prevRotation = 0;
    int delay = 6000;
    while (true)
    {
        pthread_mutex_lock(&closedMutex);
        if(closed)
        {
            pthread_mutex_unlock(&closedMutex);
            break;
        }
        pthread_mutex_unlock(&closedMutex);
        prevX = pacman_direction_x;
        prevY = pacman_direction_y;
        prevRotation = rotation;
        // Lock mutex before accessing shared variable
        pthread_mutex_lock(&inputMutex);
        // Update movement based on current key
        switch (userInputKey)
        {
        case Keyboard::Up:
            pacman_direction_x = 0;
            pacman_direction_y = -1;
            rotation = 270;
            break;
        case Keyboard::Down:
            pacman_direction_x = 0;
            pacman_direction_y = 1;
            rotation = 90;
            break;
        case Keyboard::Left:
            pacman_direction_x = -1;
            pacman_direction_y = 0;
            rotation = 180;
            break;
        case Keyboard::Right:
            pacman_direction_x = 1;
            pacman_direction_y = 0;
            rotation = 0;
            break;
        default:
            break;
        }
        pthread_mutex_unlock(&inputMutex);

        // Calculate the next position
        float nextX, nextY;
        pthread_mutex_lock(&pacmanMutex);
        nextX = pacman_x + pacman_direction_x;
        nextY = pacman_y + pacman_direction_y;
        // Check if the next position is a wall
        int d2 = (pacman_direction_x == 0 ? pacman_direction_y : pacman_direction_x);
        pthread_mutex_lock(&gameMapMutex);
        bool valid = isValidP(nextX / CELL_SIZE, nextY / CELL_SIZE, gameMap,(pacman_direction_x == 0 ? 1 : 0),d2);
        pthread_mutex_unlock(&gameMapMutex);
        if (!valid)
        {
            rotation = prevRotation;
            if(prevX == pacman_direction_x && prevY == pacman_direction_y)
            {
                pacman_direction_x = 0;
                pacman_direction_y = 0;
            }
            else
            {
                pacman_direction_x = prevX;
                pacman_direction_y = prevY;
            }
        }
        // Move pacman
        pacman_x += pacman_direction_x;
        pacman_y += pacman_direction_y;
        nextX = pacman_x;
        nextY = pacman_y;
        pacman_shape->setPosition(nextX + 25/2, nextY + 25/2); // Update pacman position
        pacman_shape->setRotation(rotation);
        pthread_mutex_unlock(&pacmanMutex);
        pthread_mutex_lock(&gameMapMutex);
        int value = gameMap[(int)nextY / CELL_SIZE][(int)nextX / CELL_SIZE];
        pthread_mutex_unlock(&gameMapMutex);
        if ( value == 2 || value == 3)
        {
            // Handle scoring when encountering red (2) or white (3) balls
            pthread_mutex_lock(&gameMapMutex);
            int ballValue = gameMap[(int)nextY / CELL_SIZE][(int)nextX / CELL_SIZE];
            pthread_mutex_unlock(&gameMapMutex);
            if (ballValue != 0)
            { // Check if the ball hasn't been consumed already
                pthread_mutex_lock(&scoreMutex);
                score += ballValue;
                pthread_mutex_unlock(&scoreMutex);
                // Update the game grid to mark the ball as consumed
                pthread_mutex_lock(&gameMapMutex);
                gameMap[(int)nextY / CELL_SIZE][(int)nextX / CELL_SIZE] = 0;
                pthread_mutex_unlock(&gameMapMutex);
            }
        }
        else if(value == 4)
        {
            // power pellet makes ghosts afraid
            //num ghosts outside
            pthread_mutex_lock(&afraidMutex);
            if(!afraid)
            {
                afraid = true;
                pthread_mutex_lock(&gameMapMutex);
                gameMap[(int)nextY / CELL_SIZE][(int)nextX / CELL_SIZE] = 0;
                pthread_mutex_unlock(&gameMapMutex);
                delay = 4000;
                afraidClock.restart();
            }
            pthread_mutex_unlock(&afraidMutex);
        }
        pthread_mutex_lock(&afraidMutex);
        if(afraid)
        {
            if(afraidClock.getElapsedTime().asSeconds() >= 10)
            {
                afraid = false;
                delay = 6000;
            }
        }
        pthread_mutex_unlock(&afraidMutex);
        usleep(delay); // Sleep for 0.3 seconds
        pthread_mutex_lock(&livesResetMutex);
        if(lives_reset)
        {
            resetPositions(0);
            allReset++;
            pthread_mutex_unlock(&livesResetMutex);
            continue;
        }
        pthread_mutex_unlock(&livesResetMutex);
    }
}


void handleLevelChange() {
    bool levelComplete = true;
    pthread_mutex_lock(&gameMapMutex);
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (gameMap[i][j] == 2 || gameMap[i][j] == 3) {
                levelComplete = false;
                break;
            }
        }
        if (!levelComplete) {
            break;
        }
    }
    pthread_mutex_unlock(&gameMapMutex);
    if (levelComplete) {
        currentLevel++;  // Increment the level counter
        cout<<"level "<<currentLevel<<endl;
        initializeGameBoard();  // Reset the game board for the next level
        resetPositions(0);  // Reset Pac-Man position
        // Add any other necessary logic for level change here
    }
}
bool isValidG2(float x, float y, int gameMap[ROWS][COLS],int direction,int d2,int d3 = 0)
{
    if(direction == 0)//moving horizontally
    {
        if(y - (int)y != 0)
            return false;
        else
        {
            if(d2 == 1)
                return (x >= 0 && x < ROWS && y >= 0 && y < COLS && gameMap[(int)ceil(y)][(int)floor(x)] != -2 && gameMap[(int)ceil(y)][(int)floor(x)] != 1 && gameMap[(int)ceil(y)][(int)floor(x)] != -1);
            else
                return (x >= 0 && x < ROWS && y >= 0 && y < COLS && gameMap[(int)ceil(y)][(int)ceil(x)] != -2 && gameMap[(int)ceil(y)][(int)ceil(x)] != 1 && gameMap[(int)ceil(y)][(int)ceil(x)] != -1);
        }
    }
    else if(direction == 1)//moving vertically
    {
        if((x - (int)x) != 0) 
            return 0;
        else 
        {
            if(d2 == 1)
            {
                return (x >= 0 && x < ROWS && y >= 0 && y < COLS && gameMap[(int)floor(y)][(int)ceil(x)] != -2 && gameMap[(int)floor(y)][(int)ceil(x)] != 1 && gameMap[(int)floor(y)][(int)ceil(x)] != -1);
            }
            else
                return (x >= 0 && x < ROWS && y >= 0 && y < COLS && gameMap[(int)ceil(y)][(int)ceil(x)] != -2 && gameMap[(int)ceil(y)][(int)ceil(x)] != 1 && gameMap[(int)ceil(y)][(int)ceil(x)] != -1);
        }
    }
    else
    {
        if((x - (int)x) != 0) 
            return 1;
        if((y - (int)y) != 0) 
            return 1;
        return (x >= 0 && x < ROWS && y >= 0 && y < COLS && gameMap[(int)ceil(y)][(int)ceil(x)] != -2 && gameMap[(int)ceil(y)][(int)ceil(x)] != 1 && gameMap[(int)ceil(y)][(int)ceil(x)] != -1);
    }
}
int shortestPath(float startX, float startY, float destX, float destY, int gameMap[ROWS][COLS]) {
    bool visited[ROWS][COLS] = {false};
    std::queue<std::pair<int, int>> q;
    q.push({startX, startY});
    visited[(int)startX][(int)startY] = true;

    int dx[] = {0, 0, 1, -1};
    int dy[] = {1, -1, 0, 0};
    int dist[ROWS][COLS] = {0}; // Store distances from start position

    while (!q.empty()) {
        int x = q.front().first;
        int y = q.front().second;
        q.pop();

        // Check if destination is reached
        if (round(x) == round(destX) && round(y) == round(destY)) {
            return dist[x][y];
        }

        // Explore adjacent cells
        for (int i = 0; i < 4; ++i) {
            int newX = x + dx[i];
            int newY = y + dy[i];

            // Check validity and unvisited status
            int d2 = (dx[i] == 0 ? dy[i] : dx[i]);
            int direction = (dx[i] == 0 ? 1 : 0);
            pthread_mutex_lock(&gameMapMutex);
            bool valid = isValid(newX, newY, gameMap);
            pthread_mutex_unlock(&gameMapMutex);
            if (valid && !visited[newX][newY]) {
                q.push({newX, newY});
                visited[newX][newY] = true;
                dist[newX][newY] = dist[x][y] + 1; // Update distance
            }
        }
    }

    // If destination is unreachable
    return INT_MAX;
}
std::pair<float, float> findNextMove(int gameMap[ROWS][COLS], float ghostX, float ghostY, float pacmanX, float pacmanY) 
{
    int dx[] = {0, 0, 1, -1};
    int dy[] = {1, -1, 0, 0};
    float tempghostX,tempghostY;
    std::pair<std::pair<float, float>, int> nextMove[4];
    for(int i=0;i<4;i++)
    {
        nextMove[i] = {{-1, -1}, INT_MAX};
    }
    for (int i = 0; i < 4; i++) {
        int dir = (dx[i] == 0 ? 1 : 0);
        int d2 = (dx[i] == 0 ? dy[i] : dx[i]);
        tempghostX = ghostX;
        tempghostY = ghostY;
        if(ghostX - (int)ghostX !=0 && i <= 1)
            continue;
        if(ghostY - (int)ghostY !=0 && i > 1)
        {
            continue;
        }
        if(dir == 0)//moving horizontal
        {
            if(d2 == 1)//moving right
            {
                tempghostY = round(ghostY);
                tempghostX = floor(ghostX);
            }
            else
            {
                tempghostY = round(ghostY);
                tempghostX = ceil(ghostX);
            }
        }
        else
        {
            if(d2 == 1)//moving down
            {
                tempghostX = round(ghostX);
                tempghostY = floor(ghostY);
            }
            else
            {
                tempghostX = round(ghostX);
                tempghostY = ceil(ghostY);
            }
        }

        int x = tempghostX + dx[i];
        int y = tempghostY + dy[i];
        nextMove[i] = {{-1, -1}, INT_MAX};
        pthread_mutex_lock(&gameMapMutex);
        bool valid = isValid(x, y, gameMap);
        pthread_mutex_unlock(&gameMapMutex);
        if (valid) {
            nextMove[i].first = {x, y};
            // call recursive function to find shortest path
            bool visited[ROWS][COLS] = {false};
            nextMove[i].second = shortestPath(x, y, pacmanX, pacmanY, gameMap);
        }
    }

    // find minimum of the calculated distances
    int minDist = INT_MAX;
    int minIndex = -1;
    for (int i = 0; i < 4; i++) {
        if (nextMove[i].second < minDist) {
            minDist = nextMove[i].second;
            minIndex = i;
        }
    }
    return nextMove[minIndex].first;
}
//Ghost Eyes 1
void changeEyes(Texture& ghostTexture,Sprite* ghost_shape, int diffX, int diffY,int gNum)
{
    switch(gNum)
    {
    case 1:
        if(abs(diffX) > abs(diffY))
        {
            if(diffX > 0)
            {
                if (!ghostTexture.loadFromFile("img/ghost1_1.png"))
                {
                    // Handle loading error
                    std::cerr << "Failed to load ghost texture!" << std::endl;
                    return; // Exit the program or handle the error appropriately
                }
                ghost_shape->setTexture(ghostTexture);
            }
            else
            {
                if (!ghostTexture.loadFromFile("img/ghost1_4.png"))
                {
                    // Handle loading error
                    std::cerr << "Failed to load ghost texture!" << std::endl;
                    return; // Exit the program or handle the error appropriately
                }
                ghost_shape->setTexture(ghostTexture);
            }
        }
        else
        {
            if(diffY > 0)
            {
                if (!ghostTexture.loadFromFile("img/ghost1_3.png"))
                {
                    // Handle loading error
                    std::cerr << "Failed to load ghost texture!" << std::endl;
                    return; // Exit the program or handle the error appropriately
                }
                ghost_shape->setTexture(ghostTexture);
            }
            else
            {
                if (!ghostTexture.loadFromFile("img/ghost1_2.png"))
                {
                    // Handle loading error
                    std::cerr << "Failed to load ghost texture!" << std::endl;
                    return; // Exit the program or handle the error appropriately
                }
                ghost_shape->setTexture(ghostTexture);
            }
        }
        break;
    case 3:
        if(abs(diffX) > abs(diffY))
        {
            if(diffX > 0)
            {
                if (!ghostTexture.loadFromFile("img/ghost3_1.png"))
                {
                    // Handle loading error
                    std::cerr << "Failed to load ghost texture!" << std::endl;
                    return; // Exit the program or handle the error appropriately
                }
                ghost_shape->setTexture(ghostTexture);
            }
            else
            {
                if (!ghostTexture.loadFromFile("img/ghost3_4.png"))
                {
                    // Handle loading error
                    std::cerr << "Failed to load ghost texture!" << std::endl;
                    return; // Exit the program or handle the error appropriately
                }
                ghost_shape->setTexture(ghostTexture);
            }
        }
        else
        {
            if(diffY > 0)
            {
                if (!ghostTexture.loadFromFile("img/ghost3_3.png"))
                {
                    // Handle loading error
                    std::cerr << "Failed to load ghost texture!" << std::endl;
                    return; // Exit the program or handle the error appropriately
                }
                ghost_shape->setTexture(ghostTexture);
            }
            else
            {
                if (!ghostTexture.loadFromFile("img/ghost3_2.png"))
                {
                    // Handle loading error
                    std::cerr << "Failed to load ghost texture!" << std::endl;
                    return; // Exit the program or handle the error appropriately
                }
                ghost_shape->setTexture(ghostTexture);
            }
        }
        break;
    }
}
void bob(sf::Clock& clock, sf::Sprite* ghost_shape,int& pos)
{
    float time = clock.getElapsedTime().asSeconds();
    if(time >= 0.5)
    {
        ghost_shape->move(0, pos);
        clock.restart();
        pos = -pos;
    }
}
//bool try aqcuisition
bool tryAcquire(bool& flag) 
{
    if(flag)
    {
        pthread_mutex_lock(&acquiredMutex);
        if(!acquired)
        {
            cout<<"Starting delayed turn"<<endl;
            cout<<"Acquired:\t"<<acquired<<endl;
            acquired = true;
            pthread_mutex_lock(&countMutex);
            countG++;
            cout<<countG<<endl;
            pthread_mutex_unlock(&countMutex);
            flag = 0;
            cout<<"returning function"<<endl;
            cout<<"Acquired:\t"<<acquired<<endl;
            pthread_mutex_unlock(&acquiredMutex);
            return 1;
        }
        else
        {
            pthread_mutex_unlock(&acquiredMutex);
            return 0;
        }
        pthread_mutex_unlock(&acquiredMutex);
    }
    pthread_mutex_lock(&keyMutex);
    if(key>0)
        key--;
    else
    {
        pthread_mutex_unlock(&keyMutex);
        return false;
    }
    pthread_mutex_unlock(&keyMutex);
    pthread_mutex_lock(&permitMutex);
    if(permit == 0) // permit has already been acquired release key and return false
    {
        pthread_mutex_unlock(&permitMutex);
        pthread_mutex_lock(&keyMutex);
        key++;
        pthread_mutex_unlock(&keyMutex);
        return false;
    }
    else // grab permit
    {
        permit--;
        pthread_mutex_unlock(&permitMutex);
        pthread_mutex_lock(&acquiredMutex);
        if(acquired)
        {
            cout<<"Acquired but waiting"<<endl;
            pthread_mutex_unlock(&acquiredMutex);
            flag = 1; // flag to know it has acquired and is waiting for delayed turn;
            return false;
        }
        else
        {
            cout<<"Acquired"<<endl;
            acquired = true;
            pthread_mutex_lock(&countMutex);
            countG++;
            pthread_mutex_unlock(&countMutex);
            pthread_mutex_unlock(&acquiredMutex);
            return true;
        }
        pthread_mutex_unlock(&acquiredMutex);
        return true;
    }
}

void resetAquired(sf::Clock& clock, bool& flag) 
{
    // Check if deadlock condition is reached
    if (clock.getElapsedTime().asSeconds() >= 5 && flag == 1) 
    {
        pthread_mutex_lock(&acquiredMutex);
        acquired = false; // Reset acquired state
        pthread_mutex_unlock(&acquiredMutex);
        clock.restart(); // Restart the clock
    }
    // Check if the ghost acquired both resources
    pthread_mutex_lock(&countMutex);
    int value = countG;
    pthread_mutex_unlock(&countMutex);
    if (value == 2) {
        // Reset state and release resources
        pthread_mutex_lock(&acquiredMutex);
        acquired = true;
        pthread_mutex_unlock(&acquiredMutex); // Release the lock
        
        cout << "Reset" << endl;
        
        pthread_mutex_lock(&keyMutex);
        key = 2; // Release the keys
        pthread_mutex_unlock(&keyMutex);
        pthread_mutex_lock(&permitMutex);
        permit = 2; // Release the permits
        pthread_mutex_unlock(&permitMutex);
        pthread_mutex_lock(&countMutex);
        countG = 0; //reset count
        pthread_mutex_unlock(&countMutex);
        flag = 1;
        clock.restart(); // Restart the clock
    }
}

void startWait(sf::Clock& clock, bool& flag) {
    // Check if the ghost has been waiting for too long
    if (clock.getElapsedTime().asSeconds() >= 5 && flag == 1) {
        pthread_mutex_lock(&acquiredMutex);
        acquired = false; // Reset acquired state
        pthread_mutex_unlock(&acquiredMutex);
        flag = 0;
        clock.restart(); // Restart the clock
    }
    else
        return;
}

void changeEyes2(pair<int,int> direction, sf::Sprite* ghost_shape, sf::Texture& ghostTexture, int gNum)
{
    std::string ghostTextureFile;
    switch(gNum)
    {
    case 2:
        if(direction.first == 1)// right
            ghostTextureFile = "img/ghost2_1.png";
        else if(direction.first == -1)//left
            ghostTextureFile = "img/ghost2_4.png";
        else if(direction.second == 1)//down
            ghostTextureFile = "img/ghost2_3.png";
        else
            ghostTextureFile = "img/ghost2_2.png";
        if (!ghostTexture.loadFromFile(ghostTextureFile))
        {
            // Handle loading error
            std::cerr << "Failed to load ghost texture!" << std::endl;
            return; // Exit the program or handle the error appropriately
        }
        ghost_shape->setTexture(ghostTexture);
        break;
    case 4:
        if(direction.first == 1)// right
            ghostTextureFile = "img/ghost4_1.png";
        else if(direction.first == -1)//left
            ghostTextureFile = "img/ghost4_4.png";
        else if(direction.second == 1)//down
            ghostTextureFile = "img/ghost4_3.png";
        else
            ghostTextureFile = "img/ghost4_2.png";
        if (!ghostTexture.loadFromFile(ghostTextureFile))
        {
            // Handle loading error
            std::cerr << "Failed to load ghost texture!" << std::endl;
            return; // Exit the program or handle the error appropriately
        }
        ghost_shape->setTexture(ghostTexture);
        break;
    }
}
bool requestSpeedBoost(int gNum,int priority,bool& flag,Clock& clock)
{
    if(!flag && !timeOut[gNum])//not already requested boost
    {
        cout<<"Boost Requested By: "<<gNum<<endl;
        pthread_mutex_lock(&speedMutex);
        speedQueue.push({priority,gNum});
        pthread_mutex_unlock(&speedMutex);
        flag = 1;
        return false;
    }
    else if(timeOut[gNum])
    {
        if(clock.getElapsedTime().asSeconds() >= 10 && !flag)//another 5 seconds before requesting again
        {
            timeOut[gNum] = false;
            return false;
        }
        else if(clock.getElapsedTime().asSeconds() >= 5 && flag)//timeout boost after 5 seconds
        {
            cout<<"Speed boost Timed out by: "<<gNum<<endl;
            pthread_mutex_lock(&speedMutex);
            boosts++;
            cout<<"Boosts remaining: "<<boosts<<endl;
            pthread_mutex_unlock(&speedMutex);
            flag = 0;
            return false;
        }
        return true;
    }
    else
    {
        pthread_mutex_lock(&speedMutex);
        //check if bro is top of queue
        if(speedQueue.top().second == gNum)
        {
            if(boosts > 0)
            {
                cout<<"Speed boost Attained by: "<<gNum<<endl;
                speedQueue.pop();
                boosts--;
                cout<<"Boosts remaining: "<<boosts<<endl;
                timeOut[gNum] = true;
                pthread_mutex_unlock(&speedMutex);
                clock.restart();
                return true;
            }
            else
            {
                pthread_mutex_unlock(&speedMutex);
                return false;
            }
        }
        else
        {
            pthread_mutex_unlock(&speedMutex);
            return false;
        }
    }
}
//house wait function
void houseWait(sf::Clock& clock, sf::Sprite* ghost_shape,int& pos,bool& flag)
{
    bool reset = true;
    while(reset)
    {
        pthread_mutex_lock(&livesResetMutex);
        reset = lives_reset;
        pthread_mutex_unlock(&livesResetMutex);
    }
    cout<<"Waiting"<<endl;
    while(1)
    {
        bob(clock, ghost_shape,pos);
        if(tryAcquire(flag))
        {
            break;
        }
        pthread_mutex_lock(&closedMutex);
        if(closed)
        {
            pthread_mutex_unlock(&closedMutex);
            pthread_exit(NULL);

        }
        pthread_mutex_unlock(&closedMutex);
    }
}
//Function to get furthest point from pacman
std::pair<float, float> findFurthestPoint(float pacmanX, float pacmanY)
{
    //furthest point is the corner opposite to pacman
    if(pacmanX < 15 && pacmanY < 15)
        return {23,23};
    else if(pacmanX < 15 && pacmanY > 15)
        return {23,1};
    else if(pacmanX > 15 && pacmanY < 15)
        return {1,23};
    else
        return {1,1};
}
void moveGhost1(void* arg) { // smart movement
    void ** args = (void**)arg;
    int* gN0 = (int*)args[0];
    int gNum = *gN0;
    float& ghostX = (gNum == 1 ? ghost1X : ghost3X);
    float& ghostY = (gNum == 1 ? ghost1Y : ghost3Y);
    int priority = (gNum == 1 ? 1 : 0);
    sf::Sprite* ghost_shape = (sf::Sprite*)args[1];
    sf::Texture ghostTexture;
    sf::Clock clock;
    ghost_shape->setPosition(ghostX + 25/8, ghostY + 25/4); // Adjust position based on sprite size
    // bob up and down while waiting for key
    int pos = 25;
    bool flag = 0; // flag to know it has acquired
    bool speed = false;
    int delay = 7000;
    bool timeFlag1 = false;
    pair<float,float> offset = {25/8,25/4};
    ///////////////////////////////////////////////////////////////////////----------------House
    houseWait(clock, ghost_shape,pos,flag);
    ///////////////////////////////////////////////////////////////////////----------------Outside
    pthread_mutex_lock(&afraidMutex);
    if(afraid)
        timeFlag1 = true;
    else
        timeFlag1 = false;
    pthread_mutex_unlock(&afraidMutex);
    ghost_shape->setPosition(ghostX + 25/8, ghostY + 25/4); // Adjust position based on sprite size
    while(1)
    {
        pthread_mutex_lock(&afraidMutex);
        bool afraidLocal = afraid;
        pthread_mutex_unlock(&afraidMutex);
        if(!afraidLocal)
        {
            if(requestSpeedBoost(gNum,1,speed,clock))
                delay = 5000;
            else
                delay = 7000;
        }
        pthread_mutex_lock(&closedMutex);
        if(closed)
        {
            pthread_mutex_unlock(&closedMutex);
            break;
        }
        pthread_mutex_unlock(&closedMutex);
        pthread_mutex_lock(&pacmanMutex);
        float pacX = pacman_x / CELL_SIZE;
        float pacY = pacman_y / CELL_SIZE;
        pthread_mutex_unlock(&pacmanMutex);
        (gNum == 1) ? pthread_mutex_lock(&ghost1Mutex) : pthread_mutex_lock(&ghost3Mutex);
        int diffX = pacX - ghostX / CELL_SIZE;
        int diffY = pacY - ghostY / CELL_SIZE;
        //change texture to look at pacman
        std::pair<int, int> nextMove;
        pair<float,float> pacPoint;
        float nextX;
        float nextY;
        if(afraidLocal)
        {
            pacPoint = findFurthestPoint(pacX,pacY);
            nextMove = findNextMove(gameMap, (ghostX / CELL_SIZE), (ghostY / CELL_SIZE), pacPoint.first, pacPoint.second);
            if(timeFlag1)
            {
                timeFlag1 = false;
                ghostTexture.loadFromFile("img/a.png");
                ghost_shape->setTexture(ghostTexture);
                ghost_shape->setScale(1.7,1.7);
                offset = {-7.5,-10};
                delay = 8000;
            }
            nextX = nextMove.first;
            nextY = nextMove.second;
            pthread_mutex_lock(&pacmanMutex);
            float pacX = pacman_x / CELL_SIZE;
            float pacY = pacman_y / CELL_SIZE;
            pthread_mutex_unlock(&pacmanMutex);
            if ((round(pacX) == round(ghostX / CELL_SIZE) && round(pacY) == round(ghostY / CELL_SIZE)) || (round(nextX) == round(pacX) && round(nextY) == round(pacY)))
            {
                (gNum == 1) ? pthread_mutex_unlock(&ghost1Mutex) : pthread_mutex_unlock(&ghost3Mutex);
                pthread_mutex_lock(&scoreMutex);
                score += 200; // Increment score
                pthread_mutex_unlock(&scoreMutex);
                resetPositions(gNum);
                changeEyes(ghostTexture,ghost_shape,diffX,diffY,gNum);
                ghost_shape->setScale(1,1);
                offset = {25/8,25/4};
                ghost_shape->setPosition(ghostX + offset.first, ghostY + offset.second);
                pthread_mutex_lock(&livesResetMutex);
                allReset++;
                pthread_mutex_unlock(&livesResetMutex);
                pos = 25;
                flag = 0;
                houseWait(clock, ghost_shape,pos,flag);
                pthread_mutex_lock(&afraidMutex);
                if(afraid)
                    timeFlag1 = true;
                else
                    timeFlag1 = false;
                pthread_mutex_unlock(&afraidMutex);
                continue;
            }
        }
        else
        {
            changeEyes(ghostTexture,ghost_shape,diffX,diffY,gNum);
            nextMove = findNextMove(gameMap, (ghostX / CELL_SIZE), (ghostY / CELL_SIZE), pacX, pacY);
            if(!timeFlag1)
            {
                ghost_shape->setScale(1,1);
                offset = {25/8,25/4};
                timeFlag1 = true;
            }
            nextX = nextMove.first;
            nextY = nextMove.second;
            if ((round(pacX) == round(ghostX / CELL_SIZE) && round(pacY)== round(ghostY / CELL_SIZE)) || (round(nextX) == round(pacX) && round(nextY) == round(pacY)))
            {
                pthread_mutex_lock(&livesResetMutex);
                if(!lives_reset)
                {
                    pthread_mutex_lock(&livesMutex);
                    lives--; // Decrement lives
                    pthread_mutex_unlock(&livesMutex);
                }
                pthread_mutex_unlock(&livesResetMutex);
                // Reset positions if lives are greater than 0
                if (lives > 0) 
                {
                    pthread_mutex_lock(&livesResetMutex);
                    lives_reset = true;
                    pthread_mutex_unlock(&livesResetMutex);
                }
            }
        }
        float prevX = ghostX;
        float prevY = ghostY;
        //make sure ghost moves only one cell
        ghostX = (ceil(nextX) > (ghostX/CELL_SIZE)) ? ghostX + 1 : (ceil(nextX) < (ghostX/CELL_SIZE)) ? ghostX - 1 : ghostX;
        ghostY = (ceil(nextY) > (ghostY/CELL_SIZE)) ? ghostY + 1 : (ceil(nextY) < (ghostY/CELL_SIZE)) ? ghostY - 1 : ghostY;

        ghost_shape->setPosition(ghostX + offset.first, ghostY + offset.second);
        (gNum == 1) ? pthread_mutex_unlock(&ghost1Mutex) : pthread_mutex_unlock(&ghost3Mutex);
        usleep(delay); // Sleep for 0.3 seconds
        pthread_mutex_lock(&livesResetMutex);
        if(lives_reset)
        {
            cout<<"Resetting"<<endl;
            resetPositions(gNum);
            ghost_shape->setPosition(ghostX + 25/8, ghostY + 25/4);
            allReset++;
            pthread_mutex_unlock(&livesResetMutex);
            pos = 25;
            flag = 0;
            houseWait(clock, ghost_shape,pos,flag);
            pthread_mutex_lock(&afraidMutex);
            if(afraid)
                timeFlag1 = true;
            else
                timeFlag1 = false;
            pthread_mutex_unlock(&afraidMutex);
            continue;
        }
        pthread_mutex_unlock(&livesResetMutex);
    }
    pthread_exit(NULL);
}
void moveGhost2(void* arg) 
{ // random movement with direction persistence
    void** args = (void**)arg;
    int* gN0 = (int*)args[0];
    int gNum = *gN0;
    float& ghostX = (gNum == 2 ? ghost2X : ghost4X);
    float& ghostY = (gNum == 2 ? ghost2Y : ghost4Y);
    sf::Sprite* ghost_shape = (sf::Sprite*)args[1];
    sf::Texture ghostTexture;
    pair<int,int> direction =  {0,-1}; // Random initial direction
    sf::Clock clock;
    ghost_shape->setPosition(ghostX + 25/8, ghostY + 25/4); // Adjust position based on sprite size
    int pos = 25;
    bool flag = 0; // flag to know it has acquired
    bool speed = false;
    int priority = (gNum == 2 ? 2 : 3);
    int delay = 10000;
    bool timeFlag1 = false;
    pair<float,float> offset = {25/8,25/4};
    ///////////////////////////////////////////////////////////////////////----------------House
    houseWait(clock, ghost_shape,pos,flag);
    ///////////////////////////////////////////////////////////////////////----------------Outside
    pthread_mutex_lock(&afraidMutex);
    if(afraid)
        timeFlag1 = true;
    else
        timeFlag1 = false;
    pthread_mutex_unlock(&afraidMutex);
    while(1)
    {
        pthread_mutex_lock(&afraidMutex);
        bool afraidLocal = afraid;
        pthread_mutex_unlock(&afraidMutex);
        if(!afraidLocal)
        {
            if(requestSpeedBoost(gNum,priority,speed,clock))
                delay = 5000;
            else
                delay = 8000;
        }
        pthread_mutex_lock(&closedMutex);
        if(closed)
        {
            pthread_mutex_unlock(&closedMutex);
            break;
        }
        pthread_mutex_unlock(&closedMutex);
        float nextMoveX;
        float nextMoveY;

        std::pair<int, int> nextMove = {0, 0};
        //find if any turns
        int rando = ((rand()%2)-(rand()%2));
        pthread_mutex_lock(&pacmanMutex);
        float pacX = pacman_x / CELL_SIZE;
        float pacY = pacman_y / CELL_SIZE;
        pthread_mutex_unlock(&pacmanMutex);
        (gNum == 2) ? pthread_mutex_lock(&ghost2Mutex) : pthread_mutex_lock(&ghost4Mutex);
        if(!afraidLocal)
        {
            if(abs(direction.first) == 1)
            {
                while(rando == 0)
                    rando = ((rand()%2)-(rand()%2));
                pthread_mutex_lock(&gameMapMutex);
                if(isValidG2(ghostX/CELL_SIZE, ghostY/CELL_SIZE + (rando), gameMap,0,(rando)) && !isValidG2(ghostX/CELL_SIZE+(rando), ghostY/CELL_SIZE + (rando), gameMap,-1,rando,rando) && !isValidG2(ghostX/CELL_SIZE+(-rando), ghostY/CELL_SIZE + (rando), gameMap,-1,-rando,rando))
                    nextMove = {0,rando};
                else if(isValidG2(ghostX/CELL_SIZE, ghostY/CELL_SIZE + (rando*-1), gameMap,1,(-rando)) && !isValidG2(ghostX/CELL_SIZE+(rando), ghostY/CELL_SIZE + (rando*-1), gameMap,-1,rando,-rando) && !isValidG2(ghostX/CELL_SIZE+(-rando), ghostY/CELL_SIZE + (rando*-1), gameMap,-1,-rando,rando))
                    nextMove = {0,-rando};
                else if(isValidG2(ghostX/CELL_SIZE + direction.first, ghostY/CELL_SIZE, gameMap,0,direction.first))
                    nextMove = {direction.first,0};
                else if(isValidG2(ghostX/CELL_SIZE, ghostY/CELL_SIZE + (rando), gameMap,1,rando))
                    nextMove = {0,rando};
                else if(isValidG2(ghostX/CELL_SIZE, ghostY/CELL_SIZE + (-rando), gameMap,1,-rando))
                    nextMove = {0,-rando};
                else
                    nextMove =  {-direction.first,0};
                pthread_mutex_unlock(&gameMapMutex);
            }
            else
            {
                while(rando == 0)
                    rando = ((rand()%2)-(rand()%2));
                pthread_mutex_lock(&gameMapMutex);
                if(isValidG2(ghostX/CELL_SIZE + (rando), ghostY/CELL_SIZE, gameMap,0,rando) && !isValidG2(ghostX/CELL_SIZE + (rando), ghostY/CELL_SIZE + (rando), gameMap,-1,rando,rando) && !isValidG2(ghostX/CELL_SIZE + (rando), ghostY/CELL_SIZE + (-rando), gameMap,-1,rando,-rando))
                    nextMove = {rando,0};
                else if(isValidG2(ghostX/CELL_SIZE + (rando*-1), ghostY/CELL_SIZE, gameMap,0,(-rando)) && !isValidG2(ghostX/CELL_SIZE + (rando*-1), ghostY/CELL_SIZE + (rando), gameMap,-1,-rando,rando) && !isValidG2(ghostX/CELL_SIZE + (rando*-1), ghostY/CELL_SIZE + (-rando), gameMap,-1,-rando,-rando))
                    nextMove = {-rando,0};
                else if(isValidG2(ghostX/CELL_SIZE, ghostY/CELL_SIZE + direction.second, gameMap,1,direction.second))
                    nextMove = {0,direction.second};
                else if(isValidG2(ghostX/CELL_SIZE + (rando), ghostY/CELL_SIZE, gameMap,0,rando))
                    nextMove = {rando,0};
                else if(isValidG2(ghostX/CELL_SIZE + (-rando), ghostY/CELL_SIZE, gameMap,0,-rando))
                    nextMove = {-rando,0};
                else
                    nextMove =  {0,-direction.second};
                pthread_mutex_unlock(&gameMapMutex);
            }
            if(!timeFlag1)
            {
                ghost_shape->setScale(1,1);
                offset = {25/8,25/4};
                timeFlag1 = true;
            }
            changeEyes2(direction,ghost_shape,ghostTexture,gNum);
            direction = nextMove;
            nextMoveX = ghostX + (nextMove.first);
            nextMoveY = ghostY + (nextMove.second);
            if((round(pacX) == round(ghostX/CELL_SIZE) && round(pacY) == round(ghostY/CELL_SIZE)) || (round(pacX) == round(nextMoveX/CELL_SIZE) && round(pacY) == (nextMoveY/CELL_SIZE)))
            {
                pthread_mutex_lock(&livesResetMutex);
                if(!lives_reset)
                {
                    pthread_mutex_lock(&livesMutex);
                    lives--; // Decrement lives
                    pthread_mutex_unlock(&livesMutex);
                }
                pthread_mutex_unlock(&livesResetMutex);
                if (lives > 0) 
                {
                    pthread_mutex_lock(&livesResetMutex);
                    lives_reset = true;
                    pthread_mutex_unlock(&livesResetMutex);
                }
            }
            ghostX = nextMoveX;
            ghostY = nextMoveY;
        }
        else
        {
            
            pair<int,int> pacPoint = findFurthestPoint(pacX, pacY);
            nextMove = findNextMove(gameMap, (ghostX / CELL_SIZE), (ghostY / CELL_SIZE), pacPoint.first, pacPoint.second);
            nextMoveX = nextMove.first;
            nextMoveY = nextMove.second;
            if(timeFlag1)
            {
                ghostTexture.loadFromFile("img/a.png");
                ghost_shape->setTexture(ghostTexture);
                ghost_shape->setScale(1.7,1.7);
                offset = {-7.5,-10};
                timeFlag1 = false;
            }
            if((round(pacX) == round(ghostX/CELL_SIZE) && round(pacY) == round(ghostY/CELL_SIZE)) || (round(pacX) == round(nextMoveX/CELL_SIZE) && round(pacY) == (nextMoveY/CELL_SIZE)))
            {
                (gNum == 2) ? pthread_mutex_unlock(&ghost2Mutex) : pthread_mutex_unlock(&ghost4Mutex);
                resetPositions(gNum);
                pthread_mutex_lock(&scoreMutex);
                score += 200; // Increment score
                pthread_mutex_unlock(&scoreMutex);
                changeEyes2(direction,ghost_shape,ghostTexture,gNum);
                ghost_shape->setScale(1,1);
                offset = {25/8,25/4};
                ghost_shape->setPosition(ghostX + offset.first, ghostY + offset.second);
                pthread_mutex_lock(&livesResetMutex);
                allReset++;
                pthread_mutex_unlock(&livesResetMutex);
                pos = 25;
                flag = 0;
                houseWait(clock, ghost_shape,pos,flag);
                pthread_mutex_lock(&afraidMutex);
                if(afraid)
                    timeFlag1 = true;
                else
                    timeFlag1 = false;
                pthread_mutex_unlock(&afraidMutex);
                continue;
            
            }
            ghostX = (ceil(nextMoveX) > (ghostX/CELL_SIZE)) ? ghostX + 1 : (ceil(nextMoveX) < (ghostX/CELL_SIZE)) ? ghostX - 1 : ghostX;
            ghostY = (ceil(nextMoveY) > (ghostY/CELL_SIZE)) ? ghostY + 1 : (ceil(nextMoveY) < (ghostY/CELL_SIZE)) ? ghostY - 1 : ghostY;
        }
        ghost_shape->setPosition(ghostX + offset.first, ghostY + offset.second);
        (gNum == 2) ? pthread_mutex_unlock(&ghost2Mutex) : pthread_mutex_unlock(&ghost4Mutex);
        usleep(delay); // Sleep for 0.5 seconds
        pthread_mutex_lock(&livesResetMutex);
        if(lives_reset)
        {
            cout<<"Resetting"<<endl;
            resetPositions(gNum);
            ghost_shape->setPosition(ghostX + 25/8, ghostY + 25/4);
            allReset++;
            pthread_mutex_unlock(&livesResetMutex);
            pos = 25;
            flag = 0;
            houseWait(clock, ghost_shape,pos,flag);
            pthread_mutex_lock(&afraidMutex);
            if(afraid)
                timeFlag1 = true;
            else
                timeFlag1 = false;
            pthread_mutex_unlock(&afraidMutex);
            continue;
        }
        pthread_mutex_unlock(&livesResetMutex);
    }
    pthread_exit(NULL);
}

void resetKeys()
{
    pthread_mutex_lock(&inputMutex);
    userInputKey = Keyboard::Unknown;
    pthread_mutex_unlock(&inputMutex);
}
void restartLives(sf::Clock& clock,bool& flag)
{
    pthread_mutex_lock(&livesResetMutex);
    pthread_mutex_lock(&countMutex);
    if(allReset >= countG + 1)
    {
        allReset = 0;
        clock.restart();
        flag = 1;
        lives_reset = false;
        pthread_mutex_lock(&acquiredMutex);
        acquired = true;
        pthread_mutex_unlock(&acquiredMutex);
    }
    pthread_mutex_unlock(&countMutex);
    pthread_mutex_unlock(&livesResetMutex);
}
//draw lives
void drawLives(sf::RenderWindow& window, sf::Sprite& heartSprite)
{
    pthread_mutex_lock(&livesMutex);
    switch (lives)
    {
    case 3:
        heartSprite.setPosition(10, 830);
        
        window.draw(heartSprite);
        heartSprite.setPosition(40, 830);
        window.draw(heartSprite);
        heartSprite.setPosition(70, 830);
        window.draw(heartSprite);
        break;
    case 2:
        heartSprite.setPosition(10, 830);
        window.draw(heartSprite);
        heartSprite.setPosition(40, 830);
        window.draw(heartSprite);
        break;
    case 1:
        heartSprite.setPosition(10, 830);
        window.draw(heartSprite);
        break;
    case 0:
    //Handle game over logic
        break;
    }
    pthread_mutex_unlock(&livesMutex);
}
// Main function
int main()
{
    // Initialize random seed
    srand(time(nullptr));

    // Create SFML window for menu
    sf::RenderWindow menuWindow(sf::VideoMode(800, 800), "Menu");

    // Load menu backgceil texture from a PNG file
    sf::Texture menuTexture;
    if (!menuTexture.loadFromFile("img/menu.png"))
    {
        // Handle loading error
        std::cerr << "Failed to load menu backgceil texture!" << std::endl;
        return 1; // Exit the program or handle the error appropriately
    }

    // Create menu backgceil sprite
    sf::Sprite menuBackgceil(menuTexture);

    // Load font for menu text
    sf::Font menuFont;
    menuFont.loadFromFile("font/pixelmix.ttf"); // Change the file path as needed

    // Create menu text
    sf::Text menuText;
    menuText.setFont(menuFont);
    menuText.setCharacterSize(32);
    menuText.setFillColor(sf::Color::White);
    menuText.setString("Press Enter to Start");
    menuText.setPosition(280, 300);

    // Menu loop
    while (menuWindow.isOpen())
    {
        sf::Event event;
        while (menuWindow.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                menuWindow.close();
                return 0;
            }
            else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter)
                menuWindow.close();
        }

        menuWindow.clear();

        // Draw menu backgceil first
        menuWindow.draw(menuBackgceil);

        // Draw menu text on top of the backgceil
        menuWindow.draw(menuText);

        menuWindow.display();
    }

    // Close menu and start game
    menuWindow.close();

    // Initialize game board
    initializeGameBoard();
    // Create SFML window for the game
    sf::RenderWindow window(sf::VideoMode(800, 900), "SFML window");
    //2 speed boosts
    sem_init(&speed, 0, 2);

    //sem_timedwait will be used for speed boost to make sure ghost contniues its movement
    //priority queue will be used to prioritize the ghosts
    
    // Pacman Shape
    Texture pacmanTexture;
    if (!pacmanTexture.loadFromFile("img/pacman.png"))
    {
        // Handle loading error
        std::cerr << "Failed to load pacman texture!" << std::endl;
        return 1; // Exit the program or handle the error appropriately
    }
    sf::Sprite pacman_shape(pacmanTexture);
    pacman_shape.setOrigin(25/2, 25/2);
    pthread_mutex_lock(&pacmanMutex);
    pacman_shape.setPosition(pacman_x, pacman_y);
    pthread_mutex_unlock(&pacmanMutex);

    // Ghost Sprite
    sf::Texture ghostTexture1;
    if (!ghostTexture1.loadFromFile("img/ghost1_1.png"))
    {
        // Handle loading error
        std::cerr << "Failed to load ghost texture!" << std::endl;
        return 1; // Exit the program or handle the error appropriately
    }
    sf::Sprite ghost1(ghostTexture1);
    ghost1.setPosition(CELL_SIZE * 11, CELL_SIZE * 13);
    ghost1.setScale(1.1, 1.1);

    // Ghost Sprite 2
    sf::Texture ghostTexture2;
    if (!ghostTexture2.loadFromFile("img/ghost2_1.png"))
    {
        // Handle loading error
        std::cerr << "Failed to load ghost texture!" << std::endl;
        return 1; // Exit the program or handle the error appropriately
    }
    sf::Sprite ghost2(ghostTexture2);
    ghost2.setPosition(CELL_SIZE * 11, CELL_SIZE * 13);
    ghost2.setScale(1.1, 1.1);

    // Ghost Sprite 3
    sf::Texture ghostTexture3;
    if (!ghostTexture3.loadFromFile("img/ghost3_1.png"))
    {
        // Handle loading error
        std::cerr << "Failed to load ghost texture!" << std::endl;
        return 1; // Exit the program or handle the error appropriately
    }
    sf::Sprite ghost3(ghostTexture3);
    ghost3.setPosition(CELL_SIZE * 11, CELL_SIZE * 13);
    ghost3.setScale(1.1, 1.1);

    // Ghost Sprite 4
    sf::Texture ghostTexture4;
    if (!ghostTexture4.loadFromFile("img/ghost4_1.png"))
    {
        // Handle loading error
        std::cerr << "Failed to load ghost texture!" << std::endl;
        return 1; // Exit the program or handle the error appropriately
    }
    sf::Sprite ghost4(ghostTexture4);
    ghost4.setPosition(CELL_SIZE * 11, CELL_SIZE * 13);
    ghost4.setScale(1.1, 1.1);

    // Load font file for score display
    sf::Font font;
    font.loadFromFile("font/pixelmix.ttf"); // Change the file path as needed

    // Create score text
    sf::Text scoreText;
    scoreText.setFont(font);
    scoreText.setCharacterSize(32);
    scoreText.setFillColor(sf::Color::White);
    scoreText.setPosition(10, 850);
    //placeholder for multiple arguments
    // Create thread for user input
    pthread_t userInputThread;
    pthread_create(&userInputThread, nullptr, userInput, &window);

    // Create thread for movement
    pthread_t moveThread;
    pthread_create(&moveThread, nullptr, (void* (*)(void*))movePacman, &pacman_shape);

    //Create thread for ghost 1 movement
    pthread_t ghostThread;
    void* args[2];
    int gNo = 1;
    args[0] = &gNo;
    args[1] = &ghost1;
    pthread_create(&ghostThread, nullptr, (void* (*)(void*))moveGhost1, args);

    //Create thread for ghost 2 movement
    pthread_t ghostThread2;
    void* args2[2];
    int gNo2 = 2;
    args2[0] = &gNo2;
    args2[1] = &ghost2;
    pthread_create(&ghostThread2, nullptr, (void* (*)(void*))moveGhost2, args2);

    // Create thread for ghost 3 movement
    pthread_t ghostThread3;
    int gNo3 = 3;
    void* args3[2];
    args3[0] = &gNo3;
    args3[1] = &ghost3;
    pthread_create(&ghostThread3, nullptr, (void* (*)(void*))moveGhost1, args3);

    // Create thread for ghost 4 movement
    pthread_t ghostThread4;
    int gNo4 = 4;
    void* args4[2];
    args4[0] = &gNo4;
    args4[1] = &ghost4;
    pthread_create(&ghostThread4, nullptr, (void* (*)(void*))moveGhost2, args4);
    Clock clock;
    bool flag = 1;
    bool flag2 = 1;

    // Load heart texture
    sf::Texture heartTexture;
    if (!heartTexture.loadFromFile("img/heart.png"))
    {
        // Handle loading error
        std::cerr << "Failed to load heart texture!" << std::endl;
        return 1; // Exit the program or handle the error appropriately
    }
    // Create heart sprite
    sf::Sprite heartSprite(heartTexture);
    heartSprite.setPosition(10, 800);

    // Main loop
    while (window.isOpen())
    {
        // Restart lives if all ghosts have been reset
        restartLives(clock,flag2);
        //start wait for ghosts 5 seconds
        if(flag2)
            startWait(clock,flag2);
        // Clear, draw, and display
        window.clear();
        drawGrid(window);

        // Update and display score
        pthread_mutex_lock(&scoreMutex);
        scoreText.setString("Score: " + std::to_string(score));
        pthread_mutex_unlock(&scoreMutex);
        window.draw(scoreText);
        window.draw(ghost1); // Draw the ghost
        window.draw(ghost2); // Draw the ghost
        window.draw(ghost3); // Draw the ghost
        window.draw(ghost4); // Draw the ghost
        window.draw(pacman_shape);                     // Draw the player (yellow circle)
        drawLives(window, heartSprite);
            sf::Text levelText;
        levelText.setFont(font);
        levelText.setCharacterSize(24);
        levelText.setFillColor(sf::Color::White);
        levelText.setString("Level: " + std::to_string(currentLevel));
        levelText.setPosition(300, 850);
        window.draw(levelText);


        // Check for level change
        handleLevelChange();

        //check for if a ghost has aquired both key and permit
        if(!flag2)
            resetAquired(clock,flag);

        window.display();
        pthread_mutex_lock(&closedMutex);
        if(closed)
        {
            window.close();
        }
        pthread_mutex_unlock(&closedMutex);
    }

    // Join threads
    pthread_join(userInputThread, nullptr);
    pthread_join(moveThread, nullptr);
    pthread_join(ghostThread, nullptr);
    pthread_join(ghostThread2, nullptr);
    pthread_join(ghostThread3, nullptr);
    pthread_join(ghostThread4, nullptr);
    // Destroy mutexes
    pthread_mutex_destroy(&inputMutex);
    pthread_mutex_destroy(&pacmanMutex);
    pthread_mutex_destroy(&gameMapMutex);
    pthread_mutex_destroy(&closedMutex);
    pthread_mutex_destroy(&afraidMutex);
    pthread_mutex_destroy(&acquiredMutex);
    pthread_mutex_destroy(&speedMutex);
    pthread_mutex_destroy(&keyMutex);
    pthread_mutex_destroy(&permitMutex);
    pthread_mutex_destroy(&countMutex);
    pthread_mutex_destroy(&livesResetMutex);
    pthread_mutex_destroy(&ghost1Mutex);
    pthread_mutex_destroy(&ghost2Mutex);
    pthread_mutex_destroy(&ghost3Mutex);
    pthread_mutex_destroy(&ghost4Mutex);
    pthread_mutex_destroy(&livesMutex);
    // Destroy semaphores
    sem_destroy(&speed);
    return 0;
}