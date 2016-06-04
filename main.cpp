//Example usage of the DAsm library

#include <iostream>
#include <string>


#include "DAsm.h"
//The \ at the end of lines is to escape the line escape for better representation in editors
std::string source =
";SPI Spy\n\
;Author: Alfie275\n\
FLAG ARRANGECHUNKS, 1\n\
\n\
;SPI echo program\n\
org 1000\n\
.reserve 10\n\
dat \"Test\",0\n\
.asciiz \"Test\"\n\
ADD A, B\n\
ADD A, C\n\
macro SUM = ADD %0, %1 %n ADD %0, %2\n\
SUM A, B, C\n\
DAT table_loop-device_loop\n\
DAT 2/0\n\
HLT 5\n\
macro SUPER_SUM = SUM %0, %1, %2 %n SUM %0, %3, %4\n\
SUPER_SUM A, B, C, X, Y\n\
macro OOPS = OOPS\n\
.def counter, 1\n\
.def num1, 2\n\
.def num2, 3\n\
.def num3, 5\n\
macro DONUM=DAT num%e0\n\
macro NUM =DONUM counter %n.def counter, counter+1\n\
NUM\n\
NUM\n\
NUM\n\
def no_comma_define 0x00ab\n\
dat no_comma_define\n\
;Using OOPS would cause a max recursion error\n\
OPCODE MYOPCODE  , 0x1F, A\n\
MYOPCODE A\n\
org 0\n\
 SET   A, device_loop  -  table_loop\n\
;Generic code to find devices specified in lookup table:\n\
    HWN J       ;Get number of attached devices\n\
    LOG J\n\
	SET A, 1	;Current device number\n\
\n\
:device_loop\n\
	HWQ I		;Get device info for Ith number device\n\
    SET PUSH, A\n\
    SET PUSH, B\n\
    SET B, 0    ;Current table lookup index\n\
\n\
:table_loop\n\
    SET A, B    ;Set A to current lookup index\n\
    MUL A, 5    ;Multiply by 5 to get offset\n\
    ADD A, device_lookup    ;Pointer to device lookup table entry\n\
\n\
    SET C, [A]\n\
    SET Z, [A+1]\n\
\n\
    IFE C, 0x0000\n\
    IFE Z, 0x0000   ;Reached end of table\n\
    SET PC, next_device     ;Go to next device\n\
\n\
    IFE X, Z\n\
    IFE Y, C\n\
    SET PC, correct_manufacturer ;Manufacturer matches, check device ID\n\
    \n\
\n\
    SET PC, table_next     ;Manufacturer does not match, check next one\n\
:correct_manufacturer\n\
    SET Y, [SP]\n\
    SET X, [SP+1] ;Get device ID\n\
\n\
    set C, [A+2]\n\
    set Z, [A+3] \n\
\n\
    IFE X, Z\n\
    IFE Y, C    ;Check device ID matches\n\
    SET PC, table_match     ;Device matched!\n\
    \n\
:table_next\n\
    ADD B, 1    ;Check next table entry\n\
    SET PC, table_loop\n\
\n\
:table_match\n\
    SET A, [A+4]    ;Grab pointer to address storage\n\
    SET [A], I  ;set address\n\
\n\
:next_device\n\
    ADD I, 1    ;next device\n\
\n\
    SET X, POP\n\
    SET X, POP ;Get rid of stored device ID from stack\n\
\n\
    IFE I, J\n\
	SET PC, devices_done ;All devices looked for!\n\
\n\
	SET PC, device_loop\n\
:devices_done\n\
\n\
    \n\
;Main body of code\n\
\n\
    JSR setup_spi\n\
\n\
\n\
;debug stuff!!!\n\
    INT recv_message\n\
;debug stuff!!!\n\
\n\
    SET PC, end\n\
\n\
:setup_spi\n\
    IAS interrupt\n\
\n\
    SET A, 0x0001\n\
    SET B, 0x0001\n\
    HWI [spi_address]\n\
\n\
    SET A, 0x0004\n\
    SET B, 0x0000\n\
    SET C, recv_message\n\
    HWI [spi_address]\n\
    \n\
    SET PC, POP\n\
    \n\
\n\
:update_display\n\
    SET A, 0        ;Choose MEM_MAP_SCREEN\n\
	SET B, vram		;Set video ram location\n\
	HWI [display_address]   ;Update display\n\
    SET A, 3\n\
    SET B, 0xD\n\
    HWI [display_address]\n\
    \n\
    SET PC, POP\n\
\n\
:interrupt\n\
    IFN A, recv_message\n\
    RFI 0\n\
\n\
    JSR update_last_recv\n\
\n\
    SET PUSH, 0x8080\n\
    SET PUSH, 0\n\
    SET PUSH, last_text\n\
    JSR print_str\n\
\n\
    SET PUSH, 0xA000\n\
    SET PUSH, 32\n\
    SET PUSH, last_recv\n\
    JSR print_str\n\
\n\
    JSR update_display\n\
\n\
    RFI 0\n\
\n\
:gen_indicator_str ;A - 16bit sequence, B - string ptr, C - temp stuff\n\
    \n\
    SET C, A\n\
    AND C, 0x8000\n\
\n\
    SET [B], [indicator_symbol]\n\
    IFE C, 0x8000\n\
    SET [B], [indicator_symbol+1]\n\
\n\
    SHL A, 1\n\
\n\
    ADD B, 1\n\
    IFE [B], 0x0000\n\
    SET PC, POP\n\
    SET PC, gen_indicator_str\n\
    \n\
\n\
:update_last_recv\n\
    SET A, 0x000\n\
    HWI spi_address\n\
\n\
\n\
;debug stuff!!!\n\
    SET A, 0b1000000010000000\n\
    SET B, 0b1010101010101010\n\
;debug stuff!!!\n\
\n\
    SET PUSH, B\n\
\n\
    SET [last_recv + 16], 0x0000\n\
    SET B, last_recv\n\
    JSR gen_indicator_str\n\
    \n\
    SET A, POP\n\
\n\
    SET B, last_recv + 16\n\
    JSR gen_indicator_str\n\
\n\
    set PC, POP\n\
\n\
\n\
:print_str ;args -  colour, screen position, string pointer\n\
    SET PUSH, X\n\
    SET PUSH, Y     ;Store registers we're gonna use\n\
    SET PUSH, Z\n\
    SET X, [SP+4]   ;String pointer\n\
    SET Y, [SP+5]   ;Screen pos\n\
    SET Z, [SP+6]\n\
\n\
    ADD Y, vram ;Turn into vram pointer\n\
\n\
:print_str_loop     \n\
    IFE [X], 0x0000\n\
    SET PC, print_str_end\n\
    BOR [X], Z\n\
    SET [Y], [X]\n\
    ADD Y, 1\n\
    ADD X, 1\n\
    SET PC, print_str_loop\n\
:print_str_end\n\
    SET Z, POP\n\
    SET Y, POP\n\
    SET X, POP\n\
    SET [SP+3], PEEK    ;Copy return address to edge of our part of stack\n\
    ADD SP, 3   ;pop args\n\
    SET PC, POP     ;return\n\
	\n\
:end\n\
	SET PC, end\n\
\n\
\n\
:display_address\n\
DAT 0xFFFF\n\
:spi_address\n\
DAT 0xFFFF\n\
:device_lookup\n\
DAT 0x1c6c,0x8b36,0x7349,0xf615,display_address\n\
DAT 0xA87C,0x900E,0xE023,0x9088,spi_address\n\
DAT 0x0000,0x0000\n\
:indicator_symbol\n\
\n\
DAT \"01\",0x0000\n\
:vram\n\
DAT \"                                \" \n\
DAT \"                                \" \n\
DAT \"                                \"\n\
DAT \"                                \" \n\
DAT \"                                \" \n\
DAT \"                                \" \n\
DAT \"                                \" \n\
DAT \"                                \" \n\
DAT \"                                \" \n\
DAT \"                                \" \n\
DAT \"                                \" \n\
DAT \"                                \"\n\
DAT 0x0000\n\
DAT \"                                \" ;Bit of a buffer incase someone makes a silly mistake ;)\n\
:last_recv\n\
DAT \"                                \" , 0x0000\n\
:last_text\n\
DAT \"Last received 32 bit data:\", 0x0000\n\
:recv_message\n\
DAT 0x7CE5";

int main()
{
    DAsm::Program* lProgram = new DAsm::Program();

    if(lProgram->LoadSource(source)){

        //Note here we can overload the separator from the default " 0x" to whatever we want

        std::cout << "\n\nOutput:\n" << lProgram->ToHex(" 0x") << "\n" << std::endl;

        unsigned long pSize;
        //The parameters in this call are optional
        DAsm::word* lMemory = lProgram->ToBlock(DAsm::DCPU_RAM_SIZE, pSize);

        std::cout << "Length: "<< pSize << std::endl;
        delete lMemory;
    }else{
        for(auto&& error : lProgram->mErrors){
            std::cerr << "Error: " << error << std::endl;
        }

    }
    delete lProgram;

}
