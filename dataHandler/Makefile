CFLAGS = -std=c++11 ${include_dir}
include_dir = -I /home/meteorgan/workspace/CPlusPlus/muduo
lib_flags = -L /home/meteorgan/workspace/CPlusPlus/build/release/lib -lmuduo_base_cpp11 -lmuduo_net_cpp11 -pthread

all: DataServer DataHandler 

DataServer: dataServer.o genNumberExecutor.o averageExecutor.o
	g++ -o DataServer dataServer.o genNumberExecutor.o averageExecutor.o ${lib_flags}

DataHandler: dataHandler.o
	g++ -o DataHandler dataHandler.o ${lib_flags}

genNumberExecutor.o: dataExecutor.h genNumberExecutor.h genNumberExecutor.cpp
	g++ ${CFLAGS} -c genNumberExecutor.cpp

averageExecutor.o: dataExecutor.h averageExecutor.h averageExecutor.cpp
	g++ ${CFLAGS} -c averageExecutor.cpp

dataServer.o: dataServer.cpp dataServer.h
	g++ ${CFLAGS} -c dataServer.cpp

dataHandler.o: dataHandler.h dataHandler.cpp
	g++ ${CFLAGS} -c dataHandler.cpp 


clean: 
	rm *.o DataServer DataHandler
