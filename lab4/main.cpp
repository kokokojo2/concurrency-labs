#include <iostream>
#include <cstdlib>
#include <vector>
#include <future>

#include <bits/stdc++.h>


void fillArray(std::vector<int>& array) {
    for (int i = 0; i < array.size(); i++) {
        array[i] = rand() % 10;
    }
}

void multiplyArrayInPlace(std::vector<int>& array, int multiplier) {
    for (int i = 0; i < array.size(); i++) {
        array[i] = array[i] * multiplier;
    }
    std::sort(array.begin(), array.end());
}

std::vector<int> filterEven(std::vector<int>& array) {
    std::vector <int> filteredArray;
    for (int i = 0; i < array.size(); i++) {
        if(!(array[i] % 2)) {
            filteredArray.push_back(array[i]);
        }
    }
    std::sort(filteredArray.begin(), filteredArray.end());
    return filteredArray;
}


std::vector<int> filterMaxRange(std::vector<int>& array, float low, float high) {
    float max = 0;
    // finding max value
    for (int i = 0; i < array.size(); i++) {
        if(array[i] > max) {
            max = array[i];
        }
    }
    std::vector <int> filteredArray;
    // filtering array
    for (int i = 0; i < array.size(); i++) {
        if(array[i] / max >= low && array[i] / max <= high) {
            filteredArray.push_back(array[i]);
        }
    }
    std::sort(filteredArray.begin(), filteredArray.end());
    return filteredArray;
}

std::vector<int> computationProc1(int arraySize) {
    std::vector<int> array(arraySize);
    fillArray(array);
    multiplyArrayInPlace(array, 2);
    return array;
}

std::vector<int> computationProc2(int arraySize) {
    std::vector<int> array(arraySize);
    fillArray(array);
    return filterEven(array);
}

std::vector<int> computationProc3(int arraySize) {
    std::vector<int> array(arraySize);
    fillArray(array);
    return filterMaxRange(array, 0.4, 0.6);
}

std::vector<int> mergeArrays(
        const std::vector<int>& array1,
        const std::vector<int>& array2,
        const std::vector<int>& array3
) {
    std::vector<int> merged;
    for(int i = 0; i < array1.size(); i++) {
        if(std::binary_search(array2.begin(), array2.end(), array1[i]) &&
        !std::binary_search(array3.begin(), array3.end(), array1[i])) {
            merged.push_back(array1[i]);
        }
    }
    return merged;
}

void printVector(const std::vector<int>& vec) {
    for(int i = 0; i < vec.size(); i++) {
        std::cout << vec[i] << " ";
    }
    std::cout << std::endl;
}

int main() {
    srand(time(nullptr));
    int size = 20;
    std::future<std::vector<int>> futureComp1 = std::async(std::launch::async, computationProc1, size);
    std::future<std::vector<int>> futureComp2 = std::async(std::launch::async, computationProc2, size);
    std::future<std::vector<int>> futureComp3 = std::async(std::launch::async, computationProc3, size);

    std::vector<int> array1 = futureComp1.get();
    std::vector<int> array2 = futureComp2.get();
    std::vector<int> array3 = futureComp3.get();

    std::cout << "Array1: ";
    printVector(array1);

    std::cout << "Array2: ";
    printVector(array2);

    std::cout << "Array3: ";
    printVector(array3);

    std::vector<int> result = mergeArrays(array1, array2, array3);

    std::cout << "Merged array (item if item in array1 and item in array2 and item not in array3): ";
    printVector(result);

    return 0;
}