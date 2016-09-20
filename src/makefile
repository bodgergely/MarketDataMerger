IDIR =.
GTESTIDIR =/home/geri/workspace/googletest/googletest/include
GTESTLIB=/home/geri/workspace/googletest/build/googletest
CC=g++
CFLAGS_DEBUG=-I$(GTESTIDIR) -L$(GTESTLIB) -DDEBUG -fsanitize=undefined -pthread -g --std=c++11
CFLAGS=-I$(GTESTIDIR) -L$(GTESTLIB) -O2 -DNDEBUG -pthread --std=c++11

ODIR=../obj

LIBS=-lm


cout: main.cpp test.cpp
	g++ $(CFLAGS_DEBUG) -o ../bin/gcc/mdm-g main.cpp
	g++ $(CFLAGS_DEBUG) -o ../bin/gcc/test-driver-g test.cpp -lpthread -lgtest -lgtest_main
	g++ $(CFLAGS) -o ../bin/gcc/mdm main.cpp
	g++ $(CFLAGS) -o ../bin/gcc/test-driver test.cpp -lpthread -lgtest -lgtest_main
	clang++ $(CFLAGS_DEBUG) -o ../bin/clang/mdm-g main.cpp
	clang++ $(CFLAGS_DEBUG) -o ../bin/clang/test-driver-g test.cpp -lpthread -lgtest -lgtest_main
	clang++ $(CFLAGS) -o ../bin/clang/mdm main.cpp
	clang++ $(CFLAGS) -o ../bin/clang/test-driver test.cpp -lpthread -lgtest -lgtest_main
	

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o