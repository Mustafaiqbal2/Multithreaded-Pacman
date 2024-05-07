#include <iostream>
#include <cstdlib>
#include <ctime>
#include <pthread.h>
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

// Define game entities
// Define game grid
int gameMap[ROWS][COLS] = {0};
int score = 0;
//bool afraid to check if ghost is afraid
bool afraid = false;

// Mutex to protect user input
pthread_mutex_t inputMutex = PTHREAD_MUTEX_INITIALIZER;
// Mutex to protect pacman position
pthread_mutex_t pacmanMutex = PTHREAD_MUTEX_INITIALIZER;

// Shared variable for user input
Keyboard::Key userInputKey = Keyboard::Unknown;

//pacman coordinates
int pacman_x = CELL_SIZE + 25 / 8;
int pacman_y = CELL_SIZE + 25 / 4;
//ghost coordinates
int ghost1X = CELL_SIZE * 11;
int ghost1Y = CELL_SIZE * 13;
int ghost2X = CELL_SIZE * 11;
int ghost2Y = CELL_SIZE * 13;

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
            switch (gameMap[i][j])
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
                window->close();
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
        if (abs(gameMap[nextY / CELL_SIZE][nextX / CELL_SIZE]) == 1 || gameMap[nextY / CELL_SIZE][nextX / CELL_SIZE] == -1 || gameMap[nextY / CELL_SIZE][nextX / CELL_SIZE] == -2)
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
        pthread_mutex_unlock(&pacmanMutex);
        pacman_shape->setPosition(pacman_x + 25/2, pacman_y + 25/2); // Update pacman position
        pacman_shape->setRotation(rotation);
        if (gameMap[nextY / CELL_SIZE][nextX / CELL_SIZE] == 2 || gameMap[nextY / CELL_SIZE][nextX / CELL_SIZE] == 3)
        {
            // Handle scoring when encountering red (2) or white (3) balls
            pthread_mutex_lock(&pacmanMutex);
            int ballValue = gameMap[nextY / CELL_SIZE][nextX / CELL_SIZE];
            if (ballValue != 0)
            { // Check if the ball hasn't been consumed already
                score += ballValue;
                // Update the game grid to mark the ball as consumed
                gameMap[nextY / CELL_SIZE][nextX / CELL_SIZE] = 0;
            }
            pthread_mutex_unlock(&pacmanMutex);
        }
        usleep(180000); // Sleep for 0.3 seconds
    }
}
bool isValid(int x, int y, int gameMap[ROWS][COLS]) {
    return (x >= 0 && x < ROWS && y >= 0 && y < COLS && gameMap[y][x] != -2 && gameMap[y][x] != 1 && gameMap[y][x] != -1);
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
            if (isValid(newX, newY, gameMap) && !visited[newX][newY]) {
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
        if (isValid(x, y, gameMap)) {
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
// Function for ghost movement
void moveGhost1(void* arg) { // smart movement
    sf::Sprite* ghost_shape = (sf::Sprite*)arg;
    sf::Texture ghostTexture;
    while(1)
    {
        pthread_mutex_lock(&pacmanMutex);
        int pacX = pacman_x / CELL_SIZE;
        int pacY = pacman_y / CELL_SIZE;
        pthread_mutex_unlock(&pacmanMutex);
        //change texture to look at pacman
        int diffX = pacX - ghost1X / CELL_SIZE;
        int diffY = pacY - ghost1Y / CELL_SIZE;
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
        std::pair<int, int> nextMove = findNextMove(gameMap, ghost1X / CELL_SIZE, ghost1Y / CELL_SIZE, pacX, pacY);
        ghost1X = nextMove.first * CELL_SIZE;
        ghost1Y = nextMove.second* CELL_SIZE;
        ghost_shape->setPosition(ghost1X + 25/8, ghost1Y + 25/4);
        usleep(200000); // Sleep for 0.3 seconds
    }
    pthread_exit(NULL);
}
void moveGhost2(void* arg) { // random movement

    sf::Sprite* ghost_shape = (sf::Sprite*)arg;
    Texture ghostTexture;
    while(1)
    {
        pthread_mutex_lock(&pacmanMutex);
        int pacX = pacman_x / CELL_SIZE;
        int pacY = pacman_y / CELL_SIZE;
        pthread_mutex_unlock(&pacmanMutex);
        //change texture to look at pacman
        int diffX = pacX - ghost2X / CELL_SIZE;
        int diffY = pacY - ghost2Y / CELL_SIZE;
        if(abs(diffX) > abs(diffY))
        {
            if(diffX > 0)
            {
                if (!ghostTexture.loadFromFile("img/ghost2_1.png"))
                {
                    // Handle loading error
                    std::cerr << "Failed to load ghost texture!" << std::endl;
                    return; // Exit the program or handle the error appropriately
                }
                ghost_shape->setTexture(ghostTexture);
            }
            else
            {
                if (!ghostTexture.loadFromFile("img/ghost2_4.png"))
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
                if (!ghostTexture.loadFromFile("img/ghost2_3.png"))
                {
                    // Handle loading error
                    std::cerr << "Failed to load ghost texture!" << std::endl;
                    return; // Exit the program or handle the error appropriately
                }
                ghost_shape->setTexture(ghostTexture);
            }
            else
            {
                if (!ghostTexture.loadFromFile("img/ghost2_2.png"))
                {
                    // Handle loading error
                    std::cerr << "Failed to load ghost texture!" << std::endl;
                    return; // Exit the program or handle the error appropriately
                }
                ghost_shape->setTexture(ghostTexture);
            }
        }
        int nextMoveX;
        int nextMoveY;
        do
        {

            std::pair<int, int> nextMove = {rand() % 2 - rand()%2, rand() % 2  - rand()%2};
            if(abs(nextMove.first) + abs(nextMove.second) != 1)
            {
                continue;
            }
            nextMoveX = (nextMove.first + ghost2X/CELL_SIZE) * CELL_SIZE;
            nextMoveY = (nextMove.second + ghost2Y/CELL_SIZE)* CELL_SIZE;
            cout << nextMoveX / CELL_SIZE << " " << nextMoveY / CELL_SIZE << endl;
        }
        while(!isValid( nextMoveX/CELL_SIZE,nextMoveY/CELL_SIZE,gameMap));
        ghost2X = nextMoveX;
        ghost2Y = nextMoveY;
        ghost_shape->setPosition(ghost2X + 25/8, ghost2Y + 25/4);
        usleep(500000); // Sleep for 0.3 seconds
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

    // Load font file for score display
    sf::Font font;
    font.loadFromFile("font/pixelmix.ttf"); // Change the file path as needed

    // Create score text
    sf::Text scoreText;
    scoreText.setFont(font);
    scoreText.setCharacterSize(32);
    scoreText.setFillColor(sf::Color::White);
    scoreText.setPosition(10, 850);

    // Create thread for user input
    pthread_t userInputThread;
    pthread_create(&userInputThread, nullptr, userInput, &window);

    // Create thread for movement
    pthread_t moveThread;
    pthread_create(&moveThread, nullptr, (void* (*)(void*))movePacman, &pacman_shape);

    //Create thread for ghost 1 movement
    pthread_t ghostThread;
    pthread_create(&ghostThread, nullptr, (void* (*)(void*))moveGhost1, &ghost1);

    //Create thread for ghost 2 movement
    pthread_t ghostThread2;
    pthread_create(&ghostThread2, nullptr, (void* (*)(void*))moveGhost2, &ghost2);

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

        window.display();
    }

    // Join threads
    pthread_join(userInputThread, nullptr);
    pthread_join(moveThread, nullptr);
    pthread_join(ghostThread, nullptr);
    pthread_join(ghostThread2, nullptr);
    // Destroy mutexes
    pthread_mutex_destroy(&inputMutex);
    pthread_mutex_destroy(&pacmanMutex);

    return 0;
}