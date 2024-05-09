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
        if(clock.getElapsedTime().asSeconds() >= 10)//another 5 seconds before requesting again
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






// Constants for initial positions
const int INITIAL_PACMAN_X = 100;
const int INITIAL_PACMAN_Y = 100;
const int INITIAL_GHOST1_X = 200;
const int INITIAL_GHOST1_Y = 200;
// Define other initial positions as needed...


// Define variables for Ghost 2
const int INITIAL_GHOST2_X = 300;
const int INITIAL_GHOST2_Y = 300;

// Define variables for Ghost 3
const int INITIAL_GHOST3_X = 400;
const int INITIAL_GHOST3_Y = 400;

// Define variables for Ghost 4
const int INITIAL_GHOST4_X = 500;
const int INITIAL_GHOST4_Y = 500;


// Mutex for ghost positions
pthread_mutex_t ghost1Mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ghost2Mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ghost3Mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ghost4Mutex = PTHREAD_MUTEX_INITIALIZER;

// Function to reset positions of Pacman and the ghosts
void resetPositions() {
    // Reset Pacman's position
    pacman_x = INITIAL_PACMAN_X; // Reset Pacman's x position
    pacman_y = INITIAL_PACMAN_Y; // Reset Pacman's y position

    // Reset ghost 1's position
    pthread_mutex_lock(&ghost1Mutex);
    ghost1X = INITIAL_GHOST1_X; // Reset ghost 1's x position
    ghost1Y = INITIAL_GHOST1_Y; // Reset ghost 1's y position
    pthread_mutex_unlock(&ghost1Mutex);

    // Reset ghost 2's position
    pthread_mutex_lock(&ghost2Mutex);
    ghost2X = INITIAL_GHOST2_X; // Reset ghost 2's x position
    ghost2Y = INITIAL_GHOST2_Y; // Reset ghost 2's y position
    pthread_mutex_unlock(&ghost2Mutex);

    // Reset ghost 3's position
    pthread_mutex_lock(&ghost3Mutex);
    ghost3X = INITIAL_GHOST3_X; // Reset ghost 3's x position
    ghost3Y = INITIAL_GHOST3_Y; // Reset ghost 3's y position
    pthread_mutex_unlock(&ghost3Mutex);

    // Reset ghost 4's position
    pthread_mutex_lock(&ghost4Mutex);
    ghost4X = INITIAL_GHOST4_X; // Reset ghost 4's x position
    ghost4Y = INITIAL_GHOST4_Y; // Reset ghost 4's y position
    pthread_mutex_unlock(&ghost4Mutex);
}




int lives=3;
// Function for ghost movement
void moveGhost1(void* arg) { // smart movement
    void ** args = (void**)arg;
    int* gN0 = (int*)args[0];
    int gNum = *gN0;
    int& ghostX = (gNum == 1 ? ghost1X : ghost3X);
    int& ghostY = (gNum == 1 ? ghost1Y : ghost3Y);
    int priority = (gNum == 1 ? 1 : 0);
    sf::Sprite* ghost_shape = (sf::Sprite*)args[1];
    sf::Texture ghostTexture;
    sf::Clock clock;
    ghost_shape->setPosition(ghostX + 25/8, ghostY + 25/4); // Adjust position based on sprite size
    // bob up and down while waiting for key
    int pos = 25;
    bool flag = 0; // flag to know it has acquired
    bool speed = false;
    int delay = 400000;
    ///////////////////////////////////////////////////////////////////////----------------House
    while(1)
    {
        bob(clock, ghost_shape,pos);
        if(tryAcquire(flag))
        {
            break;
        }
    }
    ///////////////////////////////////////////////////////////////////////----------------Outside
    ghost_shape->setPosition(ghostX + 25/8, ghostY + 25/4); // Adjust position based on sprite size
    while(1)
    {
        if(requestSpeedBoost(gNum,1,speed,clock))
        {
            delay = 200000;
        }
        else
        {
            delay = 400000;
        }
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


           if (pacX == ghostX / CELL_SIZE && pacY == ghostY / CELL_SIZE) 
        {
            lives--; // Decrement lives
            std::cout << "Lives after decrement: " << lives << std::endl;
            // Reset positions if lives are greater than 0
            if (lives > 0) {
                resetPositions(); // Reset positions of Pacman and the ghost
            }
            
        }

        std::pair<int, int> nextMove = findNextMove(gameMap, ghostX / CELL_SIZE, ghostY / CELL_SIZE, pacX, pacY);
        ghostX = nextMove.first * CELL_SIZE;
        ghostY = nextMove.second* CELL_SIZE;
        ghost_shape->setPosition(ghostX + 25/8, ghostY + 25/4);
        usleep(delay); // Sleep for 0.3 seconds
    }
    pthread_exit(NULL);
}
void moveGhost2(void* arg) 
{ // random movement with direction persistence
    void** args = (void**)arg;
    int* gN0 = (int*)args[0];
    int gNum = *gN0;
    int &ghostX = (gNum == 2 ? ghost2X : ghost4X);
    int &ghostY = (gNum == 2 ? ghost2Y : ghost4Y);
    sf::Sprite* ghost_shape = (sf::Sprite*)args[1];
    sf::Texture ghostTexture;
    pair<int,int> direction =  {1,0}; // Random initial direction
    sf::Clock clock;
    ghost_shape->setPosition(ghostX + 25/8, ghostY + 25/4); // Adjust position based on sprite size
    int pos = 25;
    bool flag = 0; // flag to know it has acquired
    bool speed = false;
    int priority = (gNum == 2 ? 2 : 3);
    int delay = 500000;
    ///////////////////////////////////////////////////////////////////////----------------House

    while(1)
    {
        bob(clock, ghost_shape,pos);
        if(tryAcquire(flag))
            break;
    }
    ///////////////////////////////////////////////////////////////////////----------------Outside
    while(1)
    {
        if(requestSpeedBoost(gNum,priority,speed,clock))
        {
            delay = 200000;
        }
        else
        {
            delay = 500000;
        }
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

        if(pacX == ghostX/CELL_SIZE && pacY == ghostY/CELL_SIZE)
        {
            lives--; // Decrement lives
            std::cout << "Lives after decrement: " << lives << std::endl;
            // Reset positions if lives are greater than 0
            if (lives > 0) {
                resetPositions(); // Reset positions of Pacman and the ghost
            }
        }
        usleep(delay); // Sleep for 0.5 seconds
    }
    pthread_exit(NULL);
}

void resetKeys()
{
    pthread_mutex_lock(&inputMutex);
    userInputKey = Keyboard::Unknown;
    pthread_mutex_unlock(&inputMutex);
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
        //start wait for ghosts 5 seconds
        if(flag2)
            startWait(clock,flag2);
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
             window.close();
                break;
       
    }




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
    // Destroy mutexes
    pthread_mutex_destroy(&inputMutex);
    pthread_mutex_destroy(&pacmanMutex);
    pthread_mutex_destroy(&gameMapMutex);
    pthread_mutex_destroy(&closedMutex);
    pthread_mutex_destroy(&afraidMutex);
    pthread_mutex_destroy(&acquiredMutex);
    pthread_mutex_destroy(&speedMutex);
    return 0;
}