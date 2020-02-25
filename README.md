# DASM
  DCPU Assembler for TechCompliant

##Install

###Compile on Windows, starting without the tools
- [Download](https://git-scm.com/download/win) and Install Git
    You may want to verify if the git bin folder is in pour PATH
- [Download](http://www.redhat.com/services/custom/cygwin/) RedHat Cygwin Setup
- Be sure to setup cygwin to include the good version of mingw-g++ for you system :
    On a Windows 64bit you need either mingw64's version for i686 (for 32bits compilation) or x86_64(for 64bits compilation)
- Set the CXX environment variable in your .bashrc to use the right mingw g++
- Open a terminal win cygwin
- Clone this repository somewhere
- Copy libgcc_*.dll and libstdc++*.dll to your local DASM folder
    These files are located in <cygwin folder>/usr/<your favorite mingw version>/sys-root/mingw/bin
- Run make


##Usage
```
Usage:
  dasm.exe <input_file> [-bsHC] [-o output_file][--big-endian][--hex-output/concat-include][--assembler-flags||-f]=<flag1>[,-+.]<flag2>...]
Options:
  --big-endian  -b              Output in big-endian (inoperative in concat-include mode)
  --standard-output  -s         Copy the output in stdout (inoperative in normal mode)
  --file-traceability -t         Output the concatenation of included source files with filename and line (inoperative in normal mode)
  --file-full-traceability -T    Output the concatenation of included source files with relative path and filename and line (inoperative in normal mode)(overrides non-full mode)
  --assembler-flags=  -f=       Invert default state of DAsm flags (inoperative in concat-include mode)
  --hex-output  -H              Output the binary as an plain text hex file
  --concat-include -C           Output the concatenation of included source files
  ```
