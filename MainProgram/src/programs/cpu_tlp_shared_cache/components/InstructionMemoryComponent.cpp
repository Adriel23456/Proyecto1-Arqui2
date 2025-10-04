#include "programs/cpu_tlp_shared_cache/components/InstructionMemoryComponent.h"
#include <fstream>
#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>
#include <array>

namespace cpu_tlp {

    InstructionMemoryComponent::InstructionMemoryComponent()
        : m_sharedData(nullptr)
        , m_executionThread(nullptr)
        , m_memorySize(0)
        , m_isRunning(false)
        , m_processingPaused(false) {
    }

    InstructionMemoryComponent::~InstructionMemoryComponent() {
        shutdown();
    }

    bool InstructionMemoryComponent::initialize(std::shared_ptr<InstructionMemorySharedData> sharedData) {
        if (m_isRunning) {
            std::cerr << "[InstructionMemory] Component already running!" << std::endl;
            return false;
        }

        m_sharedData = sharedData;

        // Cargar el archivo de memoria de instrucciones
        if (!loadInstructionMemory()) {
            std::cerr << "[InstructionMemory] Failed to load instruction memory file" << std::endl;
            return false;
        }

        // Iniciar el hilo de ejecución
        m_isRunning = true;
        m_sharedData->should_stop = false;
        m_executionThread = std::make_unique<std::thread>(&InstructionMemoryComponent::threadMain, this);

        std::cout << "[InstructionMemory] Component initialized successfully" << std::endl;
        return true;
    }

    void InstructionMemoryComponent::shutdown() {
        if (!m_isRunning) return;

        std::cout << "[InstructionMemory] Shutting down..." << std::endl;

        // Señalar al hilo que debe detenerse
        if (m_sharedData) {
            m_sharedData->should_stop = true;
        }

        // Esperar a que termine el hilo
        if (m_executionThread && m_executionThread->joinable()) {
            m_executionThread->join();
        }

        m_isRunning = false;
        m_executionThread.reset();

        std::cout << "[InstructionMemory] Shutdown complete" << std::endl;
    }

    bool InstructionMemoryComponent::isRunning() const {
        return m_isRunning;
    }

    bool InstructionMemoryComponent::loadInstructionMemory() {
        std::string fullPath = std::string(RESOURCES_PATH) + INSTRUCTION_FILE_PATH;
        std::ifstream file(fullPath, std::ios::binary | std::ios::ate);

        if (!file.is_open()) {
            std::cerr << "[InstructionMemory] Could not open file: " << fullPath << std::endl;
            // Crear un archivo vacío mínimo para pruebas
            m_instructionMemory.clear();
            m_memorySize = 0;
            return true; // Continuamos aunque no haya archivo
        }

        // Obtener el tamaño del archivo
        m_memorySize = static_cast<size_t>(file.tellg());
        file.seekg(0, std::ios::beg);

        // Leer todo el archivo en memoria
        m_instructionMemory.resize(m_memorySize);
        file.read(reinterpret_cast<char*>(m_instructionMemory.data()), m_memorySize);
        file.close();

        std::cout << "[InstructionMemory] Loaded " << m_memorySize << " bytes from " << INSTRUCTION_FILE_PATH << std::endl;
        return true;
    }

    bool InstructionMemoryComponent::reloadInstructionMemory() {
        std::cout << "[InstructionMemory] Reloading instruction memory..." << std::endl;

        // Pausar temporalmente el procesamiento
        pauseProcessing();

        // Esperar un poco para asegurar que no hay lecturas activas
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // Intentar recargar el archivo
        bool success = loadInstructionMemory();

        if (success) {
            // Resetear los estados de todos los PEs para forzar recarga
            for (int i = 0; i < 4; ++i) {
                auto& connection = m_sharedData->pe_connections[i];
                connection.PC_F.store(0x0000000000000000ULL, std::memory_order_release);
                connection.INS_READY.store(false, std::memory_order_release);
            }
            std::cout << "[InstructionMemory] Reload successful" << std::endl;
        }
        else {
            std::cerr << "[InstructionMemory] Reload failed" << std::endl;
        }

        // Reanudar el procesamiento
        resumeProcessing();

        return success;
    }

    void InstructionMemoryComponent::pauseProcessing() {
        m_processingPaused.store(true, std::memory_order_release);
    }

    void InstructionMemoryComponent::resumeProcessing() {
        m_processingPaused.store(false, std::memory_order_release);
    }

    uint64_t InstructionMemoryComponent::bytesToUint64LittleEndian(const uint8_t* bytes) {
        uint64_t result = 0;
        for (int i = 0; i < 8; ++i) {
            result |= (static_cast<uint64_t>(bytes[i]) << (i * 8));
        }
        return result;
    }

    uint64_t InstructionMemoryComponent::readInstructionFromFile(uint64_t address) {
        // Verificar que la dirección esté dentro del rango válido
        // Asumiendo que las instrucciones son de 8 bytes (64 bits)
        if (address + 8 > m_memorySize) {
            // Dirección fuera de rango, retornar instrucción de error
            return ERROR_INSTRUCTION;
        }

        // Leer 8 bytes desde la dirección especificada
        const uint8_t* instructionBytes = &m_instructionMemory[address];

        // Convertir a uint64_t usando Little Endian
        return bytesToUint64LittleEndian(instructionBytes);
    }

    void InstructionMemoryComponent::processPERequest(int peIndex) {
        auto& connection = m_sharedData->pe_connections[peIndex];

        // Leer el PC_F actual (este es escrito por el PE)
        uint64_t currentPC = connection.PC_F.load(std::memory_order_acquire);

        // Variable estática por PE para detectar cambios en PC_F
        static std::array<uint64_t, 4> lastPC = { 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL,
                                                  0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL };

        // Detectar si PC_F cambió
        if (currentPC != lastPC[peIndex]) {
            // PC cambió, indicar que la instrucción no está lista
            connection.INS_READY.store(false, std::memory_order_release);

            // Simular latencia de lectura de memoria (opcional, puedes ajustar o eliminar)
            std::this_thread::sleep_for(std::chrono::microseconds(10));

            // Leer la instrucción desde la memoria
            uint64_t instruction = readInstructionFromFile(currentPC);

            // Escribir la instrucción en InstrF
            connection.InstrF.store(instruction, std::memory_order_release);

            // Indicar que la instrucción está lista
            connection.INS_READY.store(true, std::memory_order_release);

            // Actualizar el último PC procesado
            lastPC[peIndex] = currentPC;

            // Debug output (puedes comentar esto en producción)
            std::cout << "[InstructionMemory] PE" << peIndex
                << " - PC: 0x" << std::hex << currentPC
                << " - Instruction: 0x" << instruction << std::dec << std::endl;
        }
    }

    void InstructionMemoryComponent::threadMain() {
        std::cout << "[InstructionMemory] Thread started" << std::endl;

        // Marcar el componente como listo
        m_sharedData->component_ready.store(true, std::memory_order_release);

        // Procesar inicialmente todas las direcciones 0x0000000000000000
        for (int i = 0; i < 4; ++i) {
            processPERequest(i);
        }

        // Bucle principal del componente
        while (!m_sharedData->should_stop.load(std::memory_order_acquire)) {
            // Si el procesamiento está pausado, esperar
            if (m_processingPaused.load(std::memory_order_acquire)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            // Procesar las solicitudes de cada PE
            for (int i = 0; i < 4; ++i) {
                processPERequest(i);
            }

            // Pequeña pausa para no sobrecargar la CPU
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        std::cout << "[InstructionMemory] Thread ending" << std::endl;
    }

} // namespace cpu_tlp