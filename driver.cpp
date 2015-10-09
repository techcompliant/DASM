// Driver program to assemble code

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>


#include "DAsm.h"

/**
 * Load a file, processing any .include or #include directives.
 */
std::string getFile(std::string filename) {
    // We'll just cat everything into this stream.
    std::stringstream result;
    
    std::cerr << "Loading " << filename << std::endl;
    
    // Open the file
    std::ifstream file(filename);
    
    // TODO: warn about missing files
    
    size_t line_number = 0;
    
    std::string line;
    while(std::getline(file, line)){
        
        line_number++;
        
        size_t offset = 0;
        while(offset < line.size() && ::isspace(line[offset])) {
            // Skip any leading spaces
            offset++;
        }
        // This is the start of the first word.
        size_t word_start = offset;
        
        while(offset < line.size() && !::isspace(line[offset])) {
            // Skip anything that isn't a space
            offset++;
        }
        // This is the end of the first word.
        size_t word_end = offset;
        
        if(word_start < line.size() && word_end < line.size()) {
        
            // We found a first word. Clip it out.
            std::string first_str = line.substr(word_start, word_end - word_start);
            std::string rest_str = line.substr(word_end);
            
            // Capitalize the first part
            std::transform(first_str.begin(), first_str.end(), first_str.begin(), ::toupper);
            
            if(first_str == "INCLUDE" || first_str == ".INCLUDE" || first_str == "#INCLUDE"){
                // Handle an include
                
                // Trim whitespace, quotes, and brackets from the filename
                std::string included_filename = rest_str;
                
                // Match file, "file", and <file>
                size_t start = included_filename.find_first_not_of(" \t'\"<");
                size_t end = included_filename.find_last_not_of(" \t'\">") + 1;
                
                if(start < included_filename.size() && end <= included_filename.size() && start <= end) {
                
                    // Trim out just the filename
                    included_filename = included_filename.substr(start, end - start);
                    
                    // TODO: catch include loops
                    
                    // Recurse
                    result << getFile(included_filename);
                
                    // Don't take this line
                    continue;
                    
                } else {
                    std::cerr << "Preprocessor error: " << filename << ": " << line_number << ": Incorrect include syntax" << std::endl;
                    std::cerr << line << std::endl;
                    exit(1);
                }
                
            }
        }
        
        // If we get here it wasn't an include. Use the line.
        result << line << std::endl;
    }
    
    // Get the string and return it
    return result.str();
}

int main(int argc, char** argv) {

    if(argc < 3) {
        std::cerr << "Usage: " << argv[0] << " input.asm output.bin" << std::endl;
        return 1;
    }

    // Load up the source code.
    std::string source = getFile(argv[1]);

    DAsm::Program lProgram;

    if(lProgram.LoadSource(source)){
        // It compiled.
        unsigned long pSize;
        //The parameters in this call are optional
        DAsm::word* lMemory = lProgram.ToBlock(DAsm::DCPU_RAM_SIZE, pSize);
        
        // Open the output file
        std::ofstream out(argv[2]);
        
        // Dump the program to disk in big endian byte order
        for(int i = 0; i < pSize; i++){
            // Break into bytes
            unsigned char bytes[2];
            bytes[0] = lMemory[i] >> 8;
            bytes[1] = lMemory[i] & 0xFF;
            
            // Write the bytes
            out.write((char*) bytes, 2);
        }
        std::cout << "Wrote "<< pSize << " words to " << argv[2] << std::endl;
        delete lMemory;
    }else{
        for(auto&& error : lProgram.mErrors){
            std::cerr << "Error: " << error << std::endl;
        }
        return 1;
    }
    return 0;    
}
