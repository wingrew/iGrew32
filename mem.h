#include <stdexcept>
#include <cstdint>
#include <iostream>
#include <cassert>
#include <cstring>
#define NUM_REGISTERS 32    // 寄存器数量
#define REGISTER_SIZE 32    // 寄存器大小，单位位（64 位）
#define MEMORY_SIZE 0x80000

// 存储器结构
class Memory {
public:
    Memory() : data() {}
    
    // 读取数据
    template <typename T>
    T read(unsigned int addr) const {
        static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable");
        if (addr + sizeof(T) > MEMORY_SIZE) {
            throw std::out_of_range("Address out of bounds");
        }
        T value;
        std::memcpy(&value, &data[addr], sizeof(T));
        return value;
    }

    // 写入数据
    template <typename T>
    void write(unsigned int addr, const T& value) {
        static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable");
        if (addr + sizeof(T) > MEMORY_SIZE) {
            throw std::out_of_range("Address out of bounds");
        }
        std::memcpy(&data[addr], &value, sizeof(T));
    }

private:
    char data[MEMORY_SIZE];  // 内存空间
};

// 寄存器文件结构
class RegisterFile {
public:
    RegisterFile() : registers() {};

    // 读取寄存器值
    uint32_t read(unsigned int reg) const {
        if (reg >= NUM_REGISTERS) {
            throw std::out_of_range("Register index out of bounds");
        }
        return registers[reg];
    }

    // 写入寄存器值
    void write(unsigned int reg, int32_t value) {
        if (reg >= NUM_REGISTERS) {
            throw std::out_of_range("Register index out of bounds");
        }
        registers[reg] = value;
    }

    float fread(unsigned int reg) const {
        if (reg >= NUM_REGISTERS) {
            throw std::out_of_range("Register index out of bounds");
        }
        return fregisters[reg];
    }

    // 写入寄存器值
    void fwrite(unsigned int reg, float value) {
        if (reg >= NUM_REGISTERS) {
            throw std::out_of_range("Register index out of bounds");
        }
        fregisters[reg] = value;
    }

private:
    int32_t registers[NUM_REGISTERS];  // 寄存器文件
    float fregisters[NUM_REGISTERS];
};