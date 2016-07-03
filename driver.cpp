// Driver program to assemble code

#include "driver.h"

int main(int argc, char** argv) {

    const char *infile;
    std::string outfile;

    bool big_endian, standard_output = false;
    char output_type = 0; // 0:normal, 1:hex, 2:concat included
    char dasm_flags = 0;

    int i;

    if(argc < 2 || containsFlag(argc, argv, "--help")) {
        std::cerr << std::endl << "Please enter a input_file name." << std::endl;
        displayUsage(argv[0]);
        return 1;
    }
    big_endian = containsFlag(argc, argv, "--big-endian") != 0 || containsFlag(argc, argv, "-b") != 0;
    standard_output = containsFlag(argc, argv, "--standard-output") != 0 || containsFlag(argc, argv, "-s") != 0;
    output_type = (containsFlag(argc, argv, "--hex-output") != 0 || containsFlag(argc, argv, "-H") != 0) ? 1 : 0;
    if(containsFlag(argc, argv, "--concat-include") || containsFlag(argc, argv, "-C") != 0){
        if(output_type){
            std::cerr << std::endl << "Hex-output mode and concat-include mode are mutually exclusives." << std::endl;
            displayUsage(argv[0]);
            return 1;
        }
        output_type = 2;
    }
    if((i = containsFlag(argc, argv, "--assembler-flags")) || (i = containsFlag(argc, argv, "-f"))){
        dasm_flags = parseAssemblerFlags(argv[i]);
    }

    if((i = containsFlag(argc, argv, "-o")) > 1) {
        i++;
        if(i >= argc){
            std::cerr << std::endl << "Please enter a output_file name after the flag \"-o\"." << std::endl;
            displayUsage(argv[0]);
            return 1;
        }
        infile = argv[1];
        outfile = argv[i];
    } else {
        infile = argv[1];
        outfile = DAsmDriver::changeOrAddFileExtension(argv[1], !output_type ? "bin" : output_type == 1 ? "hex" : "cat.dasm").c_str();

        outfile = outfile.substr(outfile.find_last_of("\\") + 1, outfile.size()); // Removing path
        outfile = outfile.substr(outfile.find_last_of("/") + 1, outfile.size());

        std::clog << "Outputing to " << outfile << std::endl;
    }

    if(output_type == 2)
        return DAsmDriver::concat(infile, outfile, standard_output);

    return DAsmDriver::assemble(infile, outfile, dasm_flags, !big_endian, output_type == 1, standard_output);
}

char parseAssemblerFlags(std::string arg){
    char output = 0;

    arg = arg.substr(arg.find_last_of("=") + 1, arg.size());

    std::list<std::string> flags;
    DAsm::splitString(arg, "[^a-zA-Z]+", flags);
    for(auto flag : flags){
        std::cerr << "Flag: \"" << flag << "\"" << std::endl;
        if(flag == "IGNORELABELCASE"){
            output |= 0x01;
            continue;
        }
        if(flag == "ARRANGECHUNKS"){
            output |= 0x02;
            continue;
        }
        if(flag == "STRICTDEFINECOMMAS"){
            output |= 0x04;
            continue;
        }
        if(flag == "STRICTDIRECTIVEDOTS"){
            output |= 0x08;
            continue;
        }
    }
    return output;
}

int containsFlag(int argc, char** argv, std::string flag){
    std::string t;
    bool plain_text_flag, plain_text_arg;
    plain_text_flag = flag[0] == '-' && flag[1] == '-';

    for(int i = 1; i < argc; i++){
        t = argv[i];
        plain_text_arg = argv[i][0] == '-' && argv[i][1] == '-';
        if(plain_text_arg != plain_text_flag)
            continue;

        if(!plain_text_arg){ // If the flag is short then check each char until a '=' '
            for(int j = 1; t[j] && t[j] != '='; j++){
                if(flag[1] == t[j])
                    return i;
            }
        }else{
            for(int j = 0; flag[j]; j++){
                if(flag[j] == t[j] && !flag[j + 1])
                    return i;
            }
        }
    }
    return 0;
}

void displayUsage(char* name){
    std::string exec = name;
    exec = exec.substr(exec.find_last_of("\\") + 1, exec.size());
    exec = exec.substr(exec.find_last_of("/") + 1, exec.size());
    std::clog << std::endl << "\tDAsm" << std::endl;
    std::clog << "Usage:" << std::endl;
    std::clog << "  " << exec << " <input_file> [-bsHC] [-o output_file][--big-endian][--hex-output/concat-include][[--assembler-flags||-f]=<flag1>[,-+.]<flag2>...]" << std::endl;
    std::clog << "Options:" << std::endl;
    std::clog << "  --big-endian  -b\tOutput in big-endian (inoperant in concat-include mode)"<< std::endl;
    std::clog << "  --standard-output  -s\tCopy the output in stdout (inoperant in normal mode)"<< std::endl;
    std::clog << "  --assembler-flags  -f\tInvert default state of DAsm flags (inoperant in concat-include mode)"<< std::endl;
    std::clog << "  --hex-output  -H\tOutput the binary as an plain text hex file"<< std::endl;
    std::clog << "  --concat-include -C\tOutput the concatenation of included source files"<< std::endl;
}

namespace DAsmDriver{
    /**
    * Load a file, processing any .include or #include directives.
    * Returns the corresponding source_file structurel
    */
    source_file* getFile(std::string filename, size_t line_offset, std::stringstream &buffer) {

        // Open the file
        std::ifstream file(filename);

        if(!file.good()) {
            // File does not exist
            std::cerr << "Could not open file: " << filename << std::endl;
            return nullptr; // Exit with failure
        }

        std::clog << "Loading " << filename << std::endl;

        size_t line_number = 0;
        source_file* output = new source_file;

        output->path = filename.substr(0, filename.find_last_of("/")+1);
        output->filename = filename.substr(output->path.size(), filename.size());
        output->first_line = line_offset;
        output->last_line = line_offset;
        output->parent_file = nullptr;
        output->included_file = nullptr;
        output->next_file = nullptr;

        source_file* last_included_file = nullptr;

        std::string line;
        line_offset--;
        while(std::getline(file, line)){

            line_number++;
            line_offset++;

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

                        // Add the path to allow relative including
                        included_filename = output->path + included_filename;

                        // Recurse
                        last_included_file = last_included_file == nullptr ? output->included_file = getFile(included_filename, line_offset, buffer) : last_included_file->next_file = getFile(included_filename, line_offset + line_number, buffer); // I like ternaries
                        if(!last_included_file) {
                            // Something went wrong including, Exit with failure
                            return nullptr;
                        }
                        last_included_file->parent_file = output;
                        line_offset += last_included_file->last_line - line_offset;

                        // Don't take this line
                        continue;

                    } else {
                        std::cerr << "Preprocessor error: " << filename << ": " << line_number << ": Incorrect include syntax" << std::endl;
                        std::cerr << line << std::endl;
                        return nullptr; // Exit with failure
                    }

                }
            }

            // If we get here it wasn't an include. Use the line.
            buffer << line << std::endl;
        }

        output->last_line = line_offset;
        // Everything was ok, return true
        return output;
    }

    int concat(const char *infile, std::string outfile, bool standard_output){
        std::stringstream buffer;
        source_file* origin_file = getFile(infile, 1, buffer);
        if(origin_file) {
            std::ofstream out(outfile, std::ios::binary | std::ios::out);

            if(!out.good()) {
                // Cannot write to the output file
                std::cerr << "Cannot open file for writing: " << outfile << std::endl;
                return 1;
            }
            out << buffer.str();
            out.close();

            if(standard_output){
                std::clog << "Displaying " << outfile << std::endl;
                std::cout << buffer.str() << std::endl;
            }

            return 0;
        }
        return 1;
    }

    void setAssemblerFlags(DAsm::Program* program, char flags){
        std:std::cerr << "Flags: " << (0 + flags) << std::endl;
        program->mIgnoreLabelCase = (flags & 0x01) ? !program->mIgnoreLabelCase : program->mIgnoreLabelCase;
        program->mArrangeChunks = (flags & 0x02) ? !program->mArrangeChunks : program->mArrangeChunks;
        program->mStrictDefineCommas = (flags & 0x04) ? !program->mStrictDefineCommas : program->mStrictDefineCommas;
        program->mStrictDirectiveDots = (flags & 0x08) ? !program->mStrictDirectiveDots : program->mStrictDirectiveDots;

    }

    int assemble(const char *infile, std::string outfile, char dasm_flags, bool little_endian, bool hex_output, bool standard_output) {
        // Load up the source code.
        std::stringstream buffer;

        source_file* origin_file = getFile(infile, 1, buffer);

        if(origin_file) {
            DAsm::Program lProgram;
            setAssemblerFlags(&lProgram, dasm_flags);
            if(lProgram.LoadSource(buffer.str())){
                // It compiled.
                unsigned long pSize;
                //The parameters in this call are optional
                DAsm::word* lMemory = lProgram.ToBlock(DAsm::DCPU_RAM_SIZE, pSize);
                // Open the output file
                std::ofstream out(outfile, std::ios::binary | std::ios::out);

                if(!out.good()) {
                    // Cannot write to the output file
                    std::cerr << "Cannot open file for writing: " << outfile << std::endl;
                    delete lMemory;
                    return 1;
                }

                if(standard_output)
                    std::clog << "Displaying " << outfile << std::endl;

                // Dump the program to disk in big endian byte order
                for(int i = 0; i < pSize; i++){
                    // Break into bytes
                    unsigned char bytes[2];
                    if(little_endian){
                        bytes[1] = lMemory[i] >> 8;
                        bytes[0] = lMemory[i] & 0xFF;
                    }else{
                        bytes[0] = lMemory[i] >> 8;
                        bytes[1] = lMemory[i] & 0xFF;
                    }

                    // Write the bytes
                    if(hex_output){
                        buffer.str("");
                        buffer << (i ? " 0x" : "0x") << std::setfill('0') << std::setw(4) << std::hex << int(*((DAsm::word*)bytes));
                        if(standard_output)
                            std::cout << buffer.str();
                        out << buffer.str();
                    }else
                        out.write((char*) bytes, 2);
                }
                delete lMemory;
                std::cout << std::endl;
                std::clog << "Wrote "<< pSize << " words to " << outfile << std::endl;
            }else{
                for(auto&& error : lProgram.mErrors){
                    std::cerr << "Error: " << error << std::endl;
                }
                return 1;
            }

        } else {
            // Something went wrong during preprocessing - an error, or non-existing file
            // Error message already given so just exit with failure
            return 1;
        }

        return 0;
    }

    std::string changeOrAddFileExtension(std::string file, std::string new_extension) {

        std::string dir, filename, rawname;
        std::stringstream output;

        // Seperate the file into the directory and filename
        std::regex re("(^.*(?:\\\\|\\/)|^)(.*)");
        std::smatch match;
        std::regex_search(file, match, re);
        dir = match.str(1);
        filename = match.str(2);

        // Remove file extension from filename if possible and store in rawname
        size_t lastindex = filename.find_last_of(".");
        if(lastindex != std::string::npos) {
            rawname = filename.substr(0, lastindex);
        } else {
            rawname = filename;
        }

        // Add new extension on to rawname
        output << dir << rawname << "." << new_extension;

        return output.str();
    }

    bool isLineInFile(source_file* file, size_t global_line){
        if(file == nullptr)
            return false;

        // Check if the global_line is inside the file range
        if(file->last_line < global_line || file->first_line > global_line)
            return false;

        // Check if the global_line is inside included files
        source_file* included_file = file;
        while((included_file = included_file->next_file) != nullptr){
            if(isLineInFile(included_file, global_line))
                return false;
        }
        return true;
    }

    source_file* getLineSourceFile(source_file* origin_file, size_t global_line){
        source_file* current_file = origin_file;

        while(current_file != nullptr){
            if(current_file->first_line <= global_line && current_file->last_line >= global_line){
                if(isLineInFile(current_file, global_line)){
                    return current_file;
                }else{
                    current_file = current_file->included_file;
                }
            }else{
                current_file = current_file->next_file;
            }
        }
        return nullptr;
    }
}
