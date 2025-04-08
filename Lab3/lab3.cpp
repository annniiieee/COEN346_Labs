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
atomic<int> globalClock(0);
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
    globalClock += 10;
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
  while (globalClock < start * 100)
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
    this_thread::sleep_for(chrono::milliseconds(100));
  }
  this_thread::sleep_for(chrono::milliseconds(duration * 100));
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

  ifstream cmdFile("commands.txt");
  string line;
  int idx = 0;
  while (getline(cmdFile, line))
  {
    procCommands[idx % processCount].push_back(line);
    idx++;
  }
  cmdFile.close();

  thread clk(clockThread);

  for (int i = 0; i < processCount; ++i)
  {
    processThreads.emplace_back(runProcess, i + 1, procTimes[i].first, procTimes[i].second, ref(procCommands[i]));
  }

  for (auto &t : processThreads)
  {
    t.join();
  }

  stopClock = true;
  clk.join();
  output.close();

  ofstream diskFile("vm.txt");
  for (auto &[k, v] : disk)
  {
    diskFile << k << " " << v.value << endl;
  }
  diskFile.close();

  return 0;
}