// Driver program to assemble code

#include "driver.h"

int main(int argc, char** argv) {

    const char *infile;
    std::string outfile;

    bool big_endian, standard_output, file_tracability, full_tracability = 0;
    char output_type = 0; // 0:normal, 1:hex, 2:concat included
    char dasm_flags = 0;

    unsigned int i;

    if(argc < 2 || containsFlag(argc, argv, "--help", 0)) {
        std::cerr << std::endl << "Please enter a input_file name." << std::endl;
        displayUsage(argv[0]);
        return 1;
    }


    big_endian = containsFlag(argc, argv, "--big-endian") != 0 || containsFlag(argc, argv, "-b") != 0;
    standard_output = containsFlag(argc, argv, "--standard-output") != 0 || containsFlag(argc, argv, "-s") != 0;
    full_tracability = containsFlag(argc, argv, "--file-full-tracability") != 0 || containsFlag(argc, argv, "-T") != 0;
    file_tracability = full_tracability ||containsFlag(argc, argv, "--file-tracability") != 0 || containsFlag(argc, argv, "-t") != 0;
    output_type = (containsFlag(argc, argv, "--hex-output") != 0 || containsFlag(argc, argv, "-H") != 0) ? 1 : 0;
    if(containsFlag(argc, argv, "--concat-include") || containsFlag(argc, argv, "-C") != 0){
        if(output_type){
            std::cerr << std::endl << "Hex-output mode and concat-include mode are mutually exclusives." << std::endl;
            displayUsage(argv[0]);
            return 1;
        }
        output_type = 2;
    }

    i = 1;
    while(((i = containsFlag(argc, argv, "--assembler-flags", i)) || (i = containsFlag(argc, argv, "-f", i))) && i != 0 && i != argc){ // Dat condition trick
        dasm_flags = parseAssemblerFlags(argv[i], dasm_flags);
    }

    if((i = containsFlag(argc, argv, "-o", 1)) > 1) {
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

    switch(output_type){
        default:
        case 0:
            return DAsmDriver::assembleToBinary(infile, outfile, dasm_flags, !big_endian, standard_output);
        case 1:
            return DAsmDriver::assembleToHex(infile, outfile, dasm_flags, !big_endian, standard_output, file_tracability, full_tracability);
        case 2:
            return DAsmDriver::concat(infile, outfile, standard_output, file_tracability, full_tracability);
    }
}

char parseAssemblerFlags(std::string arg, char current_flags){

    arg = arg.substr(arg.find_last_of("=") + 1, arg.size());

    std::list<std::string> flags;
    DAsm::splitString(arg, "[^A-Z]+", flags);
    for(auto flag : flags){
        std::cerr << "Flag: \"" << flag << "\"" << std::endl;
        if(flag == "IGNORELABELCASE"){
            current_flags |= 0x01;
            continue;
        }
        if(flag == "ARRANGECHUNKS"){
            current_flags |= 0x02;
            continue;
        }
        if(flag == "STRICTDEFINECOMMAS"){
            current_flags |= 0x04;
            continue;
        }
        if(flag == "STRICTDIRECTIVEDOTS"){
            current_flags |= 0x08;
            continue;
        }
    }
    return current_flags;
}


int containsFlag(int argc, char** argv, std::string flag, unsigned int starting_arg_index){
    std::string t;
    bool plain_text_flag, plain_text_arg;

    plain_text_flag = flag.size() > 1 && flag[0] == '-' && flag[1] == '-';
    starting_arg_index++;

    if(starting_arg_index >= argc){
        if(starting_arg_index == argc)
            return argc;
        starting_arg_index = 1;
    }



    for(int i = starting_arg_index; i < argc; i++){
        plain_text_arg = argv[i][0] == '-' && argv[i][1] == '-';

        if(plain_text_arg != plain_text_flag || argv[i][1] == 0)
            continue;

        t = argv[i];

        if(!plain_text_arg && argv[i][0] == '-'){ // If the flag is short then check each char of arg until  null or a '=' '
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
    std::clog << std::endl << std::left << std::setfill(' ') << std::setw(8) << "DAsm" << std::endl;
    std::clog << "Usage:" << std::endl;
    std::clog << "  " << exec << " <input_file> [-bsHC] [-o output_file][--big-endian][--hex-output/concat-include][[--assembler-flags||-f]=<flag1>[,-+.]<flag2>...]" << std::endl;
    std::clog << "Options:" << std::endl;
    std::clog << std::setw(30) << "  --big-endian  -b" << "Output in big-endian (inoperant in concat-include mode)"<< std::endl;
    std::clog << std::setw(30) << "  --standard-output  -s" << "Copy the output in stdout (inoperant in normal mode)"<< std::endl;
    std::clog << std::setw(30) << "  --file-tracability -t" <<"Output the concatenation of included source files with filename and line (inoperant in normal mode)"<< std::endl;
    std::clog << std::setw(30) << "  --file-full-tracability -T" <<"Output the concatenation of included source files with relative path and filename and line (inoperant in normal mode)(overrides non-full mode)"<< std::endl;


    std::clog << std::setw(30) << "  --assembler-flags=  -f=" << "Invert default state of DAsm flags (inoperant in concat-include mode)(can be used multiple times)"<< std::endl;

    std::clog << std::setw(30) << "  --hex-output  -H" << "Output the binary as an plain text hex file"<< std::endl;
    std::clog << std::setw(30) << "  --concat-include -C" <<"Output the concatenation of included source files"<< std::endl;
}

namespace DAsmDriver{
    /**
    * Load a file, processing any .include or #include directives.
    * Returns the corresponding source_file structurel
    */
    source_file* getFile(std::string filename, size_t line_offset, std::stringstream &buffer, source_file* parent_file) {

        // Open the file
        std::ifstream file(filename);

        if(!file.good()) {
            // File does not exist
            std::cerr << "Could not open file: " << filename << std::endl;
            return nullptr; // Exit with failure
        }

        size_t line_number = 0;
        source_file* output = new source_file;

        output->path = filename.substr(0, filename.find_last_of("/")+1);
        output->filename = filename.substr(output->path.size(), filename.size());
        output->path = tidyPath(output->path);
        output->first_line = line_offset;
        output->last_line = line_offset;
        output->parent_file = parent_file;
        output->included_file = nullptr;
        output->next_file = nullptr;

        if(checkIncludeRecursion(output, 1)){
            std::cerr << "Circular include error :" << std::endl;
            while(output != nullptr){
                std::cerr << output->path << output->filename << (output->parent_file != nullptr ? " > " : "");
                output = output->parent_file;
            }
            std::cerr << std::endl;
            return nullptr;
        }


        std::cerr << "Loading " << output->path << output->filename  << " included in " << (output->parent_file == nullptr ? "-None-" : output->parent_file->path + output->parent_file->filename) << std::endl;



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
                        last_included_file = last_included_file == nullptr ? output->included_file = getFile(included_filename, line_offset, buffer, output) : last_included_file->next_file = getFile(included_filename, line_offset, buffer, output); // I like ternaries
                        if(!last_included_file) {
                            // Something went wrong including, Exit with failure
                            return nullptr;
                        }
                        line_offset = last_included_file->last_line;

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
            //std::cout <<"'"<< line << "'"<<std::endl;
            buffer << line << std::endl;
        }

        output->last_line = line_offset;
        // Everything was ok, return true
        return output;
    }

    std::string tidyPath(std::string path){
        bool absolute = path[0] == '/' || path[0] == '\\';
        path = std::regex_replace(path, std::regex("^[\\\\\\/]*(\\.[\\\\\\/])+"), "");
        std::list<std::string> folders;
        DAsm::splitString(path, "[\\\\\\/]+(\\.[\\\\\\/])*", folders);
        std::string current, next;
        path = current = "";
        if(folders.size() > 0){
            current = folders.front();
            folders.pop_front();
        }

        while(current != "" ||next != ""){
            if(folders.size() > 0){
                next = folders.front();
                folders.pop_front();
            }else{
                next = "";
            }
            if(next != ".." || current == ".."){
                if(current != "")
                    path = path + current + "/";
                current = next;
            }else{
                current = "";
            }
        }
        path = absolute ? "/" + path : path;
        return path;
    }

    int concat(const char *infile, std::string outfile, bool standard_output, bool file_tracability, bool full_tracability){
        std::stringstream buffer;
        source_file* origin_file = getFile(infile, 1, buffer);
        if(origin_file) {
            std::ofstream out(outfile, std::ios::binary | std::ios::out);

            if(!out.good()) {
                // Cannot write to the output file
                std::cerr << "Cannot open file for writing: " << outfile << std::endl;
                return 1;
            }
            if(file_tracability || full_tracability)
                buffer.str(concatWithFileDisplay(buffer.str(), origin_file, full_tracability));
            out << buffer.str();
            out.close();

            if(standard_output){
                std::clog << "Displaying " << outfile << std::endl;
                //Buffering to avoid terminal realted issue
                std::string buff;
                while(buffer.good()){
                    std:getline(buffer, buff);
                    std::cout << buff << std::endl;
                }

            }
            return 0;
        }
        return 1;
    }


    std::string concatWithFileDisplay(std::string concatSources, source_file* origin_file, bool relative_path){
        __label__ exit;
        if(origin_file == nullptr)
            return "";
        std::stringstream output;

        std::list<std::string> lines;
        output.str(concatSources);
        while(output.good()){
            std::getline(output, concatSources);
            lines.push_back(concatSources);
        }
        output.str("");
        output.clear();
        //DAsm::splitString(concatSources, "[\n\r]", lines);

        source_file* current_file = origin_file;
        source_file* included_file = current_file->included_file;

        size_t line = 0;
        size_t global_line_total_number_width = std::to_string(lines.size()).length();
//std::cout << "Lines size: " <<lines.size() << std::endl;
        while(current_file != nullptr && lines.size() > 0){
            line++;
            /*std::cout << std::left << std::setfill(' ') << std::setw(20) << (current_file->parent_file != nullptr ? current_file->parent_file->filename : "-None-");
            std::cout << "< " << std::setw(20) << current_file->filename << "< " << (included_file != nullptr ? included_file->filename : "-None-") << std::endl;
            if(current_file->next_file != nullptr)
                std::cout << std::setw(20) << "" << "  ^ "  << current_file->next_file->filename << std::endl;
                */
            // loop in case of nested files ending at the same line
            while(line > current_file->last_line){
                included_file = current_file->next_file;
                current_file = current_file->parent_file;
            //std::cout << "Include done returning " << (current_file != nullptr ? current_file->filename : "-None-") << std::endl;
                if(current_file == nullptr)
                    goto exit;
            }

            // loop in case of nested files containing include on first line
            while(included_file != nullptr && line >= included_file->first_line){
                current_file = included_file;
                included_file = current_file->included_file;
            }
            if(current_file != nullptr){
                output << std::left << std::setfill(' ') << std::setw(relative_path ? 60 : 30) << ((relative_path ? current_file->path : "") + current_file->filename) << std::right << std::setfill('0') << "|" << std::setw(global_line_total_number_width) << int(line) << "|" << std::setw(global_line_total_number_width) << int(getLocalLineNumber(current_file, line)) << "|" << lines.front() << std::endl;
                if(output.fail())
                    std::cerr << "Concat With File Display fail at some point" << std::endl;
                lines.pop_front();
            }
        }
        exit:
        return output.str();
    }

    void setAssemblerFlags(DAsm::Program* program, char flags){
        std:std::cerr << "Flags: " << (0 + flags) << std::endl;
        program->mIgnoreLabelCase = (flags & 0x01) ? !program->mIgnoreLabelCase : program->mIgnoreLabelCase;
        program->mArrangeChunks = (flags & 0x02) ? !program->mArrangeChunks : program->mArrangeChunks;
        program->mStrictDefineCommas = (flags & 0x04) ? !program->mStrictDefineCommas : program->mStrictDefineCommas;
        program->mStrictDirectiveDots = (flags & 0x08) ? !program->mStrictDirectiveDots : program->mStrictDirectiveDots;

    }

    int assembleToHex(const char *infile, std::string outfile, char dasm_flags, bool little_endian, bool standard_output, bool file_tracability, bool full_tracability){
        // Load up the source code.
        std::stringstream buffer;

        source_file* origin_file = getFile(infile, 1, buffer);

        if(origin_file) {
            DAsm::Program lProgram;
            setAssemblerFlags(&lProgram, dasm_flags);
            if(lProgram.LoadSource(buffer.str(), false)){
                // It compiled.

                // Open the output file
                std::ofstream out(outfile, std::ios::binary | std::ios::out);

                if(!out.good()) {
                    // Cannot write to the output file
                    std::cerr << "Cannot open file for writing: " << outfile << std::endl;
                    return 1;
                }

                //The parameters in this call are optional
                buffer.str(lProgram.ToHex("0x", " 0x", "\n", little_endian, false));
                if(file_tracability || full_tracability)
                    buffer.str(concatWithFileDisplay(buffer.str(), origin_file, full_tracability));
                buffer.clear();
                out << buffer.str();
                out.close();

                if(standard_output){
                    std::clog << "Displaying " << outfile << std::endl;
                    //Buffering to avoid terminal realted issue
                    std::cout << buffer.str() << std::endl;
                    /*std::string buff;
                    while(buffer.good()){
                        std:getline(buffer, buff);
                        std::cout << buff << std::endl << std::flush;
                    }*/
                }
            }else{
                std::list<std::string> concat_list;
                std::string concat = buffer.str();
                DAsm::splitString(concat, "\\r?\\n", concat_list);
                std::vector<std::string> concat_vector{ std::begin(concat_list), std::end(concat_list) };
                displayErrors(lProgram, concat_vector, origin_file);
                return 1;
            }

        } else {
            // Something went wrong during preprocessing - an error, or non-existing file
            // Error message already given so just exit with failure
            return 1;
        }

        return 0;
    }

    int assembleToBinary(const char *infile, std::string outfile, char dasm_flags, bool little_endian, bool standard_output){

        DAsm::word* lMemory;
        unsigned long pSize;

        source_file* origin_file = assemble(infile, lMemory, pSize, dasm_flags, little_endian);

        if(origin_file) {
            std::ofstream out(outfile, std::ios::binary | std::ios::out);

            if(standard_output)
                std::clog << "Outputing " << outfile << " to stdout" << std::endl;

            // Dump the program to disk and on stdout if asked
            for(int i = 0; i < pSize; i++){
                out.write((char*) (lMemory + i), 2);
                if(standard_output)
                    std::cout.write((char*) (lMemory + i), 2);
            }
            delete lMemory;

            std::clog << "Wrote "<< pSize << " words to " << outfile << std::endl;
        }else{
            // Something went wrong during preprocessing - an error, or non-existing file
            // Error message already given so just exit with failure
            return 1;
        }

        return 0;
    }

    source_file* assemble(const char *infile, DAsm::word* output, unsigned long& pSize, char dasm_flags, bool little_endian){
        // Load up the source code.
        std::stringstream buffer;

        source_file* origin_file = getFile(infile, 1, buffer);

        if(origin_file) {
            DAsm::Program lProgram;
            setAssemblerFlags(&lProgram, dasm_flags);

            if(lProgram.LoadSource(buffer.str(), false)){
                // It compiled.

                //The parameters in this call are optional
                DAsm::word* lMemory = lProgram.ToBlock(DAsm::DCPU_RAM_SIZE, pSize, little_endian);
                *output = *lMemory;
                return origin_file;
            }

            std::list<std::string> concat_list;
            std::string concat = buffer.str();
            DAsm::splitString(concat, "\\r?\\n", concat_list);
            std::vector<std::string> concat_vector{ std::begin(concat_list), std::end(concat_list) };
            displayErrors(lProgram, concat_vector, origin_file);
            return nullptr;
        }
        // Something went wrong during preprocessing - an error, or non-existing file
        // Error message already given so just exit with failure
        return nullptr;
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

    source_file* getIncludeTreeRoot(source_file* file){
            source_file* current_file = file;
            while(current_file != nullptr && current_file->parent_file!= nullptr){
                current_file = current_file->parent_file;
            }
            return current_file;
    }

    bool isInsideIncludeTree(source_file* origin_file, source_file* new_file){
        source_file* current_file = origin_file;
        if(new_file == nullptr)
            return false;
        while(current_file != nullptr){
            if(current_file->filename == new_file->filename &&current_file->path == new_file->path)
                return true;
            if(current_file->included_file != nullptr && isInsideIncludeTree(current_file->included_file, new_file))
                return true;
            current_file = current_file->next_file;
        }
        return false;
    }

    bool checkIncludeRecursion(source_file* file, unsigned int recursion_counter){
        source_file* current_file = file;
        while(current_file != nullptr && current_file->parent_file != nullptr){
            if(current_file->filename == file->filename && current_file->path == file->path){
                if(recursion_counter-- == 0)
                    return true;
            }
            current_file = current_file->parent_file;
        }
        return false;
    }

    bool isLineInFile(source_file* file, size_t global_line){
        if(file == nullptr)
            return false;

        // Check if the global_line is inside the file range
        if(file->last_line <= global_line || file->first_line >= global_line)
            return false;

        // Check if the global_line is inside an included files
        source_file* included_file = file->included_file;
        while(included_file != nullptr){
            if(included_file->last_line <= global_line || included_file->first_line >= global_line)
                return false;
            included_file = included_file->next_file;
        }
        return true;
    }

    source_file* getLineSourceFile(source_file* origin_file, unsigned int global_line){
        source_file* included_file;
        if(origin_file == nullptr)
            return nullptr;
        if(origin_file->first_line <= global_line && origin_file->last_line >= global_line){
            included_file = origin_file->included_file;
            while(included_file != nullptr && included_file->first_line <= global_line ){
                if(included_file->last_line >= global_line)
                    return getLineSourceFile(included_file, global_line);
                included_file = included_file->next_file;
            }
            return origin_file;
        }
        return nullptr;
    }
/* WIP
    unsigned int getLocalLineNumberFromOrigin(source_file* file,  unsigned int global_line){
        source_file* included_file;
        if(file == nullptr)
            return 0;
        unsigned int local_line = file->first_line;

        if(file->first_line <= global_line && file->last_line >= global_line){
            included_file = file->included_file;
            while(included_file != nullptr && included_file->first_line <= global_line ){
                if(included_file->last_line >= global_line)
                    return getLocalLineNumberFromOrigin(included_file, global_line);
                included_file = included_file->next_file;
            }
            return local_line;
        }
        return 0;
    }*/

    unsigned int getLocalLineNumber(source_file* file,  unsigned int global_line){
        source_file* included_file;
        if(file == nullptr)
            return 0;
        unsigned int local_line = global_line - file->first_line + 1;

        if(file->first_line <= global_line && file->last_line >= global_line){
            included_file = file->included_file;
            while(included_file != nullptr && included_file->first_line <= global_line ){
                local_line -= included_file->last_line - included_file->first_line;
                included_file = included_file->next_file;
            }
            return local_line;
        }
        return 0;
    }

    void displayErrors(DAsm::Program program, std::vector<std::string> concat, source_file* origin_file){
        source_file* file;
        for(auto&& error : program.mErrors){
            file = nullptr;
            program.updateError(error);
            if(error.source != nullptr){
                file = getLineSourceFile(origin_file, error.source->mLineNumber);

                if(file != nullptr)
                    std::cerr << file->path << file->filename << ":" << int(getLocalLineNumber(file, error.source->mLineNumber)) << ": " << error.message << std::endl;
                else
                    std::cerr << "<concat>:" << int(error.source->mLineNumber) << ": " << error.message << std::endl;
                if(error.source->mSourceLine != "")
                    std::cerr << error.source->mSourceLine << std::endl << std::endl;
                else//*/
                    std::cerr << "- Empty Line -" << std::endl << std::endl;
            }else{
                std::cerr << "Error "<< (error.source == nullptr ? "" : std::to_string(error.source->mLineNumber) + " ") <<": " << error.message << std::endl;
            }

        }
    }
}
