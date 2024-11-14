#include "mem.h"
#include "alu.h"
#include <iostream>
#include <fstream>
#include <bitset>
#include <string>
#include <algorithm>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include "cache.h"
#include <tuple>
// opcode < 2 (funct) or opcode > 21 (opcode)
const char *R[] = {"0", "1", "2", "add", "sub", "mul", "addu", "subu", "slt", "sltu", "and", "or", "xor", "nor", "sllv", "srlv",\
                 "srav", "jr", "div", "sll", "srl", "sra", "lui", "lw", "sw", "beq", "bne", "lwf", "swf", "j", "jal", "jalr"};
 // opcode == 2 (funct)
const char *F[] = {"0", "1", "2", "addf", "subf", "mulf"};
// 2 < opcode < 22 
const char *I[] = {"0", "1", "2", "addi", "4", "muli", "addiu", "7", "slti", "sltiu", "andi", "ori", "xori"};

const int BUFFER_SIZE = 7;  // 缓冲池大小
std::queue<std::tuple<uint32_t, uint32_t>> buffer;     // 指令缓冲池
std::mutex mtx;             // 互斥锁，用于保护缓冲池
std::condition_variable cv; // 条件变量，用于通知生产者和消费者

bool done = false;          // 用于停止生产者和消费者的标志
int signal = 0;
uint32_t PC = 0x20000;
uint32_t pc_data = 0;
int CLK;
int Wait_time = 0;
uint32_t inst;
extern int cycles;
int instruct_count = 0;
void cpu(Memory &mem, RegisterFile &regFile, ALU &alu){
    // 初始化 
    int ALU_int;
    float ALU_float;
    int ZERO;
    int x = 0;
    // std::cout<<"address"<<"         "<<"instruction"<<std::endl;
    do{ 
        instruct_count += 1;
        // 取指
        inst = mem.read<uint32_t>(PC);
        // printf("%08x        %08x\n", PC, inst);

        
        if(inst==0) {
            done= true; 
            break;
        }

        cycles += 1;
        // 译码
        uint32_t OPCODE = (inst>>26) & 0x3f;
        uint32_t rs1 = (inst>>21) & 0x1f;
        uint32_t rs2 = (inst>>16) & 0x1f;
        int32_t RS1 = regFile.read((inst>>21) & 0x1f);
        int32_t RS2 = regFile.read((inst>>16) & 0x1f);
        float RS1_f = regFile.fread((inst>>21) & 0x1f);
        float RS2_f = regFile.fread((inst>>16) & 0x1f);
        uint32_t rd = (inst>>11) & 0x1f;
        int32_t IMM = (inst & 0xffff) | ((inst & 0x8000) ? 0xffff0000 : 0x00000000);
        uint32_t UIMM = (uint32_t)(inst & 0xffff);
        uint32_t shamt = (inst>>6) & 0x1f;
        uint32_t FUNC = inst & 0x3f;
        uint32_t OFFSET = inst & 0x3ffffff;
        int ALU_int = 0;
        float ALU_float = 0.0;
        bool memwrite = false;
        bool memread = false;
        bool memfwrite = false;
        bool memfread = false;
        bool regwrite = false;
        bool regfwrite = false;
        bool B_J = false;
        bool J = false;
        bool JAL = false;
        bool JALR = false;
        bool JR = false;
        int temp_mem = 0;
        float temp_memf = 0.0;

        cycles += 1;
        // 执行
        switch(OPCODE){
            case OP_R:regwrite = true;ALU_int = alu.execute(FUNC, RS1, RS2, B_J);cycles += 1;break;
            case OP_F:regfwrite = true;ALU_float = alu.fexecute(FUNC, RS1_f, RS2_f);cycles += 1;break;
            case OP_RSHA:regwrite = true;ALU_int = alu.execute(FUNC, shamt, RS2, B_J);cycles += 1;break;
            case OP_J:J = true;break;
            case OP_JALR:JALR = true;break;                        
            case OP_JAL:JAL = true;break;  
            case OP_JR:JR = true;break;                     
            case OP_BEQ:ALU_int = alu.execute(OPCODE, RS1, RS2, B_J);cycles += 1;break;                    //PC
            case OP_BNE:ALU_int = alu.execute(OPCODE, RS1, RS2, B_J);cycles += 1;break;                    //PC
            case OP_LW:memread = true;rd = rs2;regwrite = true;ALU_int = alu.execute(OPCODE, RS1, IMM, B_J);cycles += 1;break;      //PC
            case OP_SW:memwrite = true;ALU_int = alu.execute(OPCODE, RS1, IMM, B_J);cycles += 1;break;     //PC
            case OP_LWF:memfread = true;rd = rs2;regfwrite = true;ALU_int = alu.execute(OPCODE, RS1, IMM, B_J);cycles += 1;break;     //PC
            case OP_SWF:{memfwrite = true;ALU_int = alu.execute(OPCODE, RS1, IMM, B_J);cycles += 1;break;}    //PC
            default:
            regwrite = true;rd = rs2;ALU_int = alu.execute(OPCODE, IMM, RS1, B_J);cycles += 1;
        }
        
        std::unique_lock<std::mutex> lock(mtx);  // 加锁
        cv.wait(lock, []{ return buffer.size() < BUFFER_SIZE; }); 
        buffer.push(std::make_tuple(inst, ALU_int));
        cv.notify_all();  // 通知消费者
        lock.unlock();    // 解锁

        // 访存
        if(memread){
            ALU_int = mem.read<int32_t>(ALU_int);
            cycles += 1;
        }else if(memfread){
            ALU_float = mem.read<float>(ALU_int);
            cycles += 1;
        }else if(memwrite){
            mem.write<int32_t>(ALU_int, RS2);
            cycles += 1;
        }else if(memfwrite){
            mem.write<float>(ALU_int, RS2_f);
            cycles += 1;
        }

        // 写回
        if(regwrite){
            regFile.write(rd, ALU_int);
            cycles += 1;
        }else if(regfwrite){
            regFile.fwrite(rd, ALU_float);
            cycles += 1;
        }else if(JAL){
            regFile.write(31, PC+4);
            cycles += 1;
        }else if(JALR){
            regFile.write(31, PC+4);
            cycles += 1;
        }

        if(B_J){
            PC = PC + IMM;
        }else if(J){
            PC = (PC+4)&0xf0000000 + OFFSET;
        }else if(JAL){
            PC = (PC+4)&0xf0000000 + OFFSET;
        }else if(JR){
            PC = RS1;
        }else if(JALR){
            PC = RS1;
        }else{
            PC += 4;
        }
    }while(inst != 0);
}

/// @brief ///////////////////////////////////////////
struct INST {
    uint32_t OPCODE;
    uint32_t rs1;
    uint32_t rs2;
    uint32_t rd;
    uint32_t funct;
    uint32_t addr;
};

struct STAGE{
    bool block;
    bool busy;
    bool valid;
    struct INST insts;
};

STAGE stageIF, stageID, stageEX, stageMEM, stageWB, stagezero;

int SIGN = 0;

void s_initial(STAGE &one){
    one.block = false;
    one.busy = false;
    one.insts.OPCODE = 0;
    one.insts.rs1 = 0;
    one.insts.rs2 = 0;
    one.insts.rd = 0;
    one.valid = true;
    one.insts.addr = 0;
}

// IF 阶段 - 指令获取
void IF_stage() {
    if(stageMEM.block || stageEX.block || stageID.block){
        stageIF.block = true;
        stagezero.valid = true;
    }else if(stageIF.busy || stageIF.insts.OPCODE == OP_JR || stageIF.insts.OPCODE == OP_J || stageIF.insts.OPCODE == OP_BEQ || stageIF.insts.OPCODE == OP_BNE){
        stagezero.valid = false;
        stageIF.block = true;

    }else{
        stagezero.valid = true;
    }
}

// ID 阶段 - 指令译码
void ID_stage() {
    if(stageMEM.block || stageEX.block){
        stageID.block = true;
    }else if(((stageID.insts.OPCODE == OP_JR || stageID.insts.OPCODE == OP_J || stageID.insts.OPCODE == OP_BEQ || stageID.insts.OPCODE == OP_BNE) && ((stageIF.insts.rs1 == stageID.insts.rs2) || stageIF.insts.rs2 == stageID.insts.rs2))){
        stageIF.valid = false;
        stageID.block = true;
        
    } 
}

// EX 阶段 - 执行
void EX_stage() {
    if(stageEX.busy || stageMEM.block){
        stageEX.block = true;
    }else if((stageEX.insts.OPCODE == OP_LW || stageEX.insts.OPCODE == OP_LWF) && (stageID.insts.rs1 == stageEX.insts.rs2 || stageID.insts.rs2 == stageEX.insts.rs2)){
        stageID.valid = false;
        stageEX.block = true;

    }
}

// MEM 阶段 - 访存
void MEM_stage() {
    if(stageMEM.busy){
        stageMEM.block = true;
        stageMEM.valid = false;
    }else{
        stageMEM.block = false;
    }
}

// WB 阶段 - 写回
void WB_stage() {
    stageMEM.block = false;
}

void cache(){
    if(Wait_time > 1){
        Wait_time -= 1; 
        SIGN = -1;
        signal = -1;
        return ;
    }else if(Wait_time == 1){
        Wait_time -= 1;
        stageMEM.busy = false;
        return ;
    }
    int kind = 0;
    if(stageMEM.insts.OPCODE == OP_LW || stageMEM.insts.OPCODE == OP_LWF){
        kind = 1;
    }else if(stageMEM.insts.OPCODE == OP_SW || stageMEM.insts.OPCODE == OP_SWF){
        kind = 2;
    }
    if(kind != 0){
        auto [index, tag] = analyze(stageMEM.insts.addr);
        int slot = 0;
        if(judge(index, tag, &slot)){
            deal_get(index, slot, kind);
        }else{
            deal_miss(tag, slot, kind, index);
            stageMEM.busy = true;
            Wait_time = 6;
            cycles += 6;
            SIGN = -1;
            signal = -1;
        }
    }
}


void time_order(){
    const char* IF;
    const char* ID;
    const char* EX;
    const char* MEM;
    const char* WB;
    int cpu_cycles = 0; 
    while (!done){
        uint32_t inst;
        uint32_t opcode;
        uint32_t rs1;
        uint32_t rs2;
        uint32_t rd;
        uint32_t funct;
        uint32_t addr;
        cpu_cycles += 1; 
        if(SIGN==0){
            std::unique_lock<std::mutex> lock(mtx);  // 加锁
            cv.wait(lock, []{ return !buffer.empty(); });  // 等待直到缓冲池不为空
            std::tie(inst, addr) = buffer.front();   // 获取指令
            cv.notify_all();  // 通知生产者
            lock.unlock();    // 解锁
            printf("-------------------Five-Stage------------------\n");
            s_initial(stageID);
            s_initial(stageEX);
            s_initial(stageMEM);
            s_initial(stageWB);
            s_initial(stagezero);
            SIGN = 1;
            opcode = (inst>>26) & 0x3f;
            rs1 = (inst>>21) & 0x1f;
            rs2 = (inst>>16) & 0x1f;
            rd = (inst>>11) & 0x1f;
            funct = inst & 0x3f;
            stageIF.valid = true;
        }else{
            std::unique_lock<std::mutex> lock(mtx);  // 加锁
            cv.wait(lock, []{ return !buffer.empty(); });  // 等待直到缓冲池不为空
            if(SIGN != -1){
                buffer.pop();   // 从缓冲池移除指令
            }
            std::tie(inst, addr) = buffer.front();  // 获取指令
            cv.notify_all();  // 通知生产者
            lock.unlock();  
            opcode = (inst>>26) & 0x3f;
            rs1 = (inst>>21) & 0x1f;
            rs2 = (inst>>16) & 0x1f;
            rd = (inst>>11) & 0x1f;
            funct = inst & 0x3f;
            SIGN = 1;
        }

        stageIF.block = false;
        stageIF.busy = false;
        if(signal == 0){
            stageIF.insts.OPCODE = opcode;
            stageIF.insts.funct = funct;
        }

        stageIF.insts.rs1 = rs1;
        stageIF.insts.rs2 = rs2;
        stageIF.insts.rd = rd;
        stageIF.insts.addr = addr;
        stagezero.valid = true;
        
        printf("cycles:%d --IF------ID------EX------MEM------WB--\n", cpu_cycles);
        if(stageIF.valid == false){
            IF = "block";
        }else{
            if(stageIF.insts.OPCODE < 2){
                IF = R[stageIF.insts.funct];
            }else if(stageIF.insts.OPCODE > 21){
                IF = R[stageIF.insts.OPCODE];
            }else if(stageIF.insts.OPCODE == 2){
                IF = F[stageIF.insts.funct];
            }else{
                IF = I[stageIF.insts.OPCODE];
            }
        }
        if(stageID.valid == false){
            ID = "block";
        }else{
            if(stageID.insts.OPCODE < 2){
                ID = R[stageID.insts.funct];
            }else if(stageID.insts.OPCODE > 21){
                ID = R[stageID.insts.OPCODE];
            }else if(stageID.insts.OPCODE == 2){
                ID = F[stageID.insts.funct];
            }else{
                ID = I[stageID.insts.OPCODE];
            }
        }

        if(stageEX.valid == false){
            EX = "block";
        }else{
            if(stageEX.insts.OPCODE < 2){
                EX = R[stageEX.insts.funct];
            }else if(stageEX.insts.OPCODE > 21){
                EX = R[stageEX.insts.OPCODE];
            }else if(stageEX.insts.OPCODE == 2){
                EX = F[stageEX.insts.funct];
            }else{
                EX = I[stageEX.insts.OPCODE];
            }
        }
        if(stageMEM.valid == false){
            MEM = "block";
        }else{
            if(stageMEM.insts.OPCODE < 2){
                MEM = R[stageMEM.insts.funct];
            }else if(stageMEM.insts.OPCODE > 21){
                MEM = R[stageMEM.insts.OPCODE];
            }else if(stageMEM.insts.OPCODE == 2){
                MEM = F[stageMEM.insts.funct];
            }else{
                MEM = I[stageMEM.insts.OPCODE];
            }
        } 
        if(stageWB.valid == false){
            WB = "block";
        }else{
            if(stageWB.insts.OPCODE < 2){
                WB = R[stageWB.insts.funct];
            }else if(stageWB.insts.OPCODE > 21){
                WB = R[stageWB.insts.OPCODE];
            }else if(stageWB.insts.OPCODE == 2){
                WB = F[stageWB.insts.funct];
            }else{
                WB = I[stageWB.insts.OPCODE];
            }
        } 
        printf("--%s------%s------%s------%s-------%s--\n\n", IF, ID, EX, MEM, WB);

        cache();
        WB_stage();
        MEM_stage();
        EX_stage();
        ID_stage();
        IF_stage();
        
        if(stageWB.block == false){
            stageWB.insts = stageMEM.insts;
        }

        if(stageMEM.block == false){
            stageMEM.insts = stageEX.insts;
        }

        if(stageEX.block == false){
            stageEX.insts = stageID.insts;
        }

        if(stageID.block == false){
            stageID.insts = stageIF.insts;
        }


        stageWB.valid = stageMEM.valid;
        if(Wait_time > 0){
            stageMEM.valid = true;
        }else{
            stageMEM.valid = stageEX.valid;
            stageEX.valid = stageID.valid;
            stageID.valid = stageIF.valid;
            stageIF.valid = stagezero.valid;
        }
            

        if(stageWB.valid == false){
            s_initial(stageWB);
            stageWB.valid = false;
        }

        if(stageMEM.valid == false){
            s_initial(stageMEM);
            stageMEM.valid = false;
        }

        if(stageEX.valid == false){
            s_initial(stageEX);
            stageEX.valid = false;
        }

        if(stageID.valid == false){
            s_initial(stageID);
            stageID.valid = false;
            SIGN = -1;
        }

        if(stageIF.valid == false){
            signal = -1;
            s_initial(stageIF);
            stageIF.valid = false;

        }else{
            signal = 0;
        }
        stageWB.block = false;
        stageMEM.block = false;
        stageEX.block = false;
        stageID.block = false;
        stageIF.block = false;
        
    }
}

///////////////////////////////////////////////////////////////
int main(){
    //初始化
    Memory mem;
    RegisterFile regFile;
    ALU alu;

    // 写入数据和指令
    regFile.write(1, pc_data);
    pc_data += 64*4;
    mem.write<float>(pc_data, 0.9);
    pc_data += 4;
    mem.write<int32_t>(pc_data, 64);
    pc_data += 4;
    mem.write<float>(pc_data, 0.5);

    std::string filename = "./instruct";  // 文件名
    std::ifstream inputFile(filename);   // 打开文件
    std::string binaryString;
    
    // 检查文件是否成功打开
    if (!inputFile.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
    }
    uint32_t pc = PC;
    // 每次从文件中读取一行二进制字符串
    while (std::getline(inputFile, binaryString)) {
        // 假设每行都是32位的二进制数
        if (binaryString.size() != 32) {
            std::cout << "Binary: " << binaryString <<binaryString.size()<<std::endl;
            std::cerr << "Invalid input in file: Expected 32-bit binary number" << std::endl;
            continue;
        }
        // std::reverse(binaryString.begin(), binaryString.end());
        // 使用bitset转换二进制字符串为整数
        std::bitset<32> bitsetValue(binaryString);
        int32_t intValue = static_cast<int32_t>(bitsetValue.to_ulong());
        mem.write<int32_t>(pc, intValue);
        pc += 4;
    }
    inputFile.close();  // 关闭文件
    
    std::thread productor(cpu, std::ref(mem), std::ref(regFile), std::ref(alu));
    std::thread consumerThread(time_order);
    cv.notify_all();
    consumerThread.join();
    productor.join();
    printf("instruct count:%d   ", instruct_count);
    printf("cycles count:%d\n", cycles);
    printf("CPI:%.2f\n", cycles*1.0/(instruct_count*1.0));
    printf("Simulator time:%.2fps\n", cycles*1.0/(instruct_count*1.0)*325*instruct_count);
    // for(int i=0;i<64;i++){
    //     std::cout<<mem.read<float>(i*4)<<std::endl;
    // }
}