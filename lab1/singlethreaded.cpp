#include <iostream>
#include <cstdlib>
#include <chrono>
#include <random>

int** allocateMatrix(int size) {
    int** matrix = new int* [size];
    for(int i = 0; i < size; i++) {
        matrix[i] = new int [size];
    }
    return matrix;
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

int main() {
    int matrixSize, sum = 0;

    std::cout << "Input matrix size: ";
    std::cin >> matrixSize;
    int **matrix = allocateMatrix(matrixSize);

    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < matrixSize; i++) {
        for(int j = 0; j < matrixSize; j++) {
            if (i == j) continue;
            int value = rand() % 500;
            matrix[i][j] = value;
            sum += value;
        }
        matrix[i][i] = sum;
    }
    auto stop = std::chrono::high_resolution_clock::now();

    std::cout << "Duration :" << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
    return 0;
}
