//
// Created by zaiyichen on 2023/12/19.
//

#ifndef FILEMAILMAN_UTILS_H
#define FILEMAILMAN_UTILS_H

#include <iostream>
#include <iomanip>
#include <spdlog/spdlog.h>

inline void printHexBytes(const char* data, std::size_t size) {
#ifdef WL_DEBUG
    spdlog::info("Data bytes in hex: ");
    for (std::size_t i = 0; i < size; ++i) {
        std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(static_cast<unsigned char>(data[i])) << ' ';
    }
    std::cout << std::dec << std::endl;
#endif
}
#endif //FILEMAILMAN_UTILS_H
