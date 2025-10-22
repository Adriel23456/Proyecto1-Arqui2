#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <mutex>
#include <cassert>

// SharedMemory es la RAM PRIVADA del programa (4096 bytes = 512 posiciones de 64 bits)
// El archivo .bin es OPCIONAL y solo se usa para cargar valores iniciales
class SharedMemory {
public:
    static constexpr uint16_t MEM_SIZE_BYTES = 4096;  // 4096 bytes totales
    static constexpr uint16_t MEM_SIZE_WORDS = 512;   // 512 posiciones de 64 bits

    struct AccessLog {
        std::string type;   // "READ", "WRITE", "LOAD"
        uint16_t address;   // dirección en BYTES
        uint64_t value;
    };

    SharedMemory();

    // Operaciones concurrencia (registran accesos)
    // address: dirección en BYTES
    uint64_t read(uint16_t address);
    void write(uint16_t address, uint64_t value);

    // Accesos sin registro (lectura/ajuste directo)
    // address: dirección en BYTES
    uint64_t get(uint16_t address) const;
    void set(uint16_t address, uint64_t value);

    // Carga OPCIONAL desde archivo binario con alineamiento (1,2,4,8)
    bool loadFromFile(const std::string& path, uint16_t startAddr = 0, size_t align = 8);

    // Log de accesos
    void clearLog();
    std::vector<AccessLog> getLog() const;
    void printLog() const;

    // Reset total (vuelve a 0 toda la RAM)
    void reset();

    // Helpers deterministas (opcionales): inicializa 4 words sucesivos (0,8,16,24)
    void initTestPattern_0_1_2_3();

private:
    // Lee/clampa dirección (en BYTES) a rango [0..MEM_SIZE_BYTES-1]
    static inline uint16_t clampAddr(uint16_t addr) {
        return static_cast<uint16_t>(addr % MEM_SIZE_BYTES);
    }

    mutable std::mutex memMutex;
    std::vector<uint64_t> memory;        // 512 words de 64 bits = 4096 bytes
    std::vector<AccessLog> accessLog;
};
