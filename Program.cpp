#include "Program.h"
#include "ProgramChunk.h"
#include "Instruction.h"

namespace DAsm{
    //Setup opcodes etc
    Program::Program(){
        Instruction::mOpcodes.push_back(str_opcode("SET", 0x01));
        Instruction::mOpcodes.push_back(str_opcode("ADD", 0x02));
        Instruction::mOpcodes.push_back(str_opcode("SUB", 0x03));
        Instruction::mOpcodes.push_back(str_opcode("MUL", 0x04));
        Instruction::mOpcodes.push_back(str_opcode("MLI", 0x05));
        Instruction::mOpcodes.push_back(str_opcode("DIV", 0x06));
        Instruction::mOpcodes.push_back(str_opcode("DVI", 0x07));
        Instruction::mOpcodes.push_back(str_opcode("MOD", 0x08));
        Instruction::mOpcodes.push_back(str_opcode("MDI", 0x09));
        Instruction::mOpcodes.push_back(str_opcode("AND", 0x0A));
        Instruction::mOpcodes.push_back(str_opcode("BOR", 0x0B));
        Instruction::mOpcodes.push_back(str_opcode("XOR", 0x0C));
        Instruction::mOpcodes.push_back(str_opcode("SHR", 0x0D));
        Instruction::mOpcodes.push_back(str_opcode("ASR", 0x0E));
        Instruction::mOpcodes.push_back(str_opcode("SHL", 0x0F));
        Instruction::mOpcodes.push_back(str_opcode("IFB", 0x10));
        Instruction::mOpcodes.push_back(str_opcode("IFC", 0x11));
        Instruction::mOpcodes.push_back(str_opcode("IFE", 0x12));
        Instruction::mOpcodes.push_back(str_opcode("IFN", 0x13));
        Instruction::mOpcodes.push_back(str_opcode("IFG", 0x14));
        Instruction::mOpcodes.push_back(str_opcode("IFA", 0x15));
        Instruction::mOpcodes.push_back(str_opcode("IFL", 0x16));
        Instruction::mOpcodes.push_back(str_opcode("IFU", 0x17));
        Instruction::mOpcodes.push_back(str_opcode("ADX", 0x1A));
        Instruction::mOpcodes.push_back(str_opcode("SBX", 0x1B));
        Instruction::mOpcodes.push_back(str_opcode("STI", 0x1E));
        Instruction::mOpcodes.push_back(str_opcode("STD", 0x1F));


        Instruction::mSpecialOpcodes.push_back(str_opcode("JSR", 0x01));
        Instruction::mSpecialOpcodes.push_back(str_opcode("INT", 0x08));
        Instruction::mSpecialOpcodes.push_back(str_opcode("IAG", 0x09));
        Instruction::mSpecialOpcodes.push_back(str_opcode("IAS", 0x0A));
        Instruction::mSpecialOpcodes.push_back(str_opcode("RFI", 0x0B));
        Instruction::mSpecialOpcodes.push_back(str_opcode("IAQ", 0x0C));
        Instruction::mSpecialOpcodes.push_back(str_opcode("HWN", 0x10));
        Instruction::mSpecialOpcodes.push_back(str_opcode("HWQ", 0x11));
        Instruction::mSpecialOpcodes.push_back(str_opcode("HWI", 0x12));
        Instruction::mSpecialOpcodes.push_back(str_opcode("LOG", 0x13));
        Instruction::mSpecialOpcodes.push_back(str_opcode("BRK", 0x14));
        Instruction::mSpecialOpcodes.push_back(str_opcode("HLT", 0x15));


        Instruction::mReg.push_back(str_opcode("A", 0x00));
        Instruction::mReg.push_back(str_opcode("B", 0x01));
        Instruction::mReg.push_back(str_opcode("C", 0x02));
        Instruction::mReg.push_back(str_opcode("X", 0x03));
        Instruction::mReg.push_back(str_opcode("Y", 0x04));
        Instruction::mReg.push_back(str_opcode("Z", 0x05));
        Instruction::mReg.push_back(str_opcode("I", 0x06));
        Instruction::mReg.push_back(str_opcode("J", 0x07));

        Instruction::mProgram = this;

        mIgnoreLabelCase = true;
        mArrangeChunks = false;
        mStrictDefineCommas = false;
        mStrictDirectiveDots = false;

        mChunks.emplace_back();
        mInstructions = &(mChunks.back().mInstructions);
        mCurChunk = &(mChunks.back());

        //Always start assembling at 0 unless otherwise specified.
        mCurChunk->mHasTargetPos = true;
        mCurChunk->mTargetPos = 0;

        //Some default macros
        AddMacro("RET=SET PC, POP");
        AddMacro(".asciiz = .dat %0, 0");
        AddMacro(".reserve = .fill 0, %0");
    }




    Program::~Program(){
    }

    void    Program::Error(std::string nError){
        mErrors.push_back(nError);
    }


    //Add word to be replaced with expression later
    void    Program::AddExpressionTarget(std::string nExpression,word* nTarget){
        if(nExpression.size())
            if(mIgnoreLabelCase && nExpression[0]!='\'')
                transform(nExpression.begin(), nExpression.end(), nExpression.begin(), ::toupper);
        mExpressionTargets.push_back(expression_target(GlobalizeLabels(nExpression), nTarget));
    }

    //Add word to be replaced with define later
    void    Program::AddDefineOnlyTarget(std::string nExpression,word* nTarget){
        if(nExpression.size())
            if(mIgnoreLabelCase && nExpression[0]!='\'')
                transform(nExpression.begin(), nExpression.end(), nExpression.begin(), ::toupper);
        mDefineOnlyTargets.push_back(expression_target(GlobalizeLabels(nExpression), nTarget));
    }


    //Add value of label
    void    Program::AddLabelValue(std::string nLabel,word nValue){
        trimWS(nLabel);
        if(mIgnoreLabelCase)
            transform(nLabel.begin(), nLabel.end(), nLabel.begin(), ::toupper);
        for(auto&& l : mChunks.back().mLabelValues){
            if(l.label == nLabel){
                l.value = nValue;
                return;
            }
        }
        mChunks.back().mLabelValues.push_back(label_value(nLabel, nValue));
    }

    //Define label value
    void    Program::AddDefine(std::string nLabel,int nValue){
        trimWS(nLabel);
        if(mIgnoreLabelCase)
            transform(nLabel.begin(), nLabel.end(), nLabel.begin(), ::toupper);
        for(auto&& d : mDefineValues){
            if(d.label == nLabel){
                d.value = nValue;
                return;
            }
        }
        mDefineValues.push_back(label_value(nLabel, nValue));
    }


    //Rewrite any local labels (beginning with .) in an expression to global ones
    //Uses the currently set global label, if any
    std::string Program::GlobalizeLabels(std::string nExpression){
        std::string globalizer = mGlobalLabel + ".";

       if(mIgnoreLabelCase)
            transform(globalizer.begin(), globalizer.end(), globalizer.begin(), ::toupper);

       //No .s should appear in expressions except at the start of labels.
        //TODO: maybe associate a scope with expressions instead of just having
        //them be strings so this can be more robust.
        return replaceString(nExpression, "\\.", globalizer);
    }


    void    Program::AddMacro(std::string nMacro){
        macro lMacro;
        //Split around the first "="
        std::list<std::string> inOutParts;
        splitString(nMacro,"=",inOutParts);



        std::string keyword = inOutParts.front();

        // Stripping the keyword from a possible dot
        if(keyword[0] == '.')
            keyword = keyword.substr(1, keyword.size());

        keyword.erase(remove_if(keyword.begin(), keyword.end(),
                                [](char x){return std::isspace(x,std::locale());}), keyword.end());
        transform(keyword.begin(), keyword.end(), keyword.begin(), ::toupper);

        inOutParts.pop_front();

        std::string outStr;
        bool first = true;
        for(auto&& p : inOutParts){
            if(!first)
                outStr.append(std::string("="));
            outStr = outStr.append(p);
            first=false;
        }


        for(auto&& m: mMacros){
            if(m.keyword == keyword){
                m.format = outStr;
                return;
            }
        }

        lMacro.keyword = keyword;
        lMacro.format = outStr;
        mMacros.push_back(lMacro);
    }

    //Returns true if is a macro
    bool    Program::IsMacro(std::string keyword){
        if(mStrictDirectiveDots && keyword[0] != '.')
            return false; // Can't be a macro if it's not dotted

        std::string undottedKeyword = keyword[0] == '.' ? keyword.substr(1, keyword.size()) : keyword;

        for(auto&& m : mMacros){
            if(m.keyword == undottedKeyword){

                return true;
            }
        }

        return false;
    }

    std::string    Program::DoMacro(std::string keyword, std::list<std::string> args){
        std::string out;
        std::string undottedKeyword = keyword[0] == '.' ? keyword.substr(1, keyword.size()) : keyword;


        for(auto&& m : mMacros){
            if(m.keyword == keyword){
                out = m.format;
            }
        }

        unsigned int i = 0;
        for(auto&& a : args){
            a = regexEscape(a);
            trimWS(a);
            std::string argNumStr = std::to_string(i++);
            if(out.find(std::string("%e").append(argNumStr))!= std::string::npos){
                bool error;
                std::string evalledStr = std::to_string(Evaluate(a, &error));
                if(!error){
                    out = replaceString(out, std::string("%e").append(argNumStr),evalledStr);
                    continue;
                }
            }
            out = replaceString(out, std::string("%").append(argNumStr), a);
        }
        return out;
    }


    //Add word to be incremented by expression
    void    Program::AddIncrementTarget(std::string nExpression, word* nTarget){
        if(nExpression.size())
            if(mIgnoreLabelCase && nExpression[0]!='\'')
                transform(nExpression.begin(), nExpression.end(), nExpression.begin(), ::toupper);
        mIncrementTargets.push_back(expression_target(GlobalizeLabels(nExpression), nTarget));
    }
    //Returns length of program so far, in words
    unsigned int    Program::GetLength(){
        unsigned int result = 0;
        for(auto && c : mChunks)
            result += c.GetLength();
        return result;
    }


    //Evaluates an expression
    int    Program::Evaluate(std::string expression, bool* errorFlag){
        if(expression.size()==0)
            return 0;

        trimWS(expression);

        //Check for character literals

        if(expression.size()==3)
            if(expression[0]=='\''&&expression[2]=='\'')
                return expression[1];

        //Basically we just go through each operator in order
        int result = 0;
        std::list<std::string> plusParts;
        splitString(expression,"\\+",plusParts,[](std::string str){return true;});

        if(plusParts.size()>1){
            for(auto&& p: plusParts){
                result += Evaluate(p, errorFlag);
            }
            return result;
        }

        std::list<std::string> minusParts;
        splitString(expression,"-",minusParts,[](std::string str){return true;});

        if(minusParts.size()>1){
            result = Evaluate(minusParts.front(), errorFlag);
            minusParts.pop_front();
            for(auto&& p: minusParts){
                result -= Evaluate(p, errorFlag);
            }
            return result;
        }


        std::list<std::string> leftShiftParts;
        splitString(expression,"<<",leftShiftParts,[](std::string str){return true;});

        if(leftShiftParts.size()>1){
            result = Evaluate(leftShiftParts.front(), errorFlag);
            leftShiftParts.pop_front();
            for(auto&& p: leftShiftParts){
                result = result << Evaluate(p, errorFlag);
            }
            return result;
        }

        std::list<std::string> rightShiftParts;
        splitString(expression,">>",rightShiftParts,[](std::string str){return true;});

       if(rightShiftParts.size()>1){
            result = Evaluate(rightShiftParts.front());
            rightShiftParts.pop_front();
            for(auto&& p: rightShiftParts){
                result = result >> Evaluate(p, errorFlag);
            }
            return result;
        }

        std::list<std::string> multiplyParts;
        splitString(expression,"\\*",multiplyParts);

        if(multiplyParts.size()>1){
            result=1;
            for(auto&& p: multiplyParts){
                result *= Evaluate(p, errorFlag);
            }
            return result;
        }

        std::list<std::string> divideParts;
        splitString(expression,"/",divideParts);

        if(divideParts.size()>1){
            result = Evaluate(divideParts.front());
            divideParts.pop_front();
            for(auto&& p: divideParts){
                int divide_value = Evaluate(p, errorFlag);
                if(divide_value != 0)
                    result /= divide_value;
            }
            return result;
        }

        std::list<std::string> andParts;
        splitString(expression,"&",andParts);

        if(andParts.size()>1){
            result = Evaluate(andParts.front());
            andParts.pop_front();
            for(auto&& p: andParts){
                result = result & Evaluate(p, errorFlag);
            }
            return result;
        }

        std::list<std::string> orParts;
        splitString(expression,"\\|",orParts);
        if(orParts.size()>1){
            result = Evaluate(orParts.front());
            orParts.pop_front();
            for(auto&& p: orParts){
                result = result | Evaluate(p, errorFlag);
            }
            return result;
        }

        bool number;

        result = getNumber(expression, number);

        if(number)
            return result;


        //Not a number,, see if it's a label/define


        //If ignoring label case, everything is uppercase
        if(mIgnoreLabelCase){
            std::transform(expression.begin(), expression.end(), expression.begin(), ::toupper);
        }

        for(auto&& v : mDefineValues)
            if(expression == v.label)
                return v.value;

        for(auto&& c : mChunks)
            for(auto&& v: c.mLabelValues)
                if(expression == v.label) {
                    return v.value;
        }
        if(errorFlag == nullptr)
            Error(std::string("Could not evaluate expression:").append(expression));
        else
            *errorFlag = true;
        return 0;


    }

    //Load and assemble program from source
    bool    Program::LoadSource(std::string source){
        //Split into lines
        std::list<std::string> lines;
        splitString(source, "\n+", lines);


        //Where actual parsing happens
        for(auto&& l : lines){
            mInstructions->emplace_back(l);
        }

        for(auto&& t : mDefineOnlyTargets){
            *t.target = Evaluate(t.expression);
        }

        if(mArrangeChunks){
            //Order chunks based on target pos
            std::list<ProgramChunk*> unOrdered;

            for(auto&& c : mChunks){

                if(c.mHasTargetPos){
                    bool found = false;
                    for(auto i = mOrdered.begin(); i!= mOrdered.end()&&!found;i++){
                        if(c.mTargetPos<(*i)->mTargetPos){
                            mOrdered.insert(i, &c);
                            found=true;
                        }
                    }
                    if(!found)
                        mOrdered.push_back(&c);
                }else{
                    unOrdered.push_back(&c);
                }
            }
            //Add any unordered chunks onto end
            mOrdered.splice(mOrdered.end(),unOrdered);
            //If the first chunk isn't at 0, add a chunk there
            if(mOrdered.front()->mTargetPos!=0){
                mChunks.emplace_back();
                ProgramChunk*   lChunk = &(mChunks.back());
                lChunk->mTargetPos=0;
                lChunk->mHasTargetPos=true;
                mOrdered.insert(mOrdered.begin(),lChunk);
            }

            //Pad spaces between chunks with "0"
            for(auto i = mOrdered.begin(); std::next(i)!= mOrdered.end();i++){
                auto n = std::next(i);

                if(!(*n)->mHasTargetPos){
                    (*n)->mTargetPos = (*i)->mTargetPos + (*i)->GetLength();
                    for(auto && l : (*n)->mLabelValues)
                        l.value += (*n)->mTargetPos;

                }else{
                    int padding = (*n)->mTargetPos - ((*i)->mTargetPos + (*i)->GetLength());

                    if(padding<0){
                        Error(std::string("Invalid chunk layout:").append(std::to_string((*i)->mTargetPos)));
                    }else{
                        (*i)->mInstructions.emplace_back();
                        (*i)->mInstructions.back().Fill(0,padding);

                    }
                }
            }





        }else{
            //Chunks with target positions get assembled as if they are there.
            //Chunks with no target positions get assigned their actual positions.
            //This is useful if your code moves itself around.
            int start = 0;
            for(auto&& c : mChunks){
                    if(!c.mHasTargetPos){
                    //Tell it it is where it actually is.
                    c.mTargetPos = start;
                    c.mHasTargetPos = true;
                    for(auto && l : c.mLabelValues)
                        l.value += c.mTargetPos;

                }
                mOrdered.push_back(&c);
                start += c.GetLength();
            }
        }





        //Replace all the expression words with values
        for(auto&& t : mExpressionTargets){
            *t.target = Evaluate(t.expression);
        }

        //Increment necessary words by expression value
        for(auto&& t : mIncrementTargets){
            *t.target += Evaluate(t.expression);
        }
        mIncrementTargets.clear();
        mExpressionTargets.clear();
        mDefineValues.clear();

        return mErrors.size() == 0;

    }

    //Outputs in hex format for easy debugging
    std::string  Program::ToHex(std::string seperator){
        std::stringstream result;
        for(auto&& o : mOrdered)
            for(auto&& i : o->mInstructions)
                for(auto&& w : i.mWords)
                    result << seperator << std::setfill('0') << std::setw(4) << std::hex << int(w);

        return result.str();
    }

    //Outputs a pojnter to a block of memory containing program, optional max size
    // and program size return, note, Program object will not free this memory upon deletion
    word*   Program::ToBlock(unsigned long maxSize){
        unsigned long actual_size;
        return ToBlock(maxSize, actual_size);
    }
    word*   Program::ToBlock(unsigned long maxSize, unsigned long& programSize){
        if(GetLength() > maxSize)
            Error(std::string("Program size exceeds DCPU memory!"));
        word* memory = new word[maxSize]();

        unsigned long cur_pos = 0;
        for(auto&& o : mOrdered)
            for(auto&& i : o->mInstructions)
                for(auto&& w : i.mWords)
                    memory[cur_pos++] = w;

        programSize = GetLength();
        return memory;
    }
}
