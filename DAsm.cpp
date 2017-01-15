/*
Copyright (c) 2015 NetCompliant LLC

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <regex>

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
    void    splitString(std::string& source, std::string splitRegex, std::list<std::string>& stringList, std::function<bool(std::string)> predicate){

        std::regex split(splitRegex);
        copy_if(std::sregex_token_iterator(source.begin(), source.end(), split, -1),
                std::sregex_token_iterator(),back_inserter(stringList), predicate);
    }

    //Search/replace by regex
    std::string replaceString(std::string source, std::string searchRegex,
                              std::string replaceStr, bool format){
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

    std::string wordToString(word w, char* prefix){
        std::string out = "0000";
        int i = 3;
        while(w != 0){
            out[i] = w % 0xF;
            if(out[i] < 10)
                out[i] += '0';
            else
                out[i] += 'A' - 10;
            i--;
            w /= 0xF;
        }

        if(prefix != nullptr)
            return prefix + out;
        return out;
    }

}
