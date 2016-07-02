#ifndef PROGRAMCHUNK_H
#define PROGRAMCHUNK_H

#include "DAsm.h"


namespace DAsm{
    class Instruction;
    class   ProgramChunk{
        public:


                                                ProgramChunk();
            virtual                             ~ProgramChunk();

            bool                                mHasTargetPos;
            word                                mTargetPos;

            unsigned int                        GetLength();

            void                                SetLabels();


            std::list<label_value>              mLabelValues;
            std::list<Instruction>              mInstructions;
    };
}

#endif
