#include <iostream>
#include <thread>
#include <cstdlib>
#include <vector>
#include <chrono>
#include <random>
#include <fstream>

int** allocateMatrix(int size) {
    int** matrix = new int* [size];
    for(int i = 0; i < size; i++) {
        matrix[i] = new int [size];
    }
    return matrix;
}

void deallocateMatrix(int** matrix, int size) {
    for(int i = 0; i < size; i++) {
        delete [] matrix[i];
    }
    delete matrix;
}

void printMatrix(int** matrix, int size) {
    for(int i = 0; i < size; i++) {
        for(int j = 0; j < size; j++) {
            std::cout << matrix[i][j] << " ";
        }
        std::cout << std::endl;
    }
}

int intRand(const int & min, const int & max) {
    static thread_local std::mt19937 generator;
    std::uniform_int_distribution<int> distribution(min,max);
    return distribution(generator);
}

void fillMatrixSegment(int** matrix, int matrixSize, int startRowIndex, int endRowIndex) {
    for(int i = startRowIndex; i < endRowIndex; i++) {
        if (i == startRowIndex) std::cout << "Work started\n";
        int sum = 0;
        if (i >= matrixSize) break;
        for(int j = 0; j < matrixSize; j++) {
            if (i == j) continue;
            int value = intRand(0, 500); //rand() % 500;
            matrix[i][j] = value;
            sum += value;
        }
        matrix[i][i] = sum;
    }
}
int run(int matrixSize, int threadNum) {
    int **matrix = allocateMatrix(matrixSize);
    int segmentSize = (matrixSize + threadNum - 1) / threadNum;

    std::vector<std::thread> threadVector;

    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < matrixSize; i += segmentSize) {
        int startRow = i, endRow = i + segmentSize;
        if (i != 0) startRow++;

        threadVector.push_back(std::thread(fillMatrixSegment, std::ref(matrix), matrixSize, startRow, endRow));
    }

    for(int i = 0; i < threadVector.size(); i++) threadVector[i].join();
    auto stop = std::chrono::high_resolution_clock::now();

    deallocateMatrix(matrix, matrixSize);
    return std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
}

void runBench(int minMatrixSize, int maxMatrixSize, int sizeStep, int minThreadNum, int maxThreadNum) {
    std::ofstream resultCSV;
    resultCSV.open("results.csv");
    resultCSV << "Num of threads,Matrix size,Run duration\n";

    for (int i = minThreadNum; i <= maxThreadNum; i++) {
        for (int j = minMatrixSize; j <= maxMatrixSize; j+= sizeStep) {
            std::cout << "Starting run with num_of_threads=" << i << " size_of_matrix=" << j << std::endl;
            int runDuration = run(j, i);
            resultCSV << std::to_string(i) + "," + std::to_string(j) + "," + std::to_string(runDuration) + "\n";
        }
    }
}

int main() {
    srand(time(NULL));
    int matrixSize, threadNum;

    std::cout << "Input matrix size: ";
    std::cin >> matrixSize;

    std::cout << "Input thread number: ";
    std::cin >> threadNum;

    runBench(1000, 10000, 100, 1, 10);
    return 0;
}
