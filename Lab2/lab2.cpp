#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

using namespace std;

class Process {
public:
    string user;
    int id;
    int readyTime;
    int serviceTime;
    int remainingTime;
    bool running = false;

    // Constructor
    Process (string user, int id, int readyTime, int serviceTime) {
        this->user = user;
        this->id = id;
        this->readyTime = readyTime;
        this->serviceTime = serviceTime;
        this->remainingTime = serviceTime;
    }
};

class User {
public:
    string name;
    vector<Process*> processes;
};


void runProcess(Process* process, ofstream& output, int& currentTime) {

}

void scheduler(int timeQuantum, vector<User>& users, ofstream& output) {

}

int main() {
    ifstream input("input.txt");
    ofstream output("output.txt");

    if (!input) {
        cerr << "Error opening input file!" << endl;
        return 1;
    }

    int timeQuantum;
    input >> timeQuantum;
    output << "Time Quantum: " << timeQuantum << endl;
    vector<User> users;

    string username;
    while (input >> username) {
        int numProcesses;
        input >> numProcesses;
        output << "User " << username << " Number of Processes: " << numProcesses << endl;
        User user;
        user.name = username;

        for (int i = 0; i < numProcesses; ++i) {
            int readyTime, serviceTime;
            input >> readyTime >> serviceTime;
            user.processes.push_back(new Process(username, i, readyTime, serviceTime));
            output << "User " << username << " Process " << i << " Ready Time: " << readyTime << ", Service Time: " << serviceTime << endl;
        }

        users.push_back(user);
    }

    thread schedulerThread(scheduler, timeQuantum, ref(users), ref(output));
    schedulerThread.join();

    for (auto& user : users) {
        for (auto& process : user.processes) {
            delete process;
        }
    }

    return 0;
}