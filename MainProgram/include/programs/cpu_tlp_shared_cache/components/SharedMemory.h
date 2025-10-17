#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <mutex>

class SharedMemory {
public:
    static constexpr uint16_t MEM_SIZE = 512; // 512 posiciones de 64 bits

    struct AccessLog {
        std::string type;   // "READ", "WRITE", "LOAD"
        uint16_t address;
        uint64_t value;
    };

    SharedMemory();

    // Operaciones concurrencia (registran accesos)
    uint64_t read(uint16_t address);
    void write(uint16_t address, uint64_t value);

    // Accesos sin registro (lectura/ajuste directo)
    uint64_t get(uint16_t address) const;
    void set(uint16_t address, uint64_t value);

    // Carga desde archivo binario con alineamiento (1,2,4,8)
    bool loadFromFile(const std::string& path, uint16_t startAddr = 0, size_t align = 8);

    // Log de accesos
    void clearLog();
    std::vector<AccessLog> getLog() const;
    void printLog() const;

    // Reset total
    void reset();

private:
    mutable std::mutex memMutex;
    std::vector<uint64_t> memory;        // tamaño MEM_SIZE
    std::vector<AccessLog> accessLog;
};
