#include <iostream>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <vector>

#include <cstdlib>

int sum = 0;
int mutexLockedSum = 0;
std::mutex sumMutex;

std::atomic<int> atomicSum(0);

void fillArray(int* array, int size) {
    for (int i = 0; i < size; i++) {
        array[i] = rand() % 100;
    }
}

long sequentialSum(const int* array, int size) {
    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < size; i++) {
        if(array[i] % 10 == 0) {
            sum += array[i];
        }
    }
    auto stop = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
}

void mutexBasedParallelSum(int* array, int start, int stop) {
    for(int i = start; i < stop; i++) {
        if(array[i] % 10 == 0) {
            sumMutex.lock();
            mutexLockedSum += array[i];
            sumMutex.unlock();
        }
    }
}

void atomicBasedParallelSum(int* array, int start, int stop) {
    for(int i = start; i < stop; i++) {
        if(array[i] % 10 == 0) {
            atomicSum += array[i];
        }
    }
}

long runThreaded(int* array, int size, int maxThreads, std::function<void(int*, int, int)> func) {
    std::vector<std::thread> threadVector;

    int chunkSize = (size + maxThreads - 1) / maxThreads;
    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < size; i += chunkSize) {
        int startElement = i, stopElement = i + chunkSize;
        if (i != 0) startElement++;
        if (stopElement > size) stopElement = size;

        threadVector.push_back(std::thread(func, std::ref(array), startElement, stopElement));
    }
    for(int i = 0; i < threadVector.size(); i++) threadVector[i].join();
    auto stop = std::chrono::high_resolution_clock::now();

    return std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
}


int main() {
    srand(time(nullptr));

    int size = 900000, threadNum, array[size];
    fillArray(array, size);

    std::cout << "Input number of threads: ";
    std::cin >> threadNum;

    auto time = sequentialSum(array, size);
    std::cout << "Sequential sum=" << sum << ". Calculated in " << time << " ms."<< std::endl;

    time = runThreaded(array, size, threadNum, mutexBasedParallelSum);
    std::cout << "Threaded with mutex lock sum=" << mutexLockedSum << ". Calculated in " << time << " ms."<< std::endl;

    time = runThreaded(array, size, threadNum, atomicBasedParallelSum);
    std::cout << "Threaded with atomic variable sum=" << atomicSum << ". Calculated in " << time << " ms."<< std::endl;

    return 0;
}
