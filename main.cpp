#include <iostream>
#include <cstdlib>
#include <ctime>
#include <pthread.h>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Audio.hpp>
#include <semaphore.h>
#include <queue>

// Define game board size
#define ROWS 10
#define COLS 10
#define CELL_SIZE 50 // Size of each cell in pixels

using namespace std;
using namespace sf;

// Define game entities
enum Entity { EMPTY, PACMAN, GHOST, WALL, PELLET, POWER_PELLET };

// Define game grid
int gameMap[ROWS][COLS] = {0};

struct Player {
    int x, y;
    int lives;
};

// Define ghost data
struct Ghost {
    int id;
    int x, y;
    int speed_boosts;
};

// Function to draw the grid with appropriate shapes for pellets, power-ups, and walls
void drawGrid(sf::RenderWindow& window)
{
    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLS; j++)
        {
            // Draw pellets, power-ups, and walls
            switch (gameMap[i][j])
            {
                case 1:
                    // Draw small white circle for pellet
                    sf::CircleShape pelletShape(5);
                    pelletShape.setFillColor(sf::Color::White);
                    pelletShape.setPosition(j * CELL_SIZE + CELL_SIZE / 2 - 5, i * CELL_SIZE + CELL_SIZE / 2 - 5);
                    window.draw(pelletShape);
                    break;
                case 2:
                    // Draw slightly bigger red circle for power-up
                    sf::CircleShape powerUpShape(8);
                    powerUpShape.setFillColor(sf::Color::Red);
                    powerUpShape.setPosition(j * CELL_SIZE + CELL_SIZE / 2 - 8, i * CELL_SIZE + CELL_SIZE / 2 - 8);
                    window.draw(powerUpShape);
                    break;
                case 0:
                    // Draw blue rectangle for wall
                    sf::RectangleShape wallShape(sf::Vector2f(CELL_SIZE, CELL_SIZE));
                    wallShape.setFillColor(sf::Color::Blue);
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
            if ((i == 1 && j == 1) || (i == ROWS - 2 && j == COLS - 2)) {
                // Fixed walls
                gameMap[i][j] = WALL;
            } else {
                // Randomly place pellets and power-ups
                int randNum = rand() % 10; // Generate random number from 0 to 9
                if (randNum == 0) {
                    gameMap[i][j] = POWER_PELLET + rand() % 3; // Randomly assign power-up value (2, 3, or 4)
                } else if (randNum < 4) {
                    gameMap[i][j] = PELLET;
                } else {
                    gameMap[i][j] = EMPTY;
                }
            }
        }
    }
}

// Mutex to protect inputQueue
pthread_mutex_t inputMutex = PTHREAD_MUTEX_INITIALIZER;

// Function to handle user input
void* userInput(void* arg) {

    // Unpack arguments
    void **args = (void**) arg;
    sf::CircleShape* pacman_shape = (sf::CircleShape*) args[0];
    sf::Event* event = (sf::Event*) args[1];
    while (true) 
    {
    // Wait for user input event
        pthread_mutex_lock(&inputMutex);
        if (event->type == sf::Event::Closed)
        {
            // Exit thread if window is closed
            pthread_mutex_unlock(&inputMutex);
            pthread_exit(NULL);
        }
        if (event->type == sf::Event::KeyPressed || event->type == sf::Event::KeyReleased) 
        {
            switch(event->key.code)
            {
                case sf::Keyboard::Up:
                    pacman_shape->move(0, -10);
                    break;
                case sf::Keyboard::Down:
                    pacman_shape->move(0, 10);
                    break;
                case sf::Keyboard::Left:
                    pacman_shape->move(-10, 0);
                    break;
                case sf::Keyboard::Right:
                    pacman_shape->move(10, 0);
                    break;
            }
            event->key.code = sf::Keyboard::Unknown;
        }
        pthread_mutex_unlock(&inputMutex);
        sleep(sf::milliseconds(100));
    }

}


// hahhahahahah
int main() {
    // Initialize random seed
    srand(time(nullptr));
    // Initialize game board
    initializeGameBoard();
    // Create SFML window
    sf::CircleShape pacman_shape(30);
    pacman_shape.setFillColor(sf::Color::Yellow);
    pacman_shape.setPosition(100, 100);

    // Create thread for user input
    pthread_t userInputThread;
    Event event;
    void* args[2];
    args[0] = &pacman_shape;
    args[1] = &event;
    pthread_create(&userInputThread, nullptr, userInput, args);

    // Create SFML window
    sf::RenderWindow window(sf::VideoMode(1000, 800), "SFML window");

    // Initialize game grid

    // Main loop
    while (window.isOpen()) {
        // Process SFML events
        pthread_mutex_lock(&inputMutex);
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed)
                window.close();
        }
        pthread_mutex_unlock(&inputMutex);

        // Clear, draw, and display
        drawGrid(window);
        window.clear();
        window.draw(pacman_shape);
        window.display();
    }

    // Join thread
    pthread_join(userInputThread, nullptr);
    return 0;
}
