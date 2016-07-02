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
#include <functional>

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
    //Stores int-width values for extra-long defines
    struct label_value{
        std::string label;
        int    value;
        label_value(std::string nLabel, int nValue){label=nLabel; value=nValue;}
    };

    struct macro{
        std::string keyword;
        std::string format;
    };

    std::string trimWS(std::string& s);

    //std::stoi is far too lenient wrt numbers with other stuff at the end
    int         getNumber(std::string str, bool& number);

    //Function to split string based on regex and put in list. Default predicate is just non-emptiness
    void        splitString(std::string& source, std::string splitRegex, std::list<std::string>& stringList,
                        std::function<bool(std::string)> predicate = [](std::string str){return (str.size() > 0);});

    //Search/replace by regex
    std::string replaceString(std::string source, std::string searchRegex, std::string replaceStr);

    //Escape a string so it can be matched as a regex
    std::string regexEscape(std::string source);
}

#endif // DASM_H
