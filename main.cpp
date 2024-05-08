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
#include <stack>
#include <climits>

// Define game board size
#define ROWS 25
#define COLS 25
#define CELL_SIZE 32 // Size of each cell in pixels

using namespace std;
using namespace sf;
//semaphores for key and permit for ghosts
sem_t key;
sem_t permit;
//semaphore for speed boosts
sem_t speed;

// Define game entities
// Define game grid
int gameMap[ROWS][COLS] = {0};
int score = 0;
//bool afraid to check if ghost is afraid
bool afraid = false;
bool closed = false;
// Mutex to protect user input
pthread_mutex_t inputMutex = PTHREAD_MUTEX_INITIALIZER;
// Mutex to protect pacman position
pthread_mutex_t pacmanMutex = PTHREAD_MUTEX_INITIALIZER;
// Mutex for game map
pthread_mutex_t gameMapMutex = PTHREAD_MUTEX_INITIALIZER;
//mutex for closed
pthread_mutex_t closedMutex = PTHREAD_MUTEX_INITIALIZER;

// Shared variable for user input
Keyboard::Key userInputKey = Keyboard::Unknown;

//pacman coordinates
int pacman_x = CELL_SIZE + 25 / 8;
int pacman_y = CELL_SIZE + 25 / 4;
//ghost coordinates
int ghost1X = CELL_SIZE * 11;
int ghost2X = CELL_SIZE * 12;
int ghost3X = CELL_SIZE * 13;
int ghost4X = CELL_SIZE * 12;
int ghost1Y = CELL_SIZE * 11;
int ghost2Y = CELL_SIZE * 11;
int ghost3Y = CELL_SIZE * 11;
int ghost4Y = CELL_SIZE * 12;
// Function to check if a cell is valid
bool isValid(int x, int y, int gameMap[ROWS][COLS]) {
    return (x >= 0 && x < ROWS && y >= 0 && y < COLS && gameMap[y][x] != -2 && gameMap[y][x] != 1 && gameMap[y][x] != -1);
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
    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLS; j++)
        {
            gameMap[i][j] = 2;
        }
    }
    for (int i = 0; i < ROWS; i++)
    {
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
                    gameMap[i][j] = 3 + rand() % 2; // Randomly assign power-up value (2, 3, or 4)
                }
                else
                {
                    gameMap[i][j] = 2;
                }
            }
        }
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
                exit(1);
            }
            else if (event.type == Event::KeyPressed)
            {
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
        int nextX, nextY;
        pthread_mutex_lock(&pacmanMutex);
        nextX = pacman_x + pacman_direction_x * CELL_SIZE;
        nextY = pacman_y + pacman_direction_y * CELL_SIZE;
        pthread_mutex_unlock(&pacmanMutex);

        // Check if the next position is a wall
        pthread_mutex_lock(&gameMapMutex);
        bool valid = isValid(nextX / CELL_SIZE, nextY / CELL_SIZE, gameMap);
        pthread_mutex_unlock(&gameMapMutex);
        if (!valid)
        {
            // If it is a wall, do not update the position
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
        pthread_mutex_lock(&pacmanMutex);
        pacman_x += pacman_direction_x * CELL_SIZE;
        pacman_y += pacman_direction_y * CELL_SIZE;
        nextX = pacman_x;
        nextY = pacman_y;
        pthread_mutex_unlock(&pacmanMutex);
        pacman_shape->setPosition(nextX + 25/2, nextY + 25/2); // Update pacman position
        pacman_shape->setRotation(rotation);
        pthread_mutex_lock(&gameMapMutex);
        int value = gameMap[nextY / CELL_SIZE][nextX / CELL_SIZE];
        pthread_mutex_unlock(&gameMapMutex);
        if ( value == 2 || value == 3)
        {
            // Handle scoring when encountering red (2) or white (3) balls
            pthread_mutex_lock(&gameMapMutex);
            int ballValue = gameMap[nextY / CELL_SIZE][nextX / CELL_SIZE];
            pthread_mutex_unlock(&gameMapMutex);
            if (ballValue != 0)
            { // Check if the ball hasn't been consumed already
                score += ballValue;
                // Update the game grid to mark the ball as consumed
                pthread_mutex_lock(&gameMapMutex);
                gameMap[nextY / CELL_SIZE][nextX / CELL_SIZE] = 0;
                pthread_mutex_unlock(&gameMapMutex);
            }
        }
        usleep(180000); // Sleep for 0.3 seconds
    }
}
int shortestPath(int startX, int startY, int destX, int destY, int gameMap[ROWS][COLS]) {
    bool visited[ROWS][COLS] = {false};
    std::queue<std::pair<int, int>> q;
    q.push({startX, startY});
    visited[startX][startY] = true;

    int dx[] = {0, 0, 1, -1};
    int dy[] = {1, -1, 0, 0};
    int dist[ROWS][COLS] = {0}; // Store distances from start position

    while (!q.empty()) {
        int x = q.front().first;
        int y = q.front().second;
        q.pop();

        // Check if destination is reached
        if (x == destX && y == destY) {
            return dist[x][y];
        }

        // Explore adjacent cells
        for (int i = 0; i < 4; ++i) {
            int newX = x + dx[i];
            int newY = y + dy[i];

            // Check validity and unvisited status
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
std::pair<int, int> findNextMove(int gameMap[ROWS][COLS], int ghostX, int ghostY, int pacmanX, int pacmanY) {

    int dx[] = {0, 0, 1, -1};
    int dy[] = {1, -1, 0, 0};

    std::pair<std::pair<int, int>, int> nextMove[4];

    for (int i = 0; i < 4; i++) {
        int x = ghostX + dx[i];
        int y = ghostY + dy[i];
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

// Function for ghost movement
void moveGhost1(void* arg) { // smart movement
    void ** args = (void**)arg;
    int* gN0 = (int*)args[0];
    int gNum = *gN0;
    int& ghostX = (gNum == 1 ? ghost1X : ghost3X);
    int& ghostY = (gNum == 1 ? ghost1Y : ghost3Y);
    sf::Sprite* ghost_shape = (sf::Sprite*)args[1];
    sf::Texture ghostTexture;
    sf::Clock clock;
    ghost_shape->setPosition(ghostX + 25/8, ghostY + 25/4); // Adjust position based on sprite size
    // bob up and down while waiting for key
    int pos = 25;
    while(1)
    {
        bob(clock, ghost_shape,pos);
    }
    ghost_shape->setPosition(ghostX + 25/8, ghostY + 25/4); // Adjust position based on sprite size
    while(1)
    {
        pthread_mutex_lock(&closedMutex);
        if(closed)
        {
            pthread_mutex_unlock(&closedMutex);
            break;
        }
        pthread_mutex_unlock(&closedMutex);
        pthread_mutex_lock(&pacmanMutex);
        int pacX = pacman_x / CELL_SIZE;
        int pacY = pacman_y / CELL_SIZE;
        pthread_mutex_unlock(&pacmanMutex);

        int diffX = pacX - ghostX / CELL_SIZE;
        int diffY = pacY - ghostY / CELL_SIZE;
        //change texture to look at pacman
        changeEyes(ghostTexture,ghost_shape,diffX,diffY,gNum);

        std::pair<int, int> nextMove = findNextMove(gameMap, ghostX / CELL_SIZE, ghostY / CELL_SIZE, pacX, pacY);
        ghostX = nextMove.first * CELL_SIZE;
        ghostY = nextMove.second* CELL_SIZE;
        ghost_shape->setPosition(ghostX + 25/8, ghostY + 25/4);
        usleep(400000); // Sleep for 0.3 seconds
    }
    pthread_exit(NULL);
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
void moveGhost2(void* arg) 
{ // random movement with direction persistence
    void** args = (void**)arg;
    int* gN0 = (int*)args[0];
    int gNum = *gN0;
    cout<<gNum<<endl;
    int &ghostX = (gNum == 2 ? ghost2X : ghost4X);
    int &ghostY = (gNum == 2 ? ghost2Y : ghost4Y);
    sf::Sprite* ghost_shape = (sf::Sprite*)args[1];
    sf::Texture ghostTexture;
    pair<int,int> direction =  {1,0}; // Random initial direction
    sf::Clock clock;
    ghost_shape->setPosition(ghostX + 25/8, ghostY + 25/4); // Adjust position based on sprite size
    int pos = 25;
    while(1)
    {
        bob(clock, ghost_shape,pos);
    }
    while(1)
    {
        pthread_mutex_lock(&closedMutex);
        if(closed)
        {
            pthread_mutex_unlock(&closedMutex);
            break;
        }
        pthread_mutex_unlock(&closedMutex);
        pthread_mutex_lock(&pacmanMutex);
        int pacX = pacman_x / CELL_SIZE;
        int pacY = pacman_y / CELL_SIZE;
        pthread_mutex_unlock(&pacmanMutex);

        int nextMoveX;
        int nextMoveY;

        std::pair<int, int> nextMove = {0, 0};
        //find if any turns

        int rando = ((rand()%2)-(rand()%2));
        if(abs(direction.first) == 1)
        {
            while(rando == 0)
                rando = ((rand()%2)-(rand()%2));
            pthread_mutex_lock(&gameMapMutex);
            if(isValid(ghostX/CELL_SIZE, ghostY/CELL_SIZE + (rando), gameMap) && !isValid(ghostX/CELL_SIZE+(rando), ghostY/CELL_SIZE + (rando), gameMap) && !isValid(ghostX/CELL_SIZE+(-rando), ghostY/CELL_SIZE + (rando), gameMap))
                nextMove = {0,rando};
            else if(isValid(ghostX/CELL_SIZE , ghostY/CELL_SIZE + (rando*-1), gameMap) && !isValid(ghostX/CELL_SIZE+(rando), ghostY/CELL_SIZE + (rando*-1), gameMap) && !isValid(ghostX/CELL_SIZE+(-rando), ghostY/CELL_SIZE + (rando*-1), gameMap))
                nextMove = {0,-rando};
            else if(isValid(ghostX/CELL_SIZE + direction.first, ghostY/CELL_SIZE, gameMap))
                nextMove = {direction.first,0};
            else if(isValid(ghostX/CELL_SIZE, ghostY/CELL_SIZE + (rando), gameMap))
                nextMove = {0,rando};
            else if(isValid(ghostX/CELL_SIZE , ghostY/CELL_SIZE + (rando*-1), gameMap))
                nextMove = {0,-rando};
            else
                nextMove = {-direction.first,0};
            pthread_mutex_unlock(&gameMapMutex);
        }
        else
        {
            while(rando == 0)
                rando = ((rand()%2)-(rand()%2));
            pthread_mutex_lock(&gameMapMutex);
            if(isValid(ghostX/CELL_SIZE  + (rando), ghostY/CELL_SIZE, gameMap) && !isValid(ghostX/CELL_SIZE  + (rando), ghostY/CELL_SIZE + (rando), gameMap) && !isValid(ghostX/CELL_SIZE  + (rando), ghostY/CELL_SIZE + (-rando), gameMap))
                nextMove = {rando,0};
            else if(isValid(ghostX/CELL_SIZE+ (rando*-1), ghostY/CELL_SIZE, gameMap)&& !isValid(ghostX/CELL_SIZE  + (-rando), ghostY/CELL_SIZE + (rando), gameMap) && !isValid(ghostX/CELL_SIZE  + (-rando), ghostY/CELL_SIZE + (-rando), gameMap))
                nextMove = {-rando,0};
            else if(isValid(ghostX/CELL_SIZE, ghostY/CELL_SIZE  + direction.second, gameMap))
                nextMove = {0,direction.second};
            else if(isValid(ghostX/CELL_SIZE  + (rando), ghostY/CELL_SIZE, gameMap))
                nextMove = {rando,0};
            else if(isValid(ghostX/CELL_SIZE+ (rando*-1), ghostY/CELL_SIZE, gameMap))
                nextMove = {-rando,0};
            else
                nextMove =  {0,-direction.second};
            pthread_mutex_unlock(&gameMapMutex);
        }
        nextMoveX = (nextMove.first + ghostX/CELL_SIZE) * CELL_SIZE;
        nextMoveY = (nextMove.second + ghostY/CELL_SIZE) * CELL_SIZE;
        direction = nextMove;
        //change texture to look at forward
        changeEyes2(direction,ghost_shape,ghostTexture,gNum);
        ghostX = nextMoveX;
        ghostY = nextMoveY;
        ghost_shape->setPosition(ghostX + 25/8, ghostY + 25/4);
        usleep(500000); // Sleep for 0.5 seconds
    }
    pthread_exit(NULL);
}



// Main function
int main()
{
    // Initialize random seed
    srand(time(nullptr));

    // Create SFML window for menu
    sf::RenderWindow menuWindow(sf::VideoMode(800, 800), "Menu");

    // Load menu background texture from a PNG file
    sf::Texture menuTexture;
    if (!menuTexture.loadFromFile("img/menu.png"))
    {
        // Handle loading error
        std::cerr << "Failed to load menu background texture!" << std::endl;
        return 1; // Exit the program or handle the error appropriately
    }

    // Create menu background sprite
    sf::Sprite menuBackground(menuTexture);

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
                menuWindow.close();
            else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter)
                menuWindow.close();
        }

        menuWindow.clear();

        // Draw menu background first
        menuWindow.draw(menuBackground);

        // Draw menu text on top of the background
        menuWindow.draw(menuText);

        menuWindow.display();
    }

    // Close menu and start game
    menuWindow.close();

    // Initialize game board
    initializeGameBoard();
    // Create SFML window for the game
    sf::RenderWindow window(sf::VideoMode(800, 900), "SFML window");
    //2 keys and 2 permits for ghosts
    sem_init(&key, 0, 2);
    sem_init(&permit, 0, 2);
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
    pacman_shape.setPosition(pacman_x, pacman_y);

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

    // Main loop
    while (window.isOpen())
    {
        // Clear, draw, and display
        window.clear();
        drawGrid(window);
        window.draw(pacman_shape);                     // Draw the player (yellow circle)

        // Update and display score
        scoreText.setString("Score: " + std::to_string(score));
        window.draw(scoreText);
        window.draw(ghost1); // Draw the ghost
        window.draw(ghost2); // Draw the ghost
        window.draw(ghost3); // Draw the ghost
        window.draw(ghost4); // Draw the ghost

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
    // Destroy mutexes
    pthread_mutex_destroy(&inputMutex);
    pthread_mutex_destroy(&pacmanMutex);
    pthread_mutex_destroy(&gameMapMutex);
    pthread_mutex_destroy(&closedMutex);

    return 0;
}