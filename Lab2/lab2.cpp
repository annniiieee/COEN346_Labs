#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <queue>
#include <string>
#include <algorithm>
#include <mutex>
#include <limits>
#include <stdexcept>


struct Process {
    int id;             // Unique identifier for the process
    int readyTime;      // Time when the process becomes ready
    int burstTime;    // Total CPU time required
    int remainingTime;  // Remaining CPU time needed
    bool started;       // Whether process has started execution
    bool finished;      // Whether process has completed execution

    Process(int id, int readyTime, int burstTime) :
        id(id), readyTime(readyTime), burstTime(burstTime),
        remainingTime(burstTime), started(false), finished(false) {}
};
struct User {
    std::string name;               // Username
    std::vector<Process> processes; // Processes owned by this user

    User(const std::string& name) : name(name) {}
};

class Scheduler {
private:
    int timeQuantum;                // Time quantum for round-robin scheduling
    std::vector<User> users;        // List of users in the system
    std::ofstream outputFile;       // Output file stream
    int currentTime;                // Current simulation time
    std::mutex fileMutex;           // Mutex for thread-safe file operations

public:
    // Constructor for Scheduler
    Scheduler(const std::string& inputFile, const std::string& outputFile) : currentTime(1) {
        // Open output file
        this->outputFile.open(outputFile);
        if (!this->outputFile.is_open()) {
            throw std::runtime_error("Error opening output file: " + outputFile);
        }
        
        // Read input file configuration
        try {
            readInputFile(inputFile);
        } catch (const std::exception& e) {
            // Close output file if input file reading fails
            if (this->outputFile.is_open()) {
                this->outputFile.close();
            }
            throw; // Re-throw the exception
        }
    }

   
     // Destructor for Scheduler
     // Ensures output file is properly closed
    ~Scheduler() {
        if (outputFile.is_open()) {
            outputFile.close();
        }
    }

    /**
     * @brief Reads scheduling configuration from input file
     * @param filename Path to the input file
     * @throws std::runtime_error If input file cannot be opened or has invalid format
     */
    void readInputFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Error opening input file: " + filename);
        }

        // Read time quantum
        if (!(file >> timeQuantum)) {
            file.close();
            throw std::runtime_error("Error reading time quantum from input file");
        }

        // Validate time quantum
        if (timeQuantum <= 0) {
            file.close();
            throw std::runtime_error("Invalid time quantum: must be positive");
        }

        std::string userName;
        int numProcesses;

        // Read user and process information
        while (file >> userName >> numProcesses) {
            // Validate number of processes
            if (numProcesses <= 0) {
                file.close();
                throw std::runtime_error("Invalid number of processes for user " + userName);
            }

            User user(userName);

            for (int i = 0; i < numProcesses; i++) {
                int readyTime, burstTime;
                
                // Read process details
                if (!(file >> readyTime >> burstTime)) {
                    file.close();
                    throw std::runtime_error("Error reading process details for user " + userName);
                }
                
                // Validate process parameters
                if (readyTime < 0) {
                    file.close();
                    throw std::runtime_error("Invalid ready time for process " + std::to_string(i) + 
                                            " of user " + userName);
                }
                
                if (burstTime <= 0) {
                    file.close();
                    throw std::runtime_error("Invalid service time for process " + std::to_string(i) + 
                                            " of user " + userName);
                }
                
                user.processes.push_back(Process(i, readyTime, burstTime));
            }

            users.push_back(user);
        }

        // Check if we have at least one user
        if (users.empty()) {
            file.close();
            throw std::runtime_error("No users found in input file");
        }

        file.close();
    }

    /**
     * @brief Writes a message to the output file in a thread-safe manner
     * @param message The message to write
     */
    void writeToFile(const std::string& message) {
        // Lock the mutex to ensure thread safety
        std::lock_guard<std::mutex> lock(fileMutex);
        
        if (outputFile.is_open()) {
            outputFile << message << std::endl;
        } else {
            std::cerr << "Warning: Attempted to write to closed file: " << message << std::endl;
        }
    }

    /**
     * @brief Simulates the scheduling of processes
     * @throws std::runtime_error If an error occurs during simulation
     */
    void simulateScheduling() {
        bool allProcessesFinished = false;

        try {
            // Continue until all processes are finished
            while (!allProcessesFinished) {
                // Get ready users and their processes for the current cycle
                std::vector<std::pair<int, int>> readyProcesses; // pair<user_index, process_index>

                // Identify all processes that are ready to run at the current time
                for (size_t userIndex = 0; userIndex < users.size(); userIndex++) {
                    for (size_t processIndex = 0; processIndex < users[userIndex].processes.size(); processIndex++) {
                        Process& process = users[userIndex].processes[processIndex];

                        // Check if process is ready and not finished
                        // add them in the ready processes list 
                        if (process.readyTime <= currentTime && !process.finished && process.remainingTime > 0) {
                            readyProcesses.push_back(std::make_pair(userIndex, processIndex));
                        }
                    }
                }

                // Handle the case where no processes are ready
                if (readyProcesses.empty()) {
                    // No ready processes, advance time to the next earliest ready time
                    int nextReadyTime = std::numeric_limits<int>::max();
                    for (const auto& user : users) {
                        for (const auto& process : user.processes) {
                            if (!process.finished && process.readyTime > currentTime && process.readyTime < nextReadyTime) {
                                nextReadyTime = process.readyTime;
                            }
                        }
                    }

                    // Check if all processes are finished
                    if (nextReadyTime == std::numeric_limits<int>::max()) {
                        allProcessesFinished = true;
                    } else {
                        currentTime = nextReadyTime;
                    }
                    continue;
                }

                // Count the number of users with ready processes
                std::map<int, bool> activeUsers;
                for (const auto& process : readyProcesses) {
                    activeUsers[process.first] = true; //user has at least 1 ready process
                }
                int numActiveUsers = activeUsers.size();

                // Calculate time slices for each user and process
                int timePerUser = timeQuantum / numActiveUsers;
                
                // Count number of processes per user
                std::map<int, int> processesPerUser;
                //readyProcesses = pair<user_index, process_index>
                for (const auto& process : readyProcesses) {
                    processesPerUser[process.first]++;
                }

                // Execute processes for this cycle
                for (const auto& processInfo : readyProcesses) {
                    int userIndex = processInfo.first;
                    int processIndex = processInfo.second;
                    Process& process = users[userIndex].processes[processIndex];

                    // Calculate time slice for this process
                    int timeSlice = timePerUser / processesPerUser[userIndex];
                    // Ensure minimum time slice of 1
                    timeSlice = std::max(1, timeSlice);

                    // If this is the first time the process is running, mark it as started
                    if (!process.started) {
                        process.started = true;
                        std::string message = "Time " + std::to_string(currentTime) + 
                                             ", User " + users[userIndex].name +
                                             ", Process " + std::to_string(process.id) + 
                                             ", Started";
                        writeToFile(message);
                    }

                    // Process is resumed
                    std::string resumeMessage = "Time " + std::to_string(currentTime) + 
                                              ", User " + users[userIndex].name +
                                              ", Process " + std::to_string(process.id) + 
                                              ", Resumed";
                    writeToFile(resumeMessage);

                    // Calculate actual execution time for this slice
                    int executionTime = std::min(timeSlice, process.remainingTime);
                    process.remainingTime -= executionTime;

                    // Process is paused after execution
                    std::string pauseMessage = "Time " + std::to_string(currentTime + executionTime) + 
                                             ", User " + users[userIndex].name +
                                             ", Process " + std::to_string(process.id) + 
                                             ", Paused";
                    writeToFile(pauseMessage);

                    // If process is finished
                    if (process.remainingTime == 0) {
                        process.finished = true;
                        std::string finishMessage = "Time " + std::to_string(currentTime + executionTime) + 
                                                  ", User " + users[userIndex].name +
                                                  ", Process " + std::to_string(process.id) + 
                                                  ", Finished";
                        writeToFile(finishMessage);
                    }

                    // Update currentTime for the next process
                    currentTime += executionTime;
                }

                // Check if all processes are finished
                allProcessesFinished = true;
                for (const auto& user : users) {
                    for (const auto& process : user.processes) {
                        if (!process.finished) {
                            allProcessesFinished = false;
                            break;
                        }
                    }
                    if (!allProcessesFinished) break;
                }
            }
        } catch (const std::exception& e) {
            throw std::runtime_error("Simulation error: " + std::string(e.what()));
        }
    }
};

/**
 * @brief Main function
 * @return 0 on success, non-zero on failure
 */
int main() {
    try {
        // Create scheduler with input and output file paths
        Scheduler scheduler("input.txt", "output.txt");
        
        // Run the simulation
        scheduler.simulateScheduling();
        
        std::cout << "Scheduling simulation completed successfully." << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}