CXX = g++
CXXFLAGS = -Wall -g -std=c++17
SRCDIR = src/
BINDIR = target/
MAIN = $(BINDIR)vmtranslator
SRCS = $(SRCDIR)main.cpp $(SRCDIR)parser.cpp $(SRCDIR)code_writer.cpp
OBJS = $(SRCS:.cpp=.o)

.PHONY: clean all

all: $(BINDIR) $(MAIN)
	@echo Compiled $(MAIN) successfully!

$(BINDIR):
	@mkdir -p $(BINDIR)

$(MAIN): $(OBJS) 
	$(CXX) $(CXXFLAGS) -o $(MAIN) $(OBJS)

$(SRCDIR)%.o: $(SRCDIR)%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	$(RM) $(SRCDIR)*.o *~ $(MAIN)