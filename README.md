# Multithreaded-Pacman

A modern C++ implementation of the classic Pac-Man game utilizing multithreading for enhanced performance and gameplay mechanics.


## Overview

This project reimagines the classic Pac-Man arcade game with a multithreaded architecture. By leveraging modern C++ concurrency features, the game achieves smoother animations, more complex ghost AI behavior, and better performance even with advanced game mechanics.

## Features

- **Multithreaded Architecture**: Separate threads for game logic, rendering, audio, and AI
- **Classic Gameplay**: Faithful recreation of the original Pac-Man mechanics
- **Enhanced Ghost AI**: More sophisticated ghost behavior powered by concurrent processing
- **Cross-Platform**: Runs on Windows, Linux, and macOS
- **Optimized Performance**: Leverages multiple CPU cores for smoother gameplay
- **Thread-Safe Game State**: Robust synchronization mechanisms for shared game state
- **Modern C++**: Utilizes C++17 features and best practices

## Requirements

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.12 or higher
- SFML 2.5+ (for graphics and audio)
- Boost 1.65+ (for additional threading utilities)
- A CPU with at least 4 cores recommended for optimal performance

## Building from Source

### Linux/macOS

```bash
# Install dependencies (Ubuntu example)
sudo apt-get install libsfml-dev libboost-all-dev cmake g++

# Clone the repository
git clone https://github.com/Mustafaiqbal2/Multithreaded-Pacman.git
cd Multithreaded-Pacman

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
make -j4

# Run the game
./pacman
```

### Windows

```powershell
# Clone the repository
git clone https://github.com/Mustafaiqbal2/Multithreaded-Pacman.git
cd Multithreaded-Pacman

# Create build directory
mkdir build
cd build

# Configure and build (with Visual Studio)
cmake ..
cmake --build . --config Release

# Run the game
.\Release\pacman.exe
```

## Game Controls

- **Arrow Keys**: Move Pac-Man
- **P**: Pause/Resume game
- **ESC**: Exit game
- **R**: Restart current level
- **M**: Mute/Unmute sound
- **+/-**: Increase/decrease volume

## Multithreading Architecture

The game utilizes multiple threads to separate concerns and improve performance:

1. **Main Thread**: Handles user input and coordinates other threads
2. **Rendering Thread**: Dedicated to drawing the game world at consistent frame rates
3. **Game Logic Thread**: Updates game state, collisions, and scoring
4. **Ghost AI Threads**: Each ghost runs on its own thread for pathfinding and decision making
5. **Audio Thread**: Manages sound effects and background music without interrupting gameplay
6. **Physics Thread**: Handles collision detection and response

Thread synchronization is achieved using a combination of mutexes, condition variables, and atomic operations to ensure thread-safe access to shared game state.

## Implementation Details

### Thread-Safe Game State

The game state is protected using a read-write lock pattern allowing multiple concurrent reads but exclusive write access:

```cpp
class GameState {
private:
    std::shared_mutex stateMutex;
    // Game state variables...

public:
    // Thread-safe state access methods
    void updatePacmanPosition(Position newPos) {
        std::unique_lock lock(stateMutex);
        // Update position...
    }
    
    Position getPacmanPosition() const {
        std::shared_lock lock(stateMutex);
        return pacmanPosition;
    }
};
```

### Ghost AI Concurrency

Each ghost operates independently with its own behavior thread:

```cpp
class Ghost {
private:
    std::thread aiThread;
    std::atomic<bool> running{true};
    
    void aiThreadFunction() {
        while (running) {
            // Ghost AI logic
            calculateNextMove();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

public:
    Ghost() {
        aiThread = std::thread(&Ghost::aiThreadFunction, this);
    }
    
    ~Ghost() {
        running = false;
        if (aiThread.joinable()) {
            aiThread.join();
        }
    }
};
```

## Performance Optimizations

- **Thread Pool**: Reuses threads for short-lived tasks to reduce overhead
- **Lock-Free Data Structures**: Minimizes contention in high-traffic shared data
- **Data Locality**: Optimized memory layout for cache-friendly access patterns
- **Work Stealing**: Balances workload across threads dynamically

## Customization

The game supports several configuration options:

- Custom maze layouts via simple text files
- Adjustable difficulty levels
- Configurable ghost behaviors
- Custom skins and themes

## Future Improvements

- [ ] Network multiplayer support
- [ ] Additional power-ups and game mechanics
- [ ] Level editor
- [ ] Advanced AI using machine learning
- [ ] VR support

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Credits

- Original Pac-Man game by Namco
- SFML team for the graphics library
- All contributors to this project

---

Developed by [Mustafa Iqbal](https://github.com/Mustafaiqbal2)

Last updated: 2025-03-14
