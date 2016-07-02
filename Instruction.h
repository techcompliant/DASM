#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <iterator>
#include <algorithm>
#include <regex>

#include "DAsm.h"

namespace DAsm{

    class   Program;

    class   Instruction{
        public:

                                                Instruction();
                                                Instruction(std::string source, unsigned int recursion_counter = 0);
            virtual                             ~Instruction();

            void                                LoadSource(std::string source, unsigned int recursion_counter = 0);

            void                                Fill(word value, unsigned int amount);

            unsigned int                        GetLength();

            word                                ParseArg(std::string source, bool isA = false);

            opcode                              GetOpcode(std::string str);
            opcode                              GetSpecialOpcode(std::string str);

            void                                Error(std::string nError);

            std::list<word>                     mWords;

            static std::list<str_opcode>        mOpcodes;
            static std::list<str_opcode>        mSpecialOpcodes;
            static std::list<str_opcode>        mReg;

            static Program*                     mProgram;

    };
}
#endif
