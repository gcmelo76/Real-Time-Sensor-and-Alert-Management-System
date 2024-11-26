# Real-Time Sensor and Alert Management System

A project designed to manage real-time sensor data and user commands using inter-process communication, threading, and semaphores. This system enables efficient processing and management of sensor data, user commands, and alerts in a concurrent environment.

---

## Features

- **Real-Time Sensor Data Processing**:
  - Handles sensor statistics and updates using dedicated functions.
  - Processes sensor messages to update or add new sensor data.
- **User Command Management**:
  - Executes user commands for managing sensors and alerts.
  - Supports commands for resetting statistics, listing sensors, and managing alerts.
- **Inter-Process Communication**:
  - Utilizes named pipes for communication between components.
  - Manages message queues with priority handling.
- **Concurrency Management**:
  - Implements threading and semaphores to ensure synchronization and avoid race conditions.
- **Alert System**:
  - Adds, lists, and removes alerts dynamically for sensors.

---

## Technologies Used

- **Programming Language**: C
- **Concurrency Tools**: Threads, Semaphores
- **Communication Mechanism**: Named Pipes
- **Data Structures**: Internal Queues for message prioritization
- **Functionality**: Signal handling, string parsing with `strtok`, resource cleanup

---

## Project Structure

### Core Functions

1. **Inter-Process Communication**:
   - `createThreads()`: Sets up threads for reading data from pipes.
   - `spawnProcesses()`: Creates child processes for message processing and alert control.
2. **Data Processing**:
   - `processSensor()`: Processes sensor data and updates statistics.
   - `processUser()`: Handles user commands and executes appropriate actions.
3. **Queue Management**:
   - `addToQueue()`: Adds messages to a prioritized internal queue.
   - `popFromQueue()`: Removes messages from the queue for processing.
4. **Thread Functions**:
   - `console()`: Reads user commands from the console and adds them to the queue.
   - `sensor()`: Reads sensor data from the pipe and adds it to the queue.
   - `dispatch()`: Processes messages from the queue and sends them for handling.
5. **System Cleanup**:
   - `cleanUp()`: Releases all allocated resources and clears temporary storage.

---

## Getting Started

### Prerequisites

- **Operating System**: Linux (recommended for named pipes and semaphore support).
- **Compiler**: GCC or any C compiler supporting POSIX.
- **Dependencies**:
  - Standard C libraries for threading and semaphore functionality.

### Installation Steps

1. **Clone the Repository**
   ```bash
   git clone https://github.com/gcmelo76/Real-Time-Sensor-and-Alert-Management-System.git
   cd real-time-sensor-and-alert-management-system

2. **Compile the Program**
   ```bash
   gcc -o system_manager main.c -lpthread

3. **Run the Application**
   ```bash
   ./system_manager
