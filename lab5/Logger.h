#include <iostream>
#include <iomanip>
#include <mutex>

#ifndef PC_LAB1_LOGGER_H
#define PC_LAB1_LOGGER_H


class Logger {
public:
    void print(const std::string& message, bool safe) const;
    std::string prefix;
};


#endif //PC_LAB1_LOGGER_H
