#include "Instruction.h"
#include "Program.h"
#include "ProgramChunk.h"

namespace DAsm{
    std::list<str_opcode> Instruction::mOpcodes;
    std::list<str_opcode> Instruction::mSpecialOpcodes;
    std::list<str_opcode> Instruction::mReg;

    Program* Instruction::mProgram;

    Instruction::Instruction(){
    }

    Instruction::Instruction(std::string source, unsigned int line_number, unsigned int recursion_counter){
        mLineNumber = line_number;
        mSourceLine = source;
        LoadSource(source, recursion_counter);
    }


    Instruction::~Instruction(){
    }


    void    Instruction::Error(std::string nError){
        mProgram->Error(nError, mLineNumber);
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
        if(first_str==".DAT"|| (first_str=="DAT" && !mProgram->mStrictDirectiveDots)){
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
        if(first_str==".ORG"|| (first_str=="ORG" && !mProgram->mStrictDirectiveDots)){
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
        if(first_str==".DEF"|| (first_str=="DEF" && !mProgram->mStrictDirectiveDots)||
           first_str==".DEFINE"|| (first_str=="DEFINE" && !mProgram->mStrictDirectiveDots)){

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

        if(first_str==".FILL"|| (first_str=="FILL" && !mProgram->mStrictDirectiveDots)){
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
        if(first_str==".FLAG"|| (first_str=="FLAG" && !mProgram->mStrictDirectiveDots)){
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
            if(flag_str == "STRICTDIRECTIVEDOTS"){
                mProgram->mStrictDirectiveDots = value;
                return;
            }


            mProgram->Error(std::string("Unknown flag:").append(flag_str));
            return;
        }

        //Passes to macro system
        if(first_str==".MACRO"|| (first_str=="MACRO" && !mProgram->mStrictDirectiveDots)){
            std::string source_upper = source;
            transform(source_upper.begin(), source_upper.end(), source_upper.begin(), ::toupper);
            //Preserve original case
            mProgram->AddMacro(source.substr(first_str.size()+source_upper.find(first_str)));
            return;
        }

        if(first_str==".OPCODE"|| (first_str=="OPCODE" && !mProgram->mStrictDirectiveDots)){
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
}
