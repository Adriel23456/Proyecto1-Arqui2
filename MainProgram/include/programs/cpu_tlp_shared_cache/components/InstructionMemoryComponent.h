#pragma once
#include "programs/cpu_tlp_shared_cache/components/SharedData.h"
#include <thread>
#include <memory>
#include <vector>
#include <string>
#include <atomic>

namespace cpu_tlp {

    class InstructionMemoryComponent {
    public:
        InstructionMemoryComponent();
        ~InstructionMemoryComponent();

        // Inicializa el componente y comienza la ejecución asíncrona
        bool initialize(std::shared_ptr<InstructionMemorySharedData> sharedData);

        // Detiene el componente y espera a que termine el hilo
        void shutdown();

        // Verifica si el componente está corriendo
        bool isRunning() const;

        // Recarga el archivo de memoria de instrucciones
        bool reloadInstructionMemory();

        // Pausa temporalmente el procesamiento (para permitir escritura del archivo)
        void pauseProcessing();

        // Resume el procesamiento
        void resumeProcessing();

    private:
        // Función principal del hilo de ejecución
        void threadMain();

        // Lee la instrucción desde el archivo binario
        uint64_t readInstructionFromFile(uint64_t address);

        // Carga el archivo de memoria de instrucciones
        bool loadInstructionMemory();

        // Procesa las solicitudes de un PE específico
        void processPERequest(int peIndex);

        // Convierte bytes a uint64 en Little Endian
        uint64_t bytesToUint64LittleEndian(const uint8_t* bytes);

    private:
        std::shared_ptr<InstructionMemorySharedData> m_sharedData;
        std::unique_ptr<std::thread> m_executionThread;

        // Buffer para almacenar el contenido del archivo de instrucciones
        std::vector<uint8_t> m_instructionMemory;

        // Tamaño del archivo de instrucciones
        size_t m_memorySize;

        // Flag interno para el estado del componente
        bool m_isRunning;

        // Flag para pausar el procesamiento durante la recarga
        std::atomic<bool> m_processingPaused;

        // Instrucción de error (cuando la dirección está fuera de rango)
        static constexpr uint64_t ERROR_INSTRUCTION = 0x4D00000000000000ULL;

        // Path al archivo de instrucciones
        static constexpr const char* INSTRUCTION_FILE_PATH = "Assets/CPU_TLP/InstMem.bin";
    };

} // namespace cpu_tlp
