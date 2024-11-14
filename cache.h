#include <iostream>
#include <cmath>
#include <tuple>

const int DC_WR_BUFF_SIZE = 10;
const int DC_NUM_SETS = 10;
const int DC_SET_SIZE = 2;
const int DC_BLOCK_SIZE = 1;
const int DC_INVALID = 0;
const int DC_VALID = 1;
const int DC_DIRTY = 2;
const int MemRdLatency = 3;

static int cycles = 0;
struct cacheBLK{
    int tag;
    int status;
    int trdy;
}dCache[DC_NUM_SETS][DC_SET_SIZE];

struct writeBuffer{
    int tag;
    int trdy;
}dcWrBuffer[DC_WR_BUFF_SIZE];

std::tuple<int, int> analyze(unsigned int addr){
    int blkOffsetBits = std::log2(DC_BLOCK_SIZE);
    int indexMask = (unsigned)(DC_NUM_SETS - 1);
    int tagMask = ~indexMask;

    int blk = ((unsigned) addr) >> blkOffsetBits;
    int index = (int)(blk & indexMask);
    int tag = (int)(blk & tagMask);

    return std::make_tuple(index, tag);
}

bool judge(int index, int tag, int *slot){
    int lruTime = dCache[index][0].trdy;
    for(int i=0;i<DC_SET_SIZE;i++){
        if((dCache[index][i].tag==tag)&&(dCache[index][i].status!=DC_INVALID)){
            *slot = i;
            return true;
        }else{
            if(dCache[index][i].trdy<=lruTime){
                lruTime = dCache[index][i].trdy;
                *slot = i;
            }
        }
    }
    return false;
}

int wrBack(int tag, int time){
    return 2;
}

void deal_get(int index, int slot, int kind){
    if(kind == 1){
        dCache[index][slot].trdy = cycles;
    }else{
        dCache[index][slot].trdy = cycles;
        dCache[index][slot].status = DC_DIRTY;
    }
}

void deal_miss(int tag, int slot, int kind, int index){
    if(kind == 1){
        int trdy = MemRdLatency;
        if(dCache[index][slot].status == DC_DIRTY)
            trdy += wrBack(tag, cycles);
        dCache[index][slot].tag = tag;
        dCache[index][slot].trdy = cycles + trdy;
        dCache[index][slot].status = DC_VALID;
    }else if(kind==2){
        int trdy = 0;
        if(dCache[index][slot].status==2){
            trdy = wrBack(tag, cycles);
        }else{
            dCache[index][slot].status = DC_DIRTY;
        }
        trdy += MemRdLatency;
        dCache[index][slot].tag = tag;
        dCache[index][slot].trdy = cycles + trdy;
    }
}