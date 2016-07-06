#ifndef PROGRAM_H
#define PROGRAM_H

#include <iomanip>

#include "DAsm.h"

namespace DAsm{

    class Instruction;
    class ProgramChunk;

    struct error_entry{
        std::string     message;
        unsigned int    line;
        Instruction*    source;
    };

    class   Program{
        public:
                                                Program();
            virtual                             ~Program();

            bool                                LoadSource(std::string source, bool ingnoreEmptyLines = true);

            void                                AddExpressionTarget(std::string nExpression,word* nTarget);

            void                                AddDefineOnlyTarget(std::string nExpression,word* nTarget);

            void                                AddIncrementTarget(std::string nExpression,word* nTarget);

            void                                AddLabelValue(std::string nLabel, word nValue);
            void                                AddDefine(std::string nLabel, int nValue);
            std::string                         GlobalizeLabels(std::string nExpression);

            unsigned int                        GetLength();
            unsigned int                        mLength;

            int                                 Evaluate(std::string expression, bool* errorFlag=nullptr);

            bool                                IsMacro(std::string keyword);
            std::string                         DoMacro(std::string keyword, std::list<std::string> args);
            void                                AddMacro(std::string nMacro);

            std::list<macro>                    mMacros;

            std::string                         ToHex(std::string firstWordSeparator = "0x", std::string wordSeparator = " 0x", std::string instructionSeparator = "\n", bool littleEndian = true, bool ignoreEmptyIntructions = true);

            word*                               ToBlock(unsigned long maxSize = DCPU_RAM_SIZE, bool little_endian = true);
            word*                               ToBlock(unsigned long maxSize, unsigned long& programSize, bool little_endian = true);

            void                                Error(std::string message, Instruction* source = nullptr);
            void                                Error(std::string message, unsigned int line_number);
            void                                updateError(error_entry &entry);
            std::list<error_entry>              mErrors;

            std::list<ProgramChunk>             mChunks;
            std::list<ProgramChunk*>            mOrdered;

            std::string                         mGlobalLabel;

            std::list<label_value>              mDefineValues;

            std::list<expression_target>        mDefineOnlyTargets;

            std::list<expression_target>        mExpressionTargets;
            std::list<expression_target>        mIncrementTargets;

            std::list<Instruction>*             mInstructions;
            ProgramChunk*                       mCurChunk;

            bool                                mStrictDefineCommas;
            bool                                mStrictDirectiveDots;
            bool                                mIgnoreLabelCase; //When true, all labels are converted to uppercase
            bool                                mArrangeChunks;//When true, .orgs control actual position of code in output
    };
}
#endif
