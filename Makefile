CXX_OBJS = \
	source/main.o \
	source/fatfs.o

CXX=g++
INCLUDE += include/
CXXFLAGS += -DDEBUG
CXXFLAGS +=-std=c++14 -Wall -Wextra -Wsign-compare -Wfloat-equal -g -ggdb
CXXFLAGS += -I$(INCLUDE)

BINDIR=bin
BINARY=fat

all: $(BINARY)

$(BINARY): $(CXX_OBJS)
	@rm -rf $(BINDIR)
	@mkdir $(BINDIR)
	@echo "LINKING $@"; $(CXX) -o $(BINDIR)/$@ $(CXX_OBJS)

.cpp.o:
	@echo "CXX $@"; $(CXX) $(CXXFLAGS) -o $@ -c $<

clean:
	@echo "Cleaning Project..."; rm -rf $(BINDIR) $(CXX_OBJS)



