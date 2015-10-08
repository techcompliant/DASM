/*
Copyright (c) 2015 NetCompliant LLC

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

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
    bool arguments;
    str_opcode(std::string nStr, opcode nValue, bool nArguments = true){str=nStr; value=nValue; arguments=nArguments;}
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
        bool                                GetArgumentFlag(std::string str);

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
        bool                                mArrangeChunks;//When true, .orgs control actual position of code in output
};

}

#endif // DASM_H
