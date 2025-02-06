#include <iostream>
#include <thread>
#include <chrono>

void printHi() {
    for (int i = 0; i < 5; i++) {
        std::cout << "Hi" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void printHello() {
    for (int i = 0; i < 5; i++) {
        std::cout << "Hello" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main() {
    std::thread t1(printHi);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    std::thread t2(printHello);

    t1.join();
    t2.join();

    return 0;
}