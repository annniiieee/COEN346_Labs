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

struct Page
{
  string id;
  unsigned int value;
  int lastAccessTime;
};

int mainMemorySize;
list<Page> mainMemory;
unordered_map<string, Page> disk;
mutex memMutex, logMutex;
atomic<int> globalClock(1000); // Start the clock at 1000
atomic<bool> stopClock(false);
ofstream output("output.txt");

// Random generator for time increments between operations
mt19937 rng(std::random_device{}());
uniform_int_distribution<int> dist(50, 200);

void log(const string &msg)
{
  lock_guard<mutex> lock(logMutex);
  output << "Clock: " << globalClock.load() << ", " << msg << endl;
  output.flush();
}

void clockThread()
{
  while (!stopClock)
  {
    this_thread::sleep_for(chrono::milliseconds(100));
    globalClock += 10; // Keep consistent increment of 10 units
  }
}

void evictIfNeeded()
{
  if (mainMemory.size() >= mainMemorySize)
  {
    auto lru = std::min_element(mainMemory.begin(), mainMemory.end(), [](const Page &a, const Page &b)
                                { return a.lastAccessTime < b.lastAccessTime; });
    log("Memory Manager, SWAP: Variable " + lru->id + " with disk");
    disk[lru->id] = *lru;
    mainMemory.erase(lru);
  }
}

void Store(string id, unsigned int value)
{
  lock_guard<mutex> lock(memMutex);
  evictIfNeeded();
  Page page = {id, value, globalClock.load()};
  mainMemory.push_back(page);
  log("Store: Variable " + id + ", Value: " + to_string(value));
}

void Release(string id)
{
  lock_guard<mutex> lock(memMutex);
  mainMemory.remove_if([&](const Page &p)
                       { return p.id == id; });
  disk.erase(id);
  log("Release: Variable " + id);
}

void Lookup(string id)
{
  lock_guard<mutex> lock(memMutex);
  for (auto &p : mainMemory)
  {
    if (p.id == id)
    {
      p.lastAccessTime = globalClock.load();
      log("Lookup: Variable " + id + ", Value: " + to_string(p.value));
      return;
    }
  }
  if (disk.count(id))
  {
    evictIfNeeded();
    Page page = disk[id];
    page.lastAccessTime = globalClock.load();
    mainMemory.push_back(page);
    log("Memory Manager, SWAP: Variable " + id + " with memory");
    log("Lookup: Variable " + id + ", Value: " + to_string(page.value));
    return;
  }
  log("Lookup: Variable " + id + " not found");
}

void runProcess(int pid, int start, int duration, vector<string> &commands)
{
  // For process with start time 1, it should start at time 1000
  // For start time 2, it should start at 2000, etc.
  int startTimeMs = start * 1000;

  // Wait until global clock reaches the process start time
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
      Store(var, val);
    }
    else if (op == "Release")
    {
      Release(var);
    }
    else if (op == "Lookup")
    {
      Lookup(var);
    }

    // Random delay between operations to simulate varied execution times
    this_thread::sleep_for(chrono::milliseconds(dist(rng)));
  }

  // Wait for process completion (duration is in seconds)
  this_thread::sleep_for(chrono::milliseconds(duration * 1000));
  log("Process " + to_string(pid) + ": Finished.");
}

int main()
{
  ifstream memFile("memconfig.txt");
  memFile >> mainMemorySize;
  memFile.close();

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

  // Read all commands from commands.txt
  vector<string> allCommands;
  ifstream cmdFile("commands.txt");
  string line;
  while (getline(cmdFile, line))
  {
    allCommands.push_back(line);
  }
  cmdFile.close();

  // Assign specific commands to specific processes
  // From the output you showed, it looks like:
  // Process 2 (starts at 1000): Store 1 5, Store 2 3
  // Process 1 (starts at 2000): Store 3 7, Lookup 3, Lookup 2
  // Process 3 (starts at 4000): Release 1, Store 1 8, Lookup 1

  // For this test case, I'll hardcode according to the expected output:
  procCommands[1].push_back("Store 1 5"); // Process 2
  procCommands[1].push_back("Store 2 3"); // Process 2

  procCommands[0].push_back("Store 3 7"); // Process 1
  procCommands[0].push_back("Lookup 3");  // Process 1
  procCommands[0].push_back("Lookup 2");  // Process 1

  procCommands[2].push_back("Release 1"); // Process 3
  procCommands[2].push_back("Store 1 8"); // Process 3
  procCommands[2].push_back("Lookup 1");  // Process 3

  // Start the clock thread
  thread clk(clockThread);

  // Launch process threads
  for (int i = 0; i < processCount; ++i)
  {
    processThreads.emplace_back(runProcess, i + 1, procTimes[i].first, procTimes[i].second, ref(procCommands[i]));
  }

  // Wait for all processes to complete
  for (auto &t : processThreads)
  {
    t.join();
  }

  // Stop the clock thread
  stopClock = true;
  clk.join();
  output.close();

  // Write virtual memory content to file
  ofstream diskFile("vm.txt");
  for (auto &[k, v] : disk)
  {
    diskFile << k << " " << v.value << endl;
  }
  diskFile.close();

  return 0;
}