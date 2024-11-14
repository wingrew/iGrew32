#include <cstdint>
#include "fast.h"

class ALU {
public:
    // ALU 执行函数
    int32_t execute(uint32_t funct, int32_t operand1, int32_t operand2, bool &B_J) {
    switch (funct) {
        case OP_ADD:
            return operand1 + operand2;      // 加法
        case OP_SUB:
            return operand1 - operand2;      // 减法
        case OP_MUL:
            return operand1 * operand2;      // 乘法
        case OP_DIV:
            if (operand2 == 0) {
                std::cerr << "Error: Division by zero" << std::endl;
                return 0;                    // 除零错误
            }
            return operand1 / operand2;      // 除法
        case OP_AND:
            return operand1 & operand2;      // 与运算
        case OP_OR:
            return operand1 | operand2;      // 或运算
        case OP_XOR:
            return operand1 ^ operand2;      // 异或运算
        case OP_NOR:
            return ~(operand1 | operand2);                // 非运算（只对operand1操作）
        case OP_SLT:
            if(operand1<operand2){
                return 1;
            }else{
                return 0;
            }
        case OP_SLLV:
            return operand2<<operand1;
        case OP_SRLV:
            return (int32_t) ((uint32_t) operand2)>>operand1;
        case OP_SRAV:
            return operand2>>operand1;
        case OP_BEQ:
            if(operand1==operand2){
                B_J = true;
            }
            return 0;
        case OP_BNE:
            if(operand1!=operand2){
                B_J = true;
            }
            return 0;
        case OP_SLL:
            return operand2<<operand1;
        case OP_SRL:
            return (int32_t) ((uint32_t) operand2)>>operand1;
        case OP_SRA:
            return operand2>>operand1;
        case OP_LW:
            return operand1 + operand2;
        case OP_SW:
            return operand1 + operand2;
        case OP_LWF:
            return operand1 + operand2;
        case OP_SWF:
            return operand1 + operand2;            
        default:
            std::cerr << "Error: Invalid operation" << std::endl;
            return 0;
        }
    }

    float fexecute(uint32_t funct, float operand1, float operand2) {
        switch (funct) {
            case OP_ADD:
                return operand1 + operand2;      // 加法
            case OP_SUB:
                return operand1 - operand2;      // 减法
            case OP_MUL:
                return operand1 * operand2;      // 乘法
            case OP_DIV:
                if (operand2 == 0) {
                    std::cerr << "Error: Division by zero" << std::endl;
                    return 0;                    // 除零错误
                }
                return operand1 / operand2;      // 除法
            default:
                std::cerr << "Error: Invalid operation" << std::endl;
                return 0;
        }
    }
};