CXX := g++-4.7
#CXXFLAGS := -c -std=c++11 `pkg-config --cflags opencv`
CXXFLAGS := -c -std=c++11 -I/usr/include/opencv -O2
LDFLAGS := `pkg-config --libs opencv` -lmraa
DEPFLAGS := -MM

OBJDIR := obj
OBJS := $(addprefix $(OBJDIR)/,main.o webcam.o laser.o)

$(OBJDIR)/%.o : %.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) $< -o $@

all : main

main : $(OBJS)
	$(CXX) $(LDFLAGS) $(OBJS) -o $@

$(OBJDIR) :
	mkdir $(OBJDIR)

clean :
	rm -rf main $(OBJDIR)

dep:
	$(CXX) $(DEPFLAGS) $(CXXFLAGS) *.cpp

$(OBJDIR)/main.o: main.cpp webcam.hpp
$(OBJDIR)/webcam.o: webcam.cpp webcam.hpp
$(OBJDIR)/laser.o: laser.cpp laser.hpp


