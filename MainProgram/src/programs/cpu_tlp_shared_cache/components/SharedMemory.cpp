#include "programs/cpu_tlp_shared_cache/components/SharedMemory.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <iterator>

SharedMemory::SharedMemory()
    : memory(MEM_SIZE_WORDS, 0)
{
}

// ================== operaciones ==================
uint64_t SharedMemory::read(uint16_t address) {
    std::lock_guard<std::mutex> lock(memMutex);

    // Si la dirección está fuera de rango (>= 0x1000), retornar 0x0 silenciosamente
    if (address >= MEM_SIZE_BYTES) {
        return 0;
    }

    // Leer 8 bytes desde la dirección especificada (puede cruzar límites de words)
    uint64_t result = 0;

    for (int i = 0; i < 8; ++i) {
        uint16_t byteAddr = address + i;

        // Si nos salimos del rango, los bytes faltantes son 0x0
        if (byteAddr >= MEM_SIZE_BYTES) {
            break;
        }

        // Calcular word y posición del byte dentro del word
        uint16_t wordIndex = byteAddr / 8;
        uint8_t byteOffset = byteAddr % 8;

        // Extraer el byte correspondiente
        uint8_t byteValue = (memory[wordIndex] >> (byteOffset * 8)) & 0xFF;

        // Ensamblar en little-endian
        result |= (static_cast<uint64_t>(byteValue) << (i * 8));
    }

    accessLog.push_back({ "READ", address, result });
    return result;
}

void SharedMemory::write(uint16_t address, uint64_t value) {
    std::lock_guard<std::mutex> lock(memMutex);

    // Si la dirección está fuera de rango (>= 0x1000), no hacer nada silenciosamente
    if (address >= MEM_SIZE_BYTES) {
        return;
    }

    // Escribir 8 bytes desde la dirección especificada (puede cruzar límites de words)
    for (int i = 0; i < 8; ++i) {
        uint16_t byteAddr = address + i;

        // Si nos salimos del rango, detener
        if (byteAddr >= MEM_SIZE_BYTES) {
            break;
        }

        // Calcular word y posición del byte dentro del word
        uint16_t wordIndex = byteAddr / 8;
        uint8_t byteOffset = byteAddr % 8;

        // Extraer el byte del valor a escribir (little-endian)
        uint8_t byteValue = (value >> (i * 8)) & 0xFF;

        // Crear máscara para limpiar el byte específico
        uint64_t mask = ~(0xFFULL << (byteOffset * 8));

        // Actualizar solo ese byte en el word
        memory[wordIndex] = (memory[wordIndex] & mask) | (static_cast<uint64_t>(byteValue) << (byteOffset * 8));
    }

    accessLog.push_back({ "WRITE", address, value });
}

// ================== acceso sin registro ==================
uint64_t SharedMemory::get(uint16_t address) const {
    std::lock_guard<std::mutex> lock(memMutex);

    // Si la dirección está fuera de rango (>= 0x1000), retornar 0x0 silenciosamente
    if (address >= MEM_SIZE_BYTES) {
        return 0;
    }

    // Leer 8 bytes desde la dirección especificada
    uint64_t result = 0;

    for (int i = 0; i < 8; ++i) {
        uint16_t byteAddr = address + i;

        if (byteAddr >= MEM_SIZE_BYTES) {
            break;
        }

        uint16_t wordIndex = byteAddr / 8;
        uint8_t byteOffset = byteAddr % 8;

        uint8_t byteValue = (memory[wordIndex] >> (byteOffset * 8)) & 0xFF;
        result |= (static_cast<uint64_t>(byteValue) << (i * 8));
    }

    return result;
}

void SharedMemory::set(uint16_t address, uint64_t value) {
    std::lock_guard<std::mutex> lock(memMutex);

    // Si la dirección está fuera de rango (>= 0x1000), no hacer nada silenciosamente
    if (address >= MEM_SIZE_BYTES) {
        return;
    }

    // Escribir 8 bytes desde la dirección especificada
    for (int i = 0; i < 8; ++i) {
        uint16_t byteAddr = address + i;

        if (byteAddr >= MEM_SIZE_BYTES) {
            break;
        }

        uint16_t wordIndex = byteAddr / 8;
        uint8_t byteOffset = byteAddr % 8;

        uint8_t byteValue = (value >> (i * 8)) & 0xFF;
        uint64_t mask = ~(0xFFULL << (byteOffset * 8));

        memory[wordIndex] = (memory[wordIndex] & mask) | (static_cast<uint64_t>(byteValue) << (byteOffset * 8));
    }
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

    std::streamsize fileSize = file.tellg();
    if (fileSize <= 0) {
        std::cerr << "[SharedMemory] Empty or invalid file: " << path << "\n";
        return false;
    }
    file.seekg(0, std::ios::beg);



    // Capacidad real desde startAddr (en bytes)
    if (startAddr >= MEM_SIZE_BYTES) {
        std::cerr << "[SharedMemory] startAddr out of range\n";
        return false;
    }
    const size_t capacityFromStart = MEM_SIZE_BYTES - startAddr;
    const size_t bytesToRead = static_cast<size_t>(std::min<std::streamsize>(fileSize, static_cast<std::streamsize>(capacityFromStart)));

    std::vector<uint8_t> buffer(bytesToRead);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), bytesToRead)) {
        std::cerr << "[SharedMemory] Error reading file: " << path << "\n";
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(memMutex);

        // Reset total y log
        std::fill(memory.begin(), memory.end(), 0);
        accessLog.clear();

        // Escribir byte a byte en layout de 64-bit words (little-endian por byte)
        for (size_t i = 0; i < bytesToRead; ++i) {
            const uint16_t byteAddr = static_cast<uint16_t>(startAddr + i);
            const uint16_t wordIndex = byteAddr / 8;
            const uint8_t  byteOff = byteAddr % 8;

            const uint64_t mask = ~(0xFFULL << (byteOff * 8));
            const uint64_t v = static_cast<uint64_t>(buffer[i]) << (byteOff * 8);
            memory[wordIndex] = (memory[wordIndex] & mask) | v;

            // Opcional: logear cuando completamos un word o al final
            const bool completedWord = (byteOff == 7) || (i + 1 == bytesToRead);
            if (completedWord) {
                accessLog.push_back({ "LOAD", static_cast<uint16_t>(wordIndex * 8), memory[wordIndex] });
            }
        }

        // Mensaje de resultado
        std::cout << "[SharedMemory] Loaded " << bytesToRead << " bytes from " << path
            << " at byte address " << startAddr;

        if (static_cast<size_t>(fileSize) > bytesToRead) {
            // Recortado por falta de espacio
            std::cout << " (truncated " << (static_cast<size_t>(fileSize) - bytesToRead) << " bytes)";
        }
        else if (startAddr > 0 || bytesToRead < MEM_SIZE_BYTES) {
            // Quedó cero-fill en el resto
            const size_t zeroFilledBytes = MEM_SIZE_BYTES - bytesToRead - startAddr;
            if (zeroFilledBytes > 0)
                std::cout << " (filled " << zeroFilledBytes << " remaining bytes with 0x0)";
        }
        std::cout << "\n";
    }

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
        std::cout << e.type << " [Byte Addr " << e.address << "] = 0x"
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