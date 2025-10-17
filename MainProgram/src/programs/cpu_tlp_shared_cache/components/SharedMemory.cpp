#include "programs/cpu_tlp_shared_cache/components/SharedMemory.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <iterator>

SharedMemory::SharedMemory()
    : memory(MEM_SIZE, 0)
{
}

// ================== operaciones ==================
uint64_t SharedMemory::read(uint16_t address) {
    std::lock_guard<std::mutex> lock(memMutex);
    if (address >= MEM_SIZE) {
        std::cerr << "[SharedMemory] Read out of range: " << address << "\n";
        return 0;
    }
    uint64_t v = memory[address];
    accessLog.push_back({ "READ", address, v });
    return v;
}

void SharedMemory::write(uint16_t address, uint64_t value) {
    std::lock_guard<std::mutex> lock(memMutex);
    if (address >= MEM_SIZE) {
        std::cerr << "[SharedMemory] Write out of range: " << address << "\n";
        return;
    }
    memory[address] = value;
    accessLog.push_back({ "WRITE", address, value });
}

// ================== acceso sin registro ==================
uint64_t SharedMemory::get(uint16_t address) const {
    std::lock_guard<std::mutex> lock(memMutex);
    if (address >= MEM_SIZE) return 0;
    return memory[address];
}

void SharedMemory::set(uint16_t address, uint64_t value) {
    std::lock_guard<std::mutex> lock(memMutex);
    if (address >= MEM_SIZE) return;
    memory[address] = value;
}

// ================== carga desde archivo ==================
bool SharedMemory::loadFromFile(const std::string& path, uint16_t startAddr, size_t align) {
    if (align != 1 && align != 2 && align != 4 && align != 8) {
        std::cerr << "[SharedMemory] Invalid alignment: " << align << "\n";
        return false;
    }

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "[SharedMemory] Could not open file: " << path << "\n";
        return false;
    }

    std::streamsize size = file.tellg();
    if (size <= 0) {
        std::cerr << "[SharedMemory] Empty or invalid file: " << path << "\n";
        return false;
    }
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        std::cerr << "[SharedMemory] Error reading file: " << path << "\n";
        return false;
    }

    // Guardar en memoria con mutex
    {
        std::lock_guard<std::mutex> lock(memMutex);

        if (startAddr >= MEM_SIZE) {
            std::cerr << "[SharedMemory] startAddr out of range\n";
            return false;
        }

        size_t totalWords = (buffer.size() + align - 1) / align;
        if (startAddr + totalWords > MEM_SIZE) {
            totalWords = MEM_SIZE - startAddr;
            // truncate silently
        }

        size_t byteIndex = 0;
        for (size_t i = 0; i < totalWords; ++i) {
            uint64_t value = 0;
            size_t chunk = std::min<size_t>(align, buffer.size() - byteIndex);
            // copy little-endian into uint64_t
            for (size_t b = 0; b < chunk; ++b) {
                value |= (uint64_t(buffer[byteIndex + b]) << (8 * b));
            }
            memory[startAddr + i] = value;
            accessLog.push_back({ "LOAD", static_cast<uint16_t>(startAddr + i), value });
            byteIndex += chunk;
            if (byteIndex >= buffer.size()) break;
        }
    }

    std::cout << "[SharedMemory] Loaded " << path << " into memory at " << startAddr << "\n";
    return true;
}

// ================== log utils ==================
void SharedMemory::clearLog() {
    std::lock_guard<std::mutex> lock(memMutex);
    accessLog.clear();
}

std::vector<SharedMemory::AccessLog> SharedMemory::getLog() const {
    std::lock_guard<std::mutex> lock(memMutex);
    return accessLog;
}

void SharedMemory::printLog() const {
    std::lock_guard<std::mutex> lock(memMutex);
    std::cout << "---- Memory Access Log ----\n";
    for (const auto& e : accessLog) {
        std::cout << e.type << " [Addr " << e.address << "] = 0x"
            << std::hex << e.value << std::dec << "\n";
    }
    std::cout << "---------------------------\n";
}

// ================== reset ==================
void SharedMemory::reset() {
    std::lock_guard<std::mutex> lock(memMutex);
    std::fill(memory.begin(), memory.end(), 0);
    accessLog.clear();
}
