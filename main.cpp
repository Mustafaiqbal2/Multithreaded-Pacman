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

void drawGrid(sf::RenderWindow& window)
{
    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLS; j++)
        {
            sf::RectangleShape cell(sf::Vector2f(CELL_SIZE, CELL_SIZE));
            cell.setOutlineColor(sf::Color::White);
            cell.setOutlineThickness(1);
            cell.setPosition(j * CELL_SIZE, i * CELL_SIZE);
            window.draw(cell);
        }
    }
}

// Define a struct to hold user input events
struct InputEvent {
    sf::Keyboard::Key key;
    bool pressed;
};

// Queue for input events
queue<InputEvent> inputQueue;

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
        }
        pthread_mutex_unlock(&inputMutex);
        sleep(sf::milliseconds(100));
    }

}

int main() {
    // Initialize random seed
    srand(time(nullptr));

    // Create SFML window
    sf::CircleShape pacman_shape(50);
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
    sf::RenderWindow window(sf::VideoMode(800, 600), "SFML window");

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
        window.draw(pacman_shape);
        window.display();
    }

    // Join thread
    pthread_join(userInputThread, nullptr);

    return 0;
}
