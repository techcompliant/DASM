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
#include "ProgramChunk.h"
#include "Instruction.h"


char parseAssemblerFlags(std::string arg, char current_flags = 0);
int containsFlag(int argc, char** argv, std::string flag, unsigned int starting_arg_index = 1);
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

    std::string getColorTag(int color);
    void setAssemblerFlags(DAsm::Program* program, char flags);
    int concat(const char *infile, std::string outfile, bool standard_output = false, bool file_tracability = false, bool full_tracability = false);
    std::string concatWithFileDisplay(std::string concatSources, source_file* origin_fil, bool relative_path = false);

    int assembleToHex(const char *infile, std::string outfile, char dasm_flags = 0, bool little_endian = true, bool standard_output = false, bool file_tracability = false, bool full_tracability = false);

    int assembleToBinary(const char *infile, std::string outfile, char dasm_flags = 0, bool little_endian = true, bool standard_output = false);

    source_file* assemble(const char *infile, DAsm::word* output, unsigned long& pSize, char dasm_flags = 0, bool little_endian = true); // Takes char* for easy passing of arguments from argv

    std::string tidyPath(std::string path);
    source_file* getFile(std::string filename, size_t line_offset, std::stringstream &buffer, source_file* parent_file = nullptr);
    std::string changeOrAddFileExtension(std::string file, std::string new_extension);

    bool isLineInFile(source_file* file, size_t global_line);
    unsigned int getLocalLineNumber(source_file* file,  unsigned int global_line);
    void displayErrors(DAsm::Program program, std::vector<std::string> concat, source_file* origin_file);

    bool checkIncludeRecursion(source_file* file, unsigned int recursion_counter = 0);

    source_file* getIncludeTreeRoot(source_file* file);
    bool isInsideIncludeTree(source_file* origin_file, source_file* new_file);
}
#endif
