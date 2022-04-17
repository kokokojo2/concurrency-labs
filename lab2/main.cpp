#include <iostream>
#include <thread>
#include "core.h"
#include <cstdlib>

auto q = Queue();
State state = State();

void CPUProc(int id) {
    auto cpu = CPU(id, q, state, "CPU" + std::to_string(id));
    cpu.run();
}

void GenProc(unsigned int procNum, RandomRanges &randomRanges) {
    ProcessGenerator gen = ProcessGenerator(q, state, "Generator", procNum, randomRanges);
    gen.run();
}

int main() {
    srand(time(nullptr));
    unsigned int numProcesses;
    RandomRanges ranges{};
    std::cout << "Input the num of processes to generate :";
    std::cin >> numProcesses;

    std::cout << "Input the min sleep time :";
    std::cin >> ranges.minSleepTime;

    std::cout << "Input the max sleep time :";
    std::cin >> ranges.maxSleepTime;

    std::cout << "Input the min execution time :";
    std::cin >> ranges.minExecutionTime;

    std::cout << "Input the max execution time :";
    std::cin >> ranges.maxExecutionTime;

    std::thread cpu1(CPUProc, 1);
    std::thread cpu2(CPUProc, 2);

    std::thread gen(GenProc, numProcesses, std::ref(ranges));

    gen.join();
    cpu1.join();
    cpu2.join();

    return 0;
}
