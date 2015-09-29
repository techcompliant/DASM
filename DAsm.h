/*
    Copyright (C) 2015 TechCompliant

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*/

#ifndef DASM_H
#define DASM_H

#include <cstdint>
#include <list>
#include <string>

namespace DAsm{

typedef uint16_t word;
typedef uint8_t opcode; //NB opcodes only actually need 5 bits

const unsigned long DCPU_RAM_SIZE = 65536;

const unsigned int MAX_RECURSION = 20;

struct  str_opcode{
    std::string  str;
    opcode  value;
    str_opcode(std::string nStr, opcode nValue){str=nStr; value=nValue;}
};
//Store words to be replaced with expression values
struct  expression_target{
    std::string expression;
    word*   target;
    expression_target(std::string nExpression, word* nTarget){expression=nExpression; target=nTarget;}
};

//Store values of labels
struct label_value{
    std::string label;
    word    value;
    label_value(std::string nLabel, word nValue){label=nLabel; value=nValue;}
};

struct macro{
    std::string keyword;
    std::string format;
};

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

class   Program{
    public:
                                            Program();
        virtual                             ~Program();

        bool                                LoadSource(std::string source);

        void                                AddExpressionTarget(std::string nExpression,word* nTarget);

        void                                AddDefineOnlyTarget(std::string nExpression,word* nTarget);

        void                                AddIncrementTarget(std::string nExpression,word* nTarget);

        void                                AddLabelValue(std::string nLabel, word nValue);
        void                                AddDefine(std::string nLabel, word nValue);

        unsigned int                        GetLength();
        unsigned int                        mLength;

        std::string                         ToHex(std::string seperator = " 0x");

        int                                 Evaluate(std::string expression);

        bool                                IsMacro(std::string keyword);
        std::string                         DoMacro(std::string keyword, std::list<std::string> args);
        void                                AddMacro(std::string nMacro);

        std::list<macro>                    mMacros;

        word*                               ToBlock(unsigned long maxSize = DCPU_RAM_SIZE);
        word*                               ToBlock(unsigned long maxSize, unsigned long& programSize);

        void                                Error(std::string nError);
        std::list<std::string>              mErrors;

        std::list<ProgramChunk>             mChunks;
        std::list<ProgramChunk*>            mOrdered;

        std::list<label_value>              mDefineValues;

        std::list<expression_target>        mDefineOnlyTargets;

        std::list<expression_target>        mExpressionTargets;
        std::list<expression_target>        mIncrementTargets;

        std::list<Instruction>*             mInstructions;
        ProgramChunk*                       mCurChunk;

        bool                                mIgnoreLabelCase; //When true, all labels are converted to uppercase

};

}

#endif // DASM_H
