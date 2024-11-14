enum opcodes{
    // R
    OP_R = 0,

    // R_shamt
    OP_RSHA,

    // R_F
    OP_F,

    // R_J
    OP_J = 0b11101,
    OP_JAL = 0b11110,
    
    // R_B
    OP_BEQ = 0b11001,
    OP_BNE = 0b11010,

    // I_MEM
    OP_LW = 0b10111,
    OP_SW = 0b11000,

    OP_LWF = 0b11011,
    OP_SWF = 0b11100
};

enum Funct{
    OP_ADD = 3,
    OP_SUB = 4,
    OP_MUL,
    OP_ADDU,
    OP_SUBU,
    OP_SLT,
    OP_SLTU,
    OP_AND,
    OP_OR,
    OP_XOR,
    OP_NOR,
    OP_SLLV,
    OP_SRLV,
    OP_SRAV,
    OP_JR,
    OP_DIV,
    OP_SLL,
    OP_SRL,
    OP_SRA,
    OP_JALR = 0b11111
};


