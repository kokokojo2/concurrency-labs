#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <iomanip>
#include <unistd.h>

#include "core.h"

std::mutex queueMutex, emptyQueueMutex, printMutex;
std::mutex condVariableMutex;
std::condition_variable newProcessCondVar;

void safePrint(const std::string &message, const std::string &prefix) {
    printMutex.lock();
    std::cout << std::left << std::setw(10) << std::setfill(' ') << prefix << '|' << message;
    printMutex.unlock();
}

void IRunnable::print(const std::string& message) const {
    safePrint(message, this->prefix);
}

IRunnable::IRunnable(const std::string &prefix) {
    this->prefix = prefix;
}

Process::Process(int executionTime, int id, const std::string& prefix) : IRunnable(prefix) {
    this->lifeTime = executionTime;
    this->pid = id;
}

int Process::getPid() const { return this->pid; }
int Process::getLifeTime() const { return this->lifeTime; }

void Process::run() {
    this->print(
            "Started executing. Duration - " + std::to_string(this->lifeTime) + " seconds.\n"
    );
    std::this_thread::sleep_for(std::chrono::seconds(this->lifeTime));
    this->print("Exited with code 0.\n");
}

Process::Process(const std::string &prefix) : IRunnable(prefix) {

}

CPU::CPU(int id, Queue &queue, State &state, const std::string &prefix) : IRunnable(prefix){
    this->queue = &queue;
    this->id = id;
    this->state = &state;

    this->print("Started.\n");
}

[[noreturn]] void CPU::run() {
    while(true) {
        Process process = Process("fd");

        std::unique_lock<std::mutex> lock{condVariableMutex};
        if (this->queue->empty()) {
            print("The process queue is empty, waiting for new process to arrive.\n");
            newProcessCondVar.wait(lock);
            process = this->state->getProcess();
        }
        else {
            print("Getting process from the queue.\n");
            process = this->queue->pop();
        }
        this->state->busy(this->id);

        print("Process(pid=" + std::to_string(process.getPid()) + ") received.\n");
        process.run();
        print("Available for the next task.\n");

        this->state->available(this->id);
    }
}

void Queue::push(const Process& process) {
    queueMutex.lock();
    this->queue.push(process);
    if(queue.size() == 1) {
        emptyQueueMutex.unlock();
    }
    queueMutex.unlock();
}

Process Queue::pop() {
    emptyQueueMutex.lock();
    queueMutex.lock();

    auto val = this->queue.front();
    this->queue.pop();

    if(!queue.empty()) {
        emptyQueueMutex.unlock();
    }
    queueMutex.unlock();

    return val;
}

bool Queue::empty() {
    return this->queue.empty();
}

ProcessGenerator::ProcessGenerator(Queue &queue, State &state, const std::string &prefix) : IRunnable(prefix) {
    this->queue = &queue;
    this->state = &state;
    print("Initialized.\n");
}

[[noreturn]] void ProcessGenerator::run() {
    while(true) {
        auto p = Process(
                10 + rand() % 9,
                this->nextPid,
                "Process" + std::to_string(this->nextPid)
        );
        this->print("Process(pid="+ std::to_string(p.getPid()) + ") was initialized.\n");

        if(this->state->isCPUAvailable()) {
            print(
                    "Some of the CPUs are available, passing the Process(pid="
                    + std::to_string(p.getPid()) + ") directly to the CPU.\n"
            );
            this->state->passProcess(p);
            newProcessCondVar.notify_one();
            this->print("Notified!\n");
        } else {
            print(
                    "There is no any available CPUs, pushing the Process(pid="
                    + std::to_string(p.getPid()) + ") to the queue."
            );
            this->queue->push(p);
        }
        // TODO: check if process is free
        // TODO: pass to a buffer if someone is free
        // TODO: pass to a queue if everyone is busy

        nextPid++;

        int sleepTime = 1 + rand() % 9;

        print("The next process will arrive in " + std::to_string(sleepTime) + " seconds\n");
        std::this_thread::sleep_for(std::chrono::seconds(sleepTime));

    }
}

void State::busy(int cpuId)  {
    if (cpuId == 1) this->CPU1isAvailable = false;
    if (cpuId == 2) this->CPU2isAvailable = false;
}

void State::available(int cpuId) {
    if (cpuId == 1) this->CPU1isAvailable = true;
    if (cpuId == 2) this->CPU2isAvailable = true;
}

bool State::isCPUAvailable() const {
    return this->CPU1isAvailable || this->CPU2isAvailable;
}

void State::passProcess(Process &process) {
    this->processBuffer = process;
}

Process State::getProcess() {
    return Process(this->processBuffer.getLifeTime(), this->processBuffer.getPid(), "fdf");
}


