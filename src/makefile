IDIR =.
GTESTIDIR =/home/geri/workspace/googletest/googletest/include
GTESTLIB=/home/geri/workspace/googletest/build/googletest
BOOST_INCLUDEDIR=/home/geri/thirdparty/boost_1_60_0/include
BOOST_LIBRARYDIR=/home/geri/thirdparty/boost_1_60_0/libs
CC=g++
CFLAGS_DEBUG=-I$(GTESTIDIR) -I$(BOOST_INCLUDEDIR) -L$(GTESTLIB) -L$(BOOST_LIBRARYDIR) -DDEBUG -fsanitize=undefined -pthread -g --std=c++11
CFLAGS=-I$(GTESTIDIR) -I$(BOOST_INCLUDEDIR) -L$(GTESTLIB) -L$(BOOST_LIBRARYDIR) -O2 -DNDEBUG -pthread --std=c++11

ODIR=../obj

LIBS=-lm


cout: main.cpp test.cpp
	g++ $(CFLAGS_DEBUG) -o ../bin/gcc/mdm-g main.cpp -lboost_system
	g++ $(CFLAGS_DEBUG) -o ../bin/gcc/test-driver-g test.cpp -lpthread -lgtest -lgtest_main -lboost_system
	g++ $(CFLAGS) -o ../bin/gcc/mdm main.cpp -lboost_system
	g++ $(CFLAGS) -o ../bin/gcc/test-driver test.cpp -lpthread -lgtest -lgtest_main -lboost_system
	

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o