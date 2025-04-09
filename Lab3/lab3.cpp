#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <unordered_map>
#include <list>
#include <vector>
#include <chrono>
#include <atomic>
#include <algorithm>
#include <random>

using namespace std;

// Structure representing a memory page
// Includes the variable ID, its value, and the last time it was accessed
struct Page
{
  string id;
  unsigned int value;
  int lastAccessTime;
};

// Global memory and synchronization variables
int mainMemorySize;
list<Page> mainMemory;
unordered_map<string, Page> disk;
mutex memMutex, logMutex;
atomic<int> globalClock(1000);
atomic<bool> stopClock(false);
ofstream output("output.txt");

// Thread-safe logging function
// Logs events with the current clock value
void log(const string &msg)
{
  lock_guard<mutex> lock(logMutex);
  output << "Clock: " << globalClock.load() << ", " << msg << endl;
  output.flush();
}

// Clock thread function
// Increments the global clock every 100 ms by 10 units
void clockThread()
{
  while (!stopClock)
  {
    this_thread::sleep_for(chrono::milliseconds(100));
    globalClock += 10;
  }
}

// Eviction function using LRU policy
// If memory is full, evicts the least recently used page
// Returns the ID of the evicted page
string evictIfNeeded(const string &incomingVar = "")
{
  if (mainMemory.size() >= mainMemorySize)
  {
    auto lru = min_element(mainMemory.begin(), mainMemory.end(), [](const Page &a, const Page &b)
                           { return a.lastAccessTime < b.lastAccessTime; });
    string swappedOut = lru->id;
    disk[swappedOut] = *lru;
    mainMemory.erase(lru);
    return swappedOut;
  }
  return "";
}

// Store operation
// Adds a variable to memory, evicting if necessary
void Store(int pid, string id, unsigned int value)
{
  lock_guard<mutex> lock(memMutex);

  string swappedOut = evictIfNeeded(id);

  Page page = {id, value, globalClock.load()};
  mainMemory.push_back(page);

  log("Process " + to_string(pid) + ", Store: Variable " + id + ", Value: " + to_string(value));

  if (!swappedOut.empty())
    log("Memory Manager, SWAP: Variable " + id + " with Variable " + swappedOut);
}

// Release operation
// Removes a variable from both memory and disk
void Release(int pid, string id)
{
  lock_guard<mutex> lock(memMutex);
  mainMemory.remove_if([&](const Page &p)
                       { return p.id == id; });
  disk.erase(id);
  log("Process " + to_string(pid) + ", Release: Variable " + id);
}

// Lookup operation
// Searches for a variable in memory or disk
// If found on disk, it is brought back into memory
void Lookup(int pid, string id)
{
  lock_guard<mutex> lock(memMutex);
  for (auto &p : mainMemory)
  {
    if (p.id == id)
    {
      p.lastAccessTime = globalClock.load();
      log("Process " + to_string(pid) + ", Lookup: Variable " + id + ", Value: " + to_string(p.value));
      return;
    }
  }
  if (disk.count(id))
  {
    string swappedOut = evictIfNeeded(id);
    Page page = disk[id];
    page.lastAccessTime = globalClock.load();
    mainMemory.push_back(page);
    if (!swappedOut.empty())
    {
      log("Memory Manager, SWAP: Variable " + id + " with Variable " + swappedOut);
    }
    log("Process " + to_string(pid) + ", Lookup: Variable " + id + ", Value: " + to_string(page.value));
    return;
  }
  log("Process " + to_string(pid) + ", Lookup: Variable " + id + " not found");
}

// Thread function that simulates the lifecycle of a process
// Executes commands between its start and end time, respecting clock
void runProcess(int pid, int start, int duration, vector<string> &commands)
{
  int startTimeMs = start * 1000;
  int endTimeMs = startTimeMs + duration * 1000;

  // Wait until the global clock reaches the process's start time
  while (globalClock < startTimeMs)
  {
    this_thread::sleep_for(chrono::milliseconds(10));
  }

  log("Process " + to_string(pid) + ": Started.");

  for (const auto &cmd : commands)
  {
    istringstream ss(cmd);
    string op, var;
    unsigned int val;
    ss >> op >> var;

    if (op == "Store")
    {
      ss >> val;
      Store(pid, var, val);
    }
    else if (op == "Release")
    {
      Release(pid, var);
    }
    else if (op == "Lookup")
    {
      Lookup(pid, var);
    }

    // Random delay between commands to simulate processing time
    int delay = 150 + (rand() % 200);
    this_thread::sleep_for(chrono::milliseconds(delay));

    // Stop early if we've reached or exceeded the process end time
    if (globalClock >= endTimeMs)
      break;
  }

  // Ensure the process runs for the full virtual time
  while (globalClock < endTimeMs)
  {
    this_thread::sleep_for(chrono::milliseconds(10));
  }

  log("Process " + to_string(pid) + ": Finished.");
}

// Main function
// Initializes memory, reads configs, assigns commands to processes, and launches threads
int main()
{
  srand(time(nullptr)); // Initialize random seed

  // Read main memory size from memconfig.txt
  ifstream memFile("memconfig.txt");
  memFile >> mainMemorySize;
  memFile.close();

  // Read process configuration from processes.txt
  ifstream procFile("processes.txt");
  int cores, processCount;
  procFile >> cores >> processCount;
  vector<thread> processThreads;
  vector<vector<string>> procCommands(processCount);
  vector<pair<int, int>> procTimes;

  for (int i = 0; i < processCount; i++)
  {
    int start, duration;
    procFile >> start >> duration;
    procTimes.push_back({start, duration});
  }
  procFile.close();

  // Sort processes by start time to assign commands fairly
  vector<int> processIndices(processCount);
  for (int i = 0; i < processCount; i++)
  {
    processIndices[i] = i;
  }
  sort(processIndices.begin(), processIndices.end(), [&procTimes](int a, int b)
       { return procTimes[a].first < procTimes[b].first; });

  // Read all commands from commands.txt
  vector<string> allCommands;
  ifstream cmdFile("commands.txt");
  string line;
  while (getline(cmdFile, line))
  {
    if (!line.empty())
    {
      allCommands.push_back(line);
    }
  }
  cmdFile.close();

  // Distribute commands to processes as evenly as possible
  int commandsPerProcess = allCommands.size() / processCount;
  int remainingCommands = allCommands.size() % processCount;

  int cmdIndex = 0;
  for (int i = 0; i < processCount; i++)
  {
    int procIdx = processIndices[i];
    int cmdsForThisProcess = commandsPerProcess + (i < remainingCommands ? 1 : 0);

    for (int j = 0; j < cmdsForThisProcess && cmdIndex < allCommands.size(); j++)
    {
      procCommands[procIdx].push_back(allCommands[cmdIndex++]);
    }
  }

  // Start the global clock thread
  thread clk(clockThread);

  // Launch process threads
  for (int i = 0; i < processCount; ++i)
  {
    processThreads.emplace_back(runProcess, i + 1, procTimes[i].first, procTimes[i].second, ref(procCommands[i]));
  }

  // Wait for all process threads to complete
  for (auto &t : processThreads)
  {
    t.join();
  }

  // Stop the clock thread
  stopClock = true;
  clk.join();
  output.close();

  // Write contents of virtual memory (disk) to vm.txt
  ofstream diskFile("vm.txt");
  for (auto &[k, v] : disk)
  {
    diskFile << k << " " << v.value << endl;
  }
  diskFile.close();

  return 0;
}
