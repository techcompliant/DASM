#ifndef PROGRAM_H
#define PROGRAM_H

#include <iomanip>

#include "DAsm.h"

namespace DAsm{

    class Instruction;
    class ProgramChunk;

    class   Program{
        public:
                                                Program();
            virtual                             ~Program();

            bool                                LoadSource(std::string source);

            void                                AddExpressionTarget(std::string nExpression,word* nTarget);

            void                                AddDefineOnlyTarget(std::string nExpression,word* nTarget);

            void                                AddIncrementTarget(std::string nExpression,word* nTarget);

            void                                AddLabelValue(std::string nLabel, word nValue);
            void                                AddDefine(std::string nLabel, int nValue);
            std::string                         GlobalizeLabels(std::string nExpression);

            unsigned int                        GetLength();
            unsigned int                        mLength;

            std::string                         ToHex(std::string seperator = " 0x");

            int                                 Evaluate(std::string expression, bool* errorFlag=nullptr);

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
