IDIR =.
GTESTIDIR =/home/geri/workspace/googletest/googletest/include
GTESTLIB=/home/geri/workspace/googletest/build/googletest
CC=g++
CFLAGS_DEBUG=-I$(GTESTIDIR) -L$(GTESTLIB) -DDEBUG -pthread -g --std=c++11
CFLAGS=-I$(GTESTIDIR) -L$(GTESTLIB) -O2 -DNDEBUG -pthread --std=c++11

ODIR=../obj

LIBS=-lm


cout: main.cpp test.cpp
	g++ $(CFLAGS_DEBUG) -o ../bin/mdm-g main.cpp
	g++ $(CFLAGS_DEBUG) -o ../bin/test-driver-g test.cpp -lpthread -lgtest -lgtest_main
	g++ $(CFLAGS) -o ../bin/mdm main.cpp
	g++ $(CFLAGS) -o ../bin/test-driver test.cpp -lpthread -lgtest -lgtest_main
	

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o