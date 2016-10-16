/*
Copyright (c) 2015 NetCompliant LLC

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <functional>
#include <iterator>
#include <iomanip>
#include <sstream>
#include <regex>
#include <algorithm>
#include <iomanip>

#include "DAsm.h"


namespace DAsm{

    std::string trimWS(std::string& s){
        std::string ws = " \t\n\r";
        s.erase(0, s.find_first_not_of(ws));
        s.erase(s.find_last_not_of(ws)+1);
        return s;
    }

    //std::stoi is far too lenient wrt numbers with other stuff at the end
    int     getNumber(std::string str, bool& number){
        std::transform(str.begin(), str.end(), str.begin(), ::toupper);
        int value = 0;
        bool negative = false;
        number = true;
        //Since we're checking for non-digits later we need to handle negatives manually
        if(str.find("-")==0){
            negative = true;
            str=str.substr(1);
        }
        //NB we're assuming we've stripped spaces
        if(str.find("0B")==0){
            for(auto&& c : str.substr(2)){
                if(std::string("01").find(c)==std::string::npos){
                    number=false;
                    return 0;
                }
            }
            value = std::stoi(str.substr(2), nullptr ,2);
            return value;
        }
        if(str.find("0X")==0){
            for(auto&& c : str.substr(2)){
                if(std::string("0123456789ABCDEF").find(c)==std::string::npos){
                    number=false;
                    return 0;
                }
            }
            value = std::stoi(str.substr(2), nullptr ,16);
            return value;
        }
        for(auto&& c : str){
            if(std::string("0123456789").find(c)==std::string::npos){
                number=false;
                return 0;
            }
        }
        value = std::stoi(str, nullptr ,10);
        if(negative)
            return -value;
        else
            return value;
    }

    //Function to split string based on regex and put in list. Default predicate is just non-emptiness
    void    splitString(std::string& source, std::string splitRegex, std::list<std::string>& stringList,
                        std::function<bool(std::string)> predicate = [](std::string str){return (str.size() > 0);}){

        std::regex split(splitRegex);
        copy_if(std::sregex_token_iterator(source.begin(), source.end(), split, -1),
                std::sregex_token_iterator(),back_inserter(stringList), predicate);
    }

    //Search/replace by regex
    std::string replaceString(std::string source, std::string searchRegex,
                              std::string replaceStr, bool format = true){
        if(!format){
            replaceStr = replaceString(replaceStr, "\\$","$$$$", true);
        }
        std::regex searchReg(searchRegex);
        return std::string(std::regex_replace(source, searchReg, replaceStr));
    }

    //Escape a string so it can be matched as a regex
    std::string regexEscape(std::string source){
        source = replaceString(source, "(\\\\|\\+|\\.|\\$|\\*|\\[|\\]|\\^|\\(|\\)|\\^|\\||\\?)","\\$&");
        return source;
    }

    std::list<str_opcode> Instruction::mOpcodes;
    std::list<str_opcode> Instruction::mSpecialOpcodes;
    std::list<str_opcode> Instruction::mReg;

    Program* Instruction::mProgram;

    Instruction::Instruction(){
    }

    Instruction::Instruction(std::string source, unsigned int recursion_counter){
        LoadSource(source, recursion_counter);
    }


    Instruction::~Instruction(){
    }


    void    Instruction::Error(std::string nError){
        mProgram->Error(nError);
    }


    void    Instruction::Fill(word value, unsigned int amount){
        for(unsigned int i=0;i<amount;i++)
            mWords.push_back(value);
    }

    //Returns opcode, or 0x00 if it is a "special" opcode
    opcode  Instruction::GetOpcode(std::string str){
        for(auto&& i : mSpecialOpcodes){
            if(i.str == str)
                return 0x00;
        }
        for(auto&& i : mOpcodes){
            if(i.str == str)
                return i.value;
        }
        Error(std::string("Opcode not found: ").append(str));
        return 0xFF;
    }

    //Returns special opcodes
    opcode  Instruction::GetSpecialOpcode(std::string str){
        for(auto&& i : mSpecialOpcodes){
            if(i.str == str)
                return i.value;
        }
        Error(std::string("Opcode not found: ").append(str));
        return 0xFF;
    }

    //Returns length of instruction in words
    unsigned int    Instruction::GetLength(){
        return mWords.size();
    }

    //Parses the A or B argument of an instruction
    word    Instruction::ParseArg(std::string source, bool isA){
        source = replaceString(source, "-","+-");

        trimWS(source);

        bool dereference = false;

        if(source[0]=='['){

            source.erase(remove(source.begin(), source.end(), '['), source.end());
            source.erase(remove(source.begin(), source.end(), ']'), source.end());

            dereference = true;

        }



        //Split into "parts" based on +, eg "A + label + 1" would be three parts
        std::list<std::string> parts;

        splitString(source, "\\+", parts);

        word ret_word = 0xFFFF;

        //Some flags used to decide which opcode
        bool have_SP = false;
        bool increment = false;
        bool have_register = false;
        bool allow_register = true;

        for(auto&& s : parts)
            trimWS(s);

        for(auto s = parts.begin();s!=parts.end();s++){
            std::list<std::string> space_parts;
            splitString(*s,"(\\s|\\t)+",space_parts);
            std::string first_bit = space_parts.front();
            space_parts.pop_front();
            if(mProgram->IsMacro(first_bit)){
                std::list<std::string> rest;
                std::copy(space_parts.begin(),space_parts.end(), std::back_inserter(rest));

                *s = mProgram->DoMacro(first_bit, rest);
                trimWS(*s);
                if ((*s)[0] == '['){
                    (*s).erase(remove((*s).begin(), (*s).end(), '['), (*s).end());
                    (*s).erase(remove((*s).begin(), (*s).end(), ']'), (*s).end());

                    dereference = true;
                }
                std::list<std::string> plus_parts;
                splitString(*s, "\\+", plus_parts);
                *s = plus_parts.front();
                trimWS(*s);
                auto ins = std::next(s,1);
                plus_parts.pop_front();
                for(auto && p : plus_parts){
                    trimWS(p);
                    parts.insert(ins, p);
                }
            }
        }

        while(parts.size()){
            //Grab part and set substring pointer (so we can get case-sensitive version if needed)
            std::string str_part = parts.front();

            //Convert to upper case, but keep the original around
            std::string str_part_upper = str_part;
            transform(str_part_upper.begin(), str_part_upper.end(), str_part_upper.begin(), ::toupper);


            parts.pop_front();
            //We can only have register per operand
            if(!have_register){
                if(!allow_register){//Eg, SP will disallow any registers, but allows constants
                    Error(std::string("Previous argument disallows register offset."));
                }
                for(auto&& r : mReg){ //Check to see if this part is a register
                    if(r.str == str_part_upper){
                        ret_word = r.value;
                        if(parts.size()||increment){//If there were, or will be, constants
                            if(dereference)
                                ret_word += 0x10;
                            else
                                Error(std::string("Offset only valid in dereference."));
                        }else{
                            if(dereference)
                                ret_word += 0x08;
                        }
                        have_register=true;
                        break;
                    }
                }
                if(have_register)
                    continue;
            }

            bool singular = false;//Singulars will allow neither register nor constant offset

            if(str_part_upper=="SP"){
                have_SP = true;
                if(dereference){
                    if(have_register)//You cannot offset SP with a register
                        Error(std::string("SP does not support register offset;"));
                    if(increment || parts.size())
                        ret_word = 0x1A;
                    else
                        ret_word = 0x19;


                    continue;
                }else{//When used as literal you cannot offset
                    ret_word = 0x1B;
                    singular = true;
                }
            }

            //These are the singulars
            if( (str_part_upper == "PUSH" && !isA) || (str_part_upper == "POP" && isA)){
                singular = true;
                ret_word = 0x18;
            }
            if(str_part_upper == "PEEK"){
                ret_word = 0x19;
                singular = true;
            }
            if(str_part_upper == "PC"){
                singular = true;
                ret_word = 0x1C;
            }
            if(str_part_upper == "EX"){
                singular = true;
                ret_word = 0x1D;
            }

            if(singular){
                if(increment || have_register || parts.size())//If there are other parts then throw error
                    Error(str_part_upper.append(std::string(" does not support offsetting")));
                if(dereference)
                    Error(str_part_upper.append(std::string(" does not support dereferencing")));
                break;
            }

            //See if this is a number
            bool error_flag;
            int value = mProgram->Evaluate(str_part_upper, &error_flag);

            if(!error_flag){


                bool next_word = true;//Do we need to add another word to instruction?
                if(dereference){//Derefences always need another word
                    if(!have_register && !have_SP)
                        ret_word = 0x1E;
                    next_word = true;
                }else{
                    if(isA && -1 <= value && value <= 30 && !increment)
                    {//Only argument A can use the short-form literals
                        ret_word = word(0x21 + value);
                        next_word = false;
                    }
                    else
                        ret_word = 0x1F;
                }
                //If need another word so far, or will in future
                if(next_word || parts.size()){
                    if(!increment)
                        mWords.push_back(word(value));
                    else
                        mProgram->AddIncrementTarget(str_part, &mWords.back());
                }
            }else{//Not a number, assume it's a label

                if(increment)
                    mProgram->AddIncrementTarget(str_part, &mWords.back());
                else{
                    mWords.push_back(word(0x1234));
                    mProgram->AddExpressionTarget(str_part, &(mWords.back()));
                }
                if(!have_register){
                    if(!have_SP){
                        if(dereference)
                            ret_word = 0x1E;
                        else
                            ret_word = 0x1F;
                    }
                }
            }
            increment = true; //Any further constants will need to increment the word

        }
        return ret_word;
    }

    //Load line of source
    void    Instruction::LoadSource(std::string source, unsigned int recursion_counter){

        if(source.size()==0)return;

        if(recursion_counter>MAX_RECURSION){
            Error("Max instruction recursion level exceeded.");
            return;
        }

        std::list<std::string> quote_split_list;
        std::list<std::string> final_split_list;//Un-quoted sections will be further split
        std::list<bool> is_quote_list;//Is current final string a quote?

        splitString(source, "\"", quote_split_list);

        //If line starts with " then it is a quote (currently shouldn't mean anything)
        bool in_quote = (source[0]=='"');

        for(auto part = quote_split_list.begin(); part!=quote_split_list.end(); ++part){
            if(in_quote){
                if(is_quote_list.back() == true){
                    part->insert(part->begin(), '"');
                }
                in_quote = (part->back()=='\\');
                final_split_list.push_back(*part);
                is_quote_list.push_back(true);
                continue;
            }
            bool comment = false;
            std::regex split_semicolon(";");
            std::sregex_token_iterator remove_comment(part->begin(), part->end(), split_semicolon, -1);
            if((*part).length()!=remove_comment->str().length())
                comment = true;
            *part = remove_comment->str();//Grab only the first bit (others will be comment)

            std::list<std::string> comma_split_list;
            splitString(*part, ",", comma_split_list);//Split around commas

            if(comma_split_list.size() == 0){//Line is only comment
                return;
            }

            std::string first_str = *(comma_split_list.begin());

            std::list<std::string> split_first;
            splitString(first_str, "(\\s|\\t)+", split_first);//Split first string around spaces


            if(split_first.size()){

                final_split_list.push_back(split_first.front());
                is_quote_list.push_back(false);

                if(split_first.size()>1){//Join the rest, since only the first bit needs to be on its own
                    std::string second =first_str.substr(first_str.find(split_first.front())+split_first.front().size());


                    final_split_list.push_back(second);
                    is_quote_list.push_back(false);
                }
            }
            comma_split_list.pop_front();


            copy_if(comma_split_list.begin(), comma_split_list.end(),
                    back_inserter(final_split_list),
                    [&](std::string str){
                        if(str.size()){
                            is_quote_list.push_back(false);
                            return true;
                        }
                        return false;
                    });
            in_quote = true;
            if(comment)
                break;
        }

        if(final_split_list.size()==0)return;



        std::string first_str = *final_split_list.begin();
        first_str.erase(remove_if(first_str.begin(), first_str.end(),
                                  [](char x){return std::isspace(x,std::locale());}), first_str.end());

        if(first_str[0]==':'||first_str[first_str.size()-1]==':'){

            std::string label;
            if(first_str[0]==':')
                label = first_str.substr(1);
            else
                label=first_str.substr(0,first_str.size()-1);

            if(label.size() == 0){
                Error(std::string("Empty label: ").append(source));
                return;
            }

            if(label.substr(1).find('.')!=std::string::npos){
                Error(std::string("Illegal label-internal '.': ").append(source));
                return;
            }

            //Local labels (beginning with .) get the last global label name (if any) prepended.
            //You can re-use the same local label.
            //TODO: because of the way labels are fixed up in expressions, you
            //can't refer to other global labels' local labels yet.
            if(label[0] == '.'){
                label = mProgram->mGlobalLabel + label;
            }else{
               mProgram->mGlobalLabel = label;
            }

            mProgram->AddLabelValue(label, word(mProgram->mCurChunk->GetLength()+mProgram->mCurChunk->mTargetPos));

            //If there is aside from the label, we'll recursively parse it
            if(final_split_list.size()>1){
                mProgram->mInstructions->emplace_back(source.substr(first_str.size()+source.find(first_str)));
            }
            return;
        }

        transform(first_str.begin(), first_str.end(), first_str.begin(), ::toupper);

        final_split_list.pop_front();
        is_quote_list.pop_front();
        //Next bit handles DAT
        if(first_str=="DAT"||first_str==".DAT"){
            while(final_split_list.size()){
                std::string cur_str = *final_split_list.begin();
                bool is_quote = *is_quote_list.begin();

                final_split_list.pop_front();
                is_quote_list.pop_front();

                if(is_quote){//Quotes get turned into character values
                    for(auto c = cur_str.begin(); c!= cur_str.end(); ++c){
                        if(*c == '\\'){
                            if(next(c) != cur_str.end()){
                                if(*next(c) == '\\'){
                                    mWords.push_back(word('\\'));
                                    ++c;
                                    continue;
                                }
                            }
                        }else{
                            mWords.push_back(word(*c));
                        }
                    }
                }else{//Otherwise interpret as values/labels
                    bool error;
                    int value = mProgram->Evaluate(cur_str,&error);
                    if(!error){
                        mWords.push_back(word(value));
                        continue;
                    }else{
                        mWords.push_back(word(0x1234));
                        mProgram->AddExpressionTarget(cur_str, &(mWords.back()));
                    }
                }
            }
            return;
        }

        //Handles the org stuff, splitting code into chunks
        if(first_str=="ORG"||first_str==".ORG"){
            if(final_split_list.size()==0){
                Error(std::string("Insufficient operands: ").append(source));
                return;
            }
            std::string cur_str = trimWS(final_split_list.front());

            mProgram->mChunks.emplace_back();
            ProgramChunk* lChunk = &(mProgram->mChunks.back());

            mProgram->mCurChunk = lChunk;
            mProgram->mInstructions=&(lChunk->mInstructions);

            lChunk->mHasTargetPos = true;


            bool errorFlag = false;
            int value = mProgram->Evaluate(cur_str,&errorFlag);

            if(!errorFlag){
                lChunk->mTargetPos = value;
            }else{
                mProgram->AddDefineOnlyTarget(cur_str, &(lChunk->mTargetPos));
            }

            return;
        }
        //Handles defines, which are basically like labels
        if(first_str=="DEF"||first_str==".DEF"||
           first_str=="DEFINE"||first_str==".DEFINE"){

            if(final_split_list.size()==1 && !mProgram->mStrictDefineCommas){
                std::list<std::string> split_space;
                std::string total_str = final_split_list.front();
                splitString(total_str, "(\\s|\\t)+", split_space);
                if(split_space.size()>1){
                    std::string value_str =total_str.substr(total_str.find(split_space.front())+split_space.front().size());
                    final_split_list.pop_front();
                    final_split_list.push_back(split_space.front());
                    final_split_list.push_back(value_str);
                }
            }


            if(final_split_list.size()<2){
                Error(std::string("Insufficient operands: ").append(source));
                return;
            }
            std::string cur_str = trimWS(final_split_list.front());
            final_split_list.pop_front();


            bool eval_error = false;
            int value = mProgram->Evaluate(final_split_list.front(),&eval_error);

            if(eval_error){
                std::string source_upper = source;
                transform(source_upper.begin(), source_upper.end(), source_upper.begin(), ::toupper);
                //Preserve original case
                mProgram->AddMacro(cur_str.append("=").append(source.substr(source.find(final_split_list.front()))));
            }else{
                mProgram->AddDefine(cur_str, value);
            }

            return;
        }

        if(first_str=="FILL"||first_str==".FILL"){
            if(final_split_list.size()<2){
                Error(std::string("Insufficient operands: ").append(source));
                return;
            }

            std::string cur_str = final_split_list.front();
            //Safe to evaluate, since it can't depend on future labels (as their value depends on this)
            word value = mProgram->Evaluate(final_split_list.front());

            final_split_list.pop_front();
            unsigned int amount = mProgram->Evaluate(final_split_list.front());
            Fill(value, amount);
            return;
        }
        //Passes to macro system
        if(first_str=="FLAG"||first_str==".FLAG"){
            std::string flag_str = trimWS(final_split_list.front());
            final_split_list.pop_front();
            if(final_split_list.size()==0){
                Error(std::string("Insufficient operands: ").append(source));
                return;
            }
            bool value = (mProgram->Evaluate(final_split_list.front()) != 0);
            transform(flag_str.begin(), flag_str.end(), flag_str.begin(), ::toupper);

            if(flag_str == "IGNORELABELCASE"){
                mProgram->mIgnoreLabelCase = value;
                return;
            }
            if(flag_str == "ARRANGECHUNKS"){
                mProgram->mArrangeChunks = value;
                return;
            }
            if(flag_str == "STRICTDEFINECOMMAS"){
                mProgram->mStrictDefineCommas = value;
                return;
            }


            mProgram->Error(std::string("Unknown flag:").append(flag_str));
            return;
        }

        //Passes to macro system
        if(first_str=="MACRO"||first_str==".MACRO"){
            std::string source_upper = source;
            transform(source_upper.begin(), source_upper.end(), source_upper.begin(), ::toupper);
            //Preserve original case
            mProgram->AddMacro(source.substr(first_str.size()+source_upper.find(first_str)));
            return;
        }

        if(first_str=="OPCODE"||first_str==".OPCODE"){
            if(final_split_list.size()<3){
                mProgram->Error(std::string("Insufficient arguments:").append(source));
                return;
            }

            std::string op_str = trimWS(final_split_list.front()); final_split_list.pop_front();
            std::string val_str = final_split_list.front(); final_split_list.pop_front();
            std::string type_str = trimWS(final_split_list.front()); final_split_list.pop_front();
            str_opcode new_opcode = str_opcode(op_str, mProgram->Evaluate(val_str));

            if(type_str == "AB"){
                mOpcodes.push_back(new_opcode);
            }else if(type_str == "A"){
                mSpecialOpcodes.push_back(new_opcode);
            }else{
                mProgram->Error(std::string("Invalid opcode type:").append(source));
            }

            return;
        }

        if(mProgram->IsMacro(first_str)){
            auto q = is_quote_list.begin();
            //Re-insert the quotes
            for(auto i = final_split_list.begin();i!=final_split_list.end();i++){
                if(*q){
                    *i = std::string("\"").append(*i).append("\"");
                }
                q++;
            }
            std::string afterMacro = mProgram->DoMacro(first_str,final_split_list);

            //Split incase macro generates multiple lines
            std::list<std::string> lines;
            splitString(afterMacro,"\\%n",lines);
            for(auto&& l : lines)
                mProgram->mInstructions->emplace_back(l, recursion_counter+1);
            return;
        }

        word lWord = 0x0000; //Our main word
        mWords.push_back(lWord);

        opcode lOp = GetOpcode(first_str);
        if(lOp == 0xFF)return;
        if(lOp==0x00){
            lOp = GetSpecialOpcode(first_str);
            lWord |= lOp << 5;
            if(final_split_list.size()<1){
                Error(first_str.append(std::string(" opcode requires an operand")));
                return;
            }
            lWord |= ParseArg(*final_split_list.begin(), true) << 10;
        }else{
            if(final_split_list.size()<2){
                Error(first_str.append(std::string(" requires two operands")));
                return;
            }

            lWord |= lOp;
            lWord |= ParseArg(*next(final_split_list.begin()), true) << 10;
            lWord |= ParseArg(*final_split_list.begin()) << 5;

        }


        mWords.front() = lWord;

    }


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

        mChunks.emplace_back();
        mInstructions = &(mChunks.back().mInstructions);
        mCurChunk = &(mChunks.back());

        //Always start assembling at 0 unless otherwise specified.
        mCurChunk->mHasTargetPos = true;
        mCurChunk->mTargetPos = 0;

        //Some default macros
        AddMacro("RET=SET PC, POP");
        AddMacro(".asciiz = dat %0, 0");
        AddMacro(".reserve = fill 0, %0");
        AddMacro("PICK =[SP + %0]");
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
        for(auto&& m : mMacros){
            if(m.keyword == keyword){

                return true;
            }
        }

        return false;
    }

    std::string    Program::DoMacro(std::string keyword, std::list<std::string> args){
        std::string out;
        for(auto&& m : mMacros){
            if(m.keyword == keyword){
                out = m.format;
            }
        }

        unsigned int i = 0;
        for(auto&& a : args){
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
            out = replaceString(out, std::string("%").append(argNumStr), a,false);
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
