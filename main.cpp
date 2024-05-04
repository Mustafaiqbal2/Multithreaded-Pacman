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
#define ROWS 25
#define COLS 25
#define CELL_SIZE 32 // Size of each cell in pixels

using namespace std;
using namespace sf;

// Define game entities
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
    sf::CircleShape pelletShape(5);
    sf::CircleShape powerUpShape(5);
    sf::RectangleShape wallShape(sf::Vector2f(CELL_SIZE -1, CELL_SIZE -1));
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
            } 
            /*
            else if((i >= 0 || j == 0) || (i == ROWS - 1 || j == COLS - 1)) {
                // Fixed walls
                gameMap[i][j] = 1;
            }
            */
            else 
            {
                // Randomly place pellets and power-ups
                int randNum = rand() % 5; // Generate random number from 0 to 9
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
        if (event->type == sf::Event::KeyPressed) 
        {
            switch(event->key.code)
            {
                case sf::Keyboard::Up:
                    pacman_shape->move(0, CELL_SIZE * -1);
                    break;
                case sf::Keyboard::Down:
                    pacman_shape->move(0, CELL_SIZE);
                    break;
                case sf::Keyboard::Left:
                    pacman_shape->move(CELL_SIZE*-1, 0);
                    break;
                case sf::Keyboard::Right:
                    pacman_shape->move(CELL_SIZE, 0);
                    break;
            }
        }
        event->key.code = sf::Keyboard::Unknown;
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
    //cout grif
    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLS; j++)
        {
            cout << gameMap[i][j] << " ";
        }
        cout << endl;
    }
    // Create SFML window
    sf::CircleShape pacman_shape(25/2);
    pacman_shape.setFillColor(sf::Color::Yellow);
    pacman_shape.setPosition(CELL_SIZE + 25/8, CELL_SIZE + 25/4);

    // Create thread for user input
    pthread_t userInputThread;
    Event event;
    void* args[2];
    args[0] = &pacman_shape;
    args[1] = &event;
    pthread_create(&userInputThread, nullptr, userInput, args);

    // Create SFML window
    sf::RenderWindow window(sf::VideoMode(800, 800), "SFML window");

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
        window.clear();
        drawGrid(window);
        window.draw(pacman_shape);
        window.display();
    }

    // Join thread
    pthread_join(userInputThread, nullptr);
    return 0;
}
