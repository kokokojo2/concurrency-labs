#include <mutex>
#include <iostream>
#include <queue>

#ifndef PC_LAB1_CORE_H
#define PC_LAB1_CORE_H
    class IRunnable {
        public:
            virtual void run() = 0;
            void print(const std::string& message) const;
            std::string prefix;
            explicit IRunnable(const std::string &prefix);
    };

    struct RandomRanges {
        int minSleepTime;
        int maxSleepTime;
        int minExecutionTime;
        int maxExecutionTime;
    };

    class Process : IRunnable {
        private:
            int lifeTime{};
            int pid{};
            std::string prefix;
        public:
            int getPid() const;
            int getLifeTime() const;
            Process(int executionTime, int id, const std::string& prefix);
            explicit Process(const std::string& prefix);
            void run() override;
    };

    class State {
        private:
            Process processBuffer = Process("fsdf");
        public:
            bool CPU1isAvailable = true;
            bool CPU2isAvailable = true;
            void busy(int cpuId);
            void available(int cpuId);
            bool isCPUAvailable() const;
            void passProcess(Process &process);
            Process getProcess();
            State() = default;
    };


    class Queue {
        private:
            std::queue<Process> queue;
        public:
            explicit Queue() = default;;
            void push(const Process& process);
            Process pop();
            bool empty();
    };

    class CPU : IRunnable {
        private:
            int id;
            Queue *queue;
            State *state;
        public:
            explicit CPU(int id, Queue &queue, State &state, const std::string &prefix);
            void run() override;
    };
    void safePrint(const std::string &message, const std::string &prefix);

    class ProcessGenerator : IRunnable {
        private:
            unsigned int maxProcesses;
            Queue *queue;
            State *state;
            RandomRanges *ranges;
        public:
            explicit ProcessGenerator(
                    Queue &queue,
                    State &state,
                    const std::string &prefix,
                    unsigned int procNum,
                    RandomRanges &ranges
            );
            void run() override;
    };

#endif