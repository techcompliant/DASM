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

    int assemble(const char *infile, const char *outfile); // Takes char* for easy passing of arguments from argv
    bool getFile(std::string filename, std::stringstream &buffer);
    std::string changeOrAddFileExtension(std::string file, std::string new_extension);
    
}
#endif
