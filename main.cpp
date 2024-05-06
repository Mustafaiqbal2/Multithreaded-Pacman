#include <iostream>
#include <cstdlib>
#include <ctime>
#include <pthread.h>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <unistd.h>

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

// Mutex to protect user input
pthread_mutex_t inputMutex = PTHREAD_MUTEX_INITIALIZER;
// Mutex to protect pacman position
pthread_mutex_t pacmanMutex = PTHREAD_MUTEX_INITIALIZER;

// Shared variable for user input
Keyboard::Key userInputKey = Keyboard::Unknown;

//pacman coordinates
int pacman_x = CELL_SIZE + 25 / 8;
int pacman_y = CELL_SIZE + 25 / 4;

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
                powerUpShape.setPosition(j * CELL_SIZE + CELL_SIZE / 2 - 8, i * CELL_SIZE + CELL_SIZE / 2 - 8);
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
            // Openings
            else if (i == 12 || j == 12)
            {
                gameMap[i][j] = 0;
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
                    gameMap[i][j] = 2 + rand() % 3; // Randomly assign power-up value (2, 3, or 4)
                }
                else
                {
                    gameMap[i][j] = 0;
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
void movePacman()
{
    // Initial movement direction
    int pacman_direction_x = 0;
    int pacman_direction_y = 0;
    while (true)
    {
        pthread_mutex_lock(&inputMutex);
        // Update movement based on current key
        switch (userInputKey)
        {
        case Keyboard::Up:
            pacman_direction_x = 0;
            pacman_direction_y = -1;
            break;
        case Keyboard::Down:
            pacman_direction_x = 0;
            pacman_direction_y = 1;
            break;
        case Keyboard::Left:
            pacman_direction_x = -1;
            pacman_direction_y = 0;
            break;
        case Keyboard::Right:
            pacman_direction_x = 1;
            pacman_direction_y = 0;
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
            cout << "Wall detected!" << endl;
        }
        else if (gameMap[nextY / CELL_SIZE][nextX / CELL_SIZE] == 2 || gameMap[nextY / CELL_SIZE][nextX / CELL_SIZE] == 3)
        {
            // Handle scoring when encountering red (2) or white (3) balls
            pthread_mutex_lock(&pacmanMutex);
            int ballValue = gameMap[nextY / CELL_SIZE][nextX / CELL_SIZE];
            if (ballValue != 0)
            { // Check if the ball hasn't been consumed already
                score += ballValue;
                cout << "Score: " << score << endl;
                // Update the game grid to mark the ball as consumed
                gameMap[nextY / CELL_SIZE][nextX / CELL_SIZE] = 0;
            }
            pthread_mutex_unlock(&pacmanMutex);
        }
        else
        {
            // Move pacman
            pthread_mutex_lock(&pacmanMutex);
            pacman_x += pacman_direction_x * CELL_SIZE;
            pacman_y += pacman_direction_y * CELL_SIZE;
            pthread_mutex_unlock(&pacmanMutex);
        }
        usleep(150000); // Sleep for 0.3 seconds
    }
}

int main()
{
    // Initialize random seed
    srand(time(nullptr));
    // Initialize game board
    initializeGameBoard();

    // Create SFML window
    sf::RenderWindow window(sf::VideoMode(800, 800), "SFML window");
    // Create the yellow circle (player)
    sf::CircleShape pacman_shape(25 / 2);
    pacman_shape.setFillColor(sf::Color::Yellow);
    pacman_shape.setPosition(25 / 8, 25 / 4); // Set initial position to (100, 50)

    // Load font file
    sf::Font font;
    font.loadFromFile("arial.ttf");

    // Create score text object
    sf::Text scoreText;
    scoreText.setFont(font);
    scoreText.setCharacterSize(24); // Set the character size
    scoreText.setFillColor(sf::Color::White); // Set the text color
    scoreText.setPosition(10, 10); // Set the position of the text

    // Create thread for user input
    pthread_t userInputThread;
    pthread_create(&userInputThread, nullptr, userInput, &window);

    // Create thread for movement
    pthread_t moveThread;
    pthread_create(&moveThread, nullptr, (void* (*)(void*))movePacman, nullptr);

    // Main loop
    while (window.isOpen())
    {
        // Clear, draw, and display
        window.clear();
        drawGrid(window);
        pacman_shape.setPosition(pacman_x, pacman_y); // Update pacman position
        window.draw(pacman_shape); // Draw the player (yellow circle)

        // Update and draw the score text
        scoreText.setString("Score: " + std::to_string(score)); // Convert score to string and set it
        window.draw(scoreText); // Draw the score text

        window.display();
    }

    // Join threads
    pthread_join(userInputThread, nullptr);
    pthread_join(moveThread, nullptr);
    // Destroy mutexes
    pthread_mutex_destroy(&inputMutex);
    pthread_mutex_destroy(&pacmanMutex);

    return 0;
}
