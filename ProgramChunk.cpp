#include "ProgramChunk.h"
#include "Instruction.h"

namespace DAsm{

    ProgramChunk::ProgramChunk(){
        mHasTargetPos=false;
        mTargetPos=0;
    }

    ProgramChunk::~ProgramChunk(){
    }

    unsigned int    ProgramChunk::GetLength(){
        unsigned int result = 0;

        for(auto && i : mInstructions){
            result += i.GetLength();
        }
        return result;
    }
}
