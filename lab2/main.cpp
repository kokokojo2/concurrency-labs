#include <iostream>
#include <thread>
#include "core.h"
#include <cstdlib>

auto q = Queue();
State state = State();

void thr1(Process &process) {
    std::cout << "thr1 " << process.getPid() << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout << "thr1 after sleep " << process.getPid() << std::endl;

}

void thr2(Process &process) {
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << "Swapping the process" << std::endl;
    process = Process(10, 2, "Dsdf");
    std::cout << "Process swapped" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));

}

void CPUProc(int id) {
    auto cpu = CPU(id, q, state, "CPU" + std::to_string(id));
    cpu.run();
}

void GenProc() {
    ProcessGenerator gen = ProcessGenerator(q, state, "Generator");
    gen.run();
}

int main() {
    srand(time(nullptr));

    std::thread cpu1(CPUProc, 1);
    std::thread cpu2(CPUProc, 2);

    std::thread gen(GenProc);

    gen.join();
/*    int pidIncrementor = 1;
    while(true) {
        auto p = Process(rand() % 10, pidIncrementor);

        // TODO: check if process is free
        // TODO: pass to a buffer if someone is free
        // TODO: pass to a queue if everyone is busy

        q.push(p);
        pidIncrementor++;

        int sleepTime = rand() % 10;

        print("The next process will arrive in " + std::to_string(sleepTime) + " seconds\n");
        std::this_thread::sleep_for(std::chrono::seconds(sleepTime));

    }*/
}

// DONE: 1) remove params mutex from classes

// DONE: 2) add new mutex for print
// DONE: 3) add function for print to use mutex in the main
// DONE: 4) think about the ways to omit passing a process to the queue if one of CPUs is free
// TODO: 5) ask about measuring max queue size
// TODO: 6) add pretty prints
// TODO: 7) secure buffer with mutex
// TODO: 8) locate bug with wrong state management