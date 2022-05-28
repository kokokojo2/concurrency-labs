
#include "Logger.h"

std::mutex printMutex;

void Logger::print(const std::string &message, bool safe) const {
    if (safe) {
        printMutex.lock();
    }
    std::cout << std::left << std::setw(10) << std::setfill(' ') << this->prefix << "| " << message << std::endl;
    if (safe) {
        printMutex.unlock();
    }
}
