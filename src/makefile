IDIR =.
GTESTIDIR =/home/geri/workspace/googletest/googletest/include
GTESTLIB=/home/geri/workspace/googletest/build/googletest
CC=g++
CFLAGS_DEBUG=-I$(GTESTIDIR) -L$(GTESTLIB) -DDEBUG -pthread -g --std=c++11
CFLAGS=-I$(GTESTIDIR) -L$(GTESTLIB) -O2 -pthread --std=c++11

ODIR=../obj

LIBS=-lm


cout: main.cpp test.cpp
	g++ $(CFLAGS_DEBUG) -o ../bin/mdm-g main.cpp
	g++ $(CFLAGS_DEBUG) -o ../tests/test-driver-g test.cpp -lpthread -lgtest -lgtest_main

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o