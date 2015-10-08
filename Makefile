# Default settings
CPPFLAGS += -g -std=c++11
LDFLAGS += 
# Note that you need a compiler actually implementing std::regex (not gcc 4.8)

all: demo dasm

demo: main.o DAsm.o
	$(CXX) $^ -o $@ $(LDFLAGS)
	
dasm: driver.o DAsm.o
	$(CXX) $^ -o $@ $(LDFLAGS)
	
clean:
	rm -f *.o demo dasm
	
