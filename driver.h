#ifndef DRIVER_H
#define DRIVER_H

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>

#include "DAsm.h"
#include "Program.h"

char parseAssemblerFlags(std::string arg);
int containsFlag(int argc, char** argv, std::string flag);
void displayUsage(char* name);

namespace DAsmDriver{
    struct source_file{
        std::string     path;
        std::string     filename;
        size_t          first_line;
        size_t          last_line;
        source_file*    parent_file;
        source_file*    included_file;
        source_file*    next_file;
    };

    void setAssemblerFlags(DAsm::Program* program, char flags);
    int concat(const char *infile, std::string outfile, bool standard_output = false);
    int assemble(const char *infile, std::string outfile, char dasm_flags = 0, bool little_endian = true, bool hex_output = false, bool standard_output = false); // Takes char* for easy passing of arguments from argv

    bool getFile(std::string filename, std::stringstream &buffer);
    std::string changeOrAddFileExtension(std::string file, std::string new_extension);

    bool isLineInFile(source_file* file, size_t global_line);
    source_file* getLineSourceFile(source_file* origin_file, size_t global_line);
}
#endif
