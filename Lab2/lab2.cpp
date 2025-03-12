#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <algorithm>
#include <stdexcept>
#include <sstream>


// Process struct to store process information
struct Process {
    int id, readyTime, serviceTime, remainingTime;
    bool started, finished;
    Process(int id, int readyTime, int serviceTime) :
        id(id), readyTime(readyTime), serviceTime(serviceTime),
        remainingTime(serviceTime), started(false), finished(false) {}
};

// User struct to store user information and their processes
struct User {
    std::string name;
    std::vector<Process> processes;
    User(const std::string& name) : name(name) {}
};

// Scheduler class to simulate process scheduling
class Scheduler {
private:
    int timeQuantum, currentTime;
    std::vector<User> users;
    std::ofstream outputFile;
    std::mutex fileMutex, mtx;

public:
    // Constructor to read input file and open output file
    Scheduler(const std::string& inputFile, const std::string& outputFile) : currentTime(1) {
        this->outputFile.open(outputFile);
        readInputFile(inputFile);
    }

    // Destructor to close output file
    ~Scheduler() {
        if (outputFile.is_open()) {
            outputFile.close();
        }
    }

    // Read input file and store user and process information
    void readInputFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Error opening input file: " + filename);
        }
        
        // Check if file is empty
        if (file.peek() == std::ifstream::traits_type::eof()) {
            throw std::runtime_error("Input file is empty: " + filename);
        }

        // Read time quantum
        std::string line;
        bool timeQuantumRead = false;

        // validating the time quantum
        while (std::getline(file, line)) {
            if (line.empty()) continue;  // Skip empty lines

            std::istringstream iss(line);
            if (!(iss >> timeQuantum)) {
                throw std::runtime_error("Invalid time quantum format in input file.");
            }

            if (timeQuantum <= 0) {
                throw std::runtime_error("Time quantum must be a positive integer.");
            }

            timeQuantumRead = true;
            break;
        }

        if (!timeQuantumRead) {
            throw std::runtime_error("Missing time quantum in input file.");
        }

        // Read user and process information
        std::string userName;
        int numProcesses;
        
        // reads and validates the user name and process count
        while (file >> userName >> numProcesses) {
            if (userName.empty()) {
                throw std::runtime_error("Empty user name detected.");
            }

            if (numProcesses < 0) {
                throw std::runtime_error("Invalid process count for user " + userName + ": cannot be negative.");
            }

            // Create user object and read process information to then store in the user object
            User user(userName);

            for (int i = 0; i < numProcesses; i++) {
                int readyTime, serviceTime;
                bool processInfoRead = false;

                while (std::getline(file, line)) {
                    if (line.empty()) continue; // Skip empty lines

                    std::istringstream iss(line);
                    if (!(iss >> readyTime >> serviceTime)) {
                        throw std::runtime_error("Invalid process details format for user " + userName + ", process " + std::to_string(i));
                    }

                    // Validate process times
                    if (readyTime < 0) {
                        throw std::runtime_error("Ready time must be non-negative for user " + userName + ", process " + std::to_string(i));
                    }

                    if (serviceTime <= 0) {
                        throw std::runtime_error("Service time must be positive for user " + userName + ", process " + std::to_string(i));
                    }

                    processInfoRead = true;
                    break;
                }

                if (!processInfoRead) {
                    throw std::runtime_error("Missing process details for user " + userName + ", process " + std::to_string(i));
                }

                // Add process to the user object
                // emplace_back is used to construct the Process object in-place
                user.processes.emplace_back(i, readyTime, serviceTime);
            }

            // add the user to the list of users
            users.push_back(user);
        }

        // Check if we read any users
        if (users.empty()) {
            throw std::runtime_error("No valid users found in input file.");
        }

        file.close();
    }

    // Write output message to file
    // either a simple message (no process ready at this time) or a process status update
    void writeToFile(int time, const std::string& messageOrUser, int processId = -1, const std::string& status = "") {
        std::lock_guard<std::mutex> lock(fileMutex);
        if (outputFile.is_open()) {
            outputFile << "Time " << time;
            
            if (processId >= 0 && !status.empty()) {
                // Process status update format
                outputFile << ", User " << messageOrUser << ", Process " << processId << ", " << status;
            } else {
                // Simple message format
                outputFile << ", " << messageOrUser;
            }
            
            outputFile << std::endl;
        } else {
            std::cerr << "Warning: Output file is closed!" << std::endl;
        }
    }

    // check if any process is ready at the current time
    bool isAnyProcessReady(int time) {
        for (const auto& user : users) {
            for (const auto& process : user.processes) {
                if (!process.finished && process.readyTime <= time) {
                    return true;
                }
            }
        }
        return false;
    }

    // check if all processes have finished
    bool areAllProcessesFinished() {
        for (const auto& user : users) {
            for (const auto& process : user.processes) {
                if (!process.finished) {
                    return false;
                }
            }
        }
        return true;
    }

    // get the earliest ready time of all processes
    int getEarliestReadyTime() {
        int earliestTime = -1;  // Initialize with -1 to indicate no value found yet
        
        for (const auto& user : users) {
            for (const auto& process : user.processes) {
                if (!process.finished && process.readyTime > currentTime) {
                    // If this is the first valid process we've found, or it has an earlier ready time
                    if (earliestTime == -1 || process.readyTime < earliestTime) {
                        earliestTime = process.readyTime;
                    }
                }
            }
        }
        
        return earliestTime;  // Will be -1 if no valid process was found
    }


    // Simulating process execution
    void processExecution(User& user, Process& process, int timeSlice) {
        bool ready = false;
        while (!ready) {
            {
                std::lock_guard<std::mutex> lock(mtx);
                ready = (process.readyTime <= currentTime);
            }
        }
        
        // Now enter critical section using locks --> lock_guard
        {
            std::lock_guard<std::mutex> lock(mtx);
            
            if (!process.started) {
                process.started = true;
                writeToFile(currentTime, user.name, process.id, "Started");
            } else {
                writeToFile(currentTime, user.name, process.id, "Resumed");
            }

            int executionTime = std::min(timeSlice, process.remainingTime);
            process.remainingTime -= executionTime;
            currentTime += executionTime;

            if (process.remainingTime == 0) {
                process.finished = true;
                writeToFile(currentTime, user.name, process.id, "Finished");
            } else {
                writeToFile(currentTime, user.name, process.id, "Paused");
            }
        }
        
        // release the mutex
    }

    // Simulate process scheduling
    // first checks if any process is ready at the current time
    // if not, advances time to the next ready process
    // then iterates over all users and their processes to simulate process execution
    // each process is executed in a separate thread
    // the time slice for each process is calculated based on the number of active users
    void simulateScheduling() {
        // Initial idle time handling
        if (!isAnyProcessReady(currentTime)) {
            int nextReadyTime = getEarliestReadyTime();
            if (nextReadyTime > 0) {
                while (currentTime < nextReadyTime) {
                    writeToFile(currentTime, "No process is ready at this time yet");
                    currentTime++;
                }
            }
        }

        while (!areAllProcessesFinished()) {
            std::vector<std::thread> threads;
            std::map<int, int> activeUsers;

            // Count active processes for each user
            for (size_t userIdx = 0; userIdx < users.size(); userIdx++) {
                for (auto& process : users[userIdx].processes) {
                    if (!process.finished && process.readyTime <= currentTime) {
                        activeUsers[userIdx]++;
                    }
                }
            }

            // If no active processes at current time, advance time to next ready process
            if (activeUsers.empty()) {
                int nextReadyTime = getEarliestReadyTime();
                if (nextReadyTime > 0) {
                    while (currentTime < nextReadyTime) {
                        writeToFile(currentTime, "No process is ready at this time yet");
                        currentTime++;
                    }
                    continue;  // Restart the loop with the new current time
                } else {
                    // No more processes will become ready, we're done
                    break;
                }
            }

            int numActiveUsers = activeUsers.size();
            int timePerUser = timeQuantum / std::max(1, numActiveUsers);

            for (const auto& [userIdx, processCount] : activeUsers) {
                int timePerProcess = timePerUser / std::max(1, processCount);
                for (auto& process : users[userIdx].processes) {
                    if (!process.finished && process.readyTime <= currentTime) {
                        threads.emplace_back(&Scheduler::processExecution, this, std::ref(users[userIdx]), std::ref(process), timePerProcess);
                    }
                }
            }

            for (auto& th : threads) {
                if (th.joinable()) {
                    th.join();
                }
            }
        }
    }
};

int main() {
    try {
        Scheduler scheduler("input.txt", "output.txt");
        scheduler.simulateScheduling();
        std::cout << "Scheduling simulation completed successfully." << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}