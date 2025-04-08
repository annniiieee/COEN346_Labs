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

void Store(int pid, string id, unsigned int value)
{
  lock_guard<mutex> lock(memMutex);
  evictIfNeeded();
  Page page = {id, value, globalClock.load()};
  mainMemory.push_back(page);
  log("Process " + to_string(pid) + ", Store: Variable " + id + ", Value: " + to_string(value));
}

void Release(int pid, string id)
{
  lock_guard<mutex> lock(memMutex);
  mainMemory.remove_if([&](const Page &p)
                       { return p.id == id; });
  disk.erase(id);
  log("Process " + to_string(pid) + ", Release: Variable " + id);
}

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
    evictIfNeeded();
    Page page = disk[id];
    page.lastAccessTime = globalClock.load();
    mainMemory.push_back(page);
    log("Memory Manager, SWAP: Variable " + id + " with memory");
    log("Process " + to_string(pid) + ", Lookup: Variable " + id + ", Value: " + to_string(page.value));
    return;
  }
  log("Process " + to_string(pid) + ", Lookup: Variable " + id + " not found");
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

    // Small random delay between operations
    this_thread::sleep_for(chrono::milliseconds(50 + (rand() % 150)));
  }

  // Wait for process completion (duration is in seconds)
  this_thread::sleep_for(chrono::milliseconds(duration * 1000));
  log("Process " + to_string(pid) + ": Finished.");
}

int main()
{
  // Seed random number generator
  srand(time(nullptr));

  // Read memory configuration
  ifstream memFile("memconfig.txt");
  memFile >> mainMemorySize;
  memFile.close();

  // Read process information
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

  // Read commands and distribute to processes
  // First, sort processes by start time
  vector<int> processIndices(processCount);
  for (int i = 0; i < processCount; i++)
  {
    processIndices[i] = i;
  }
  sort(processIndices.begin(), processIndices.end(),
       [&procTimes](int a, int b)
       { return procTimes[a].first < procTimes[b].first; });

  // Read commands from file
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

  // Distribute commands to processes in order of their start times
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