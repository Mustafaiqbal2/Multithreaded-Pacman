#include <iostream>
#include <cstdlib>
#include <ctime>
#include <pthread.h>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <semaphore.h>

// Define game board size
#define ROWS 25
#define COLS 25
#define CELL_SIZE 32 // Size of each cell in pixels

using namespace std;
using namespace sf;

// Define game entities
// Define game grid
int gameMap[ROWS][COLS] = {0};

// Semaphore for event synchronization
sem_t eventSemaphore;

// Mutex to protect user input
pthread_mutex_t inputMutex = PTHREAD_MUTEX_INITIALIZER;

// Shared variable for user input
Keyboard::Key userInputKey = Keyboard::Unknown;

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
            if ((i == 0 || j == 0) || (i == ROWS - 1 || j == COLS - 1)) {
                // Fixed walls
                gameMap[i][j] = 1;
            } else {
                // Randomly place pellets and power-ups
                int randNum = rand() % 5; // Generate random number from 0 to 4
                if (randNum == 2) {
                    gameMap[i][j] = 2 + rand() % 3; // Randomly assign power-up value (2, 3, or 4)
                } else {
                    gameMap[i][j] = 0;
                }
            }
        }
    }
}

// Function to handle user input
void* userInput(void* arg) {
    // Unpack arguments
    sf::RenderWindow* window = (sf::RenderWindow*) arg;

    while (window->isOpen()) {
        Event event;
        // Process SFML events
        while (window->pollEvent(event)) {
            if (event.type == Event::Closed)
                window->close();
            else if (event.type == Event::KeyPressed) {
                // Lock mutex before accessing shared variable
                pthread_mutex_lock(&inputMutex);
                userInputKey = event.key.code;
                // Unlock mutex after updating shared variable
                pthread_mutex_unlock(&inputMutex);
                // Signal the semaphore to wake up the main event loop
                sem_post(&eventSemaphore);
            }
        }
    }

    pthread_exit(NULL);
}

int main() {
    // Initialize random seed
    srand(time(nullptr));
    // Initialize game board
    initializeGameBoard();

    // Create SFML window
    sf::RenderWindow window(sf::VideoMode(800, 800), "SFML window");

    // Initialize the event semaphore
    sem_init(&eventSemaphore, 0, 0);

    // Create thread for user input
    pthread_t userInputThread;
    pthread_create(&userInputThread, nullptr, userInput, &window);

    // Create the yellow circle (player)
    sf::CircleShape pacman_shape(25);
    pacman_shape.setFillColor(sf::Color::Yellow);
    pacman_shape.setPosition(400, 400); // Set initial position

    // Main loop
    while (window.isOpen()) {
        // Wait for the semaphore to be signaled
        sem_wait(&eventSemaphore);

        // Lock mutex before accessing shared variable
        pthread_mutex_lock(&inputMutex);
        Keyboard::Key currentKey = userInputKey;
        userInputKey = Keyboard::Unknown; // Reset input key
        // Unlock mutex after accessing shared variable
        pthread_mutex_unlock(&inputMutex);

        // Process user input
        if (currentKey != Keyboard::Unknown) {
            // Move the circle based on user input
            switch (currentKey) {
                case Keyboard::Up:
                    pacman_shape.move(0, -CELL_SIZE);
                    break;
                case Keyboard::Down:
                    pacman_shape.move(0, CELL_SIZE);
                    break;
                case Keyboard::Left:
                    pacman_shape.move(-CELL_SIZE, 0);
                    break;
                case Keyboard::Right:
                    pacman_shape.move(CELL_SIZE, 0);
                    break;
                default:
                    break;
            }
        }

        // Clear, draw, and display
        window.clear();
        drawGrid(window);
        window.draw(pacman_shape); // Draw the player (yellow circle)
        window.display();
    }

    // Join thread
    pthread_join(userInputThread, nullptr);
    // Destroy the event semaphore
    sem_destroy(&eventSemaphore);

    return 0;
}
