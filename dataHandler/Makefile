CFLAGS = -std=c++11 -Wall -g ${include_dir}
include_dir = -I /home/meteorgan/muduo
lib_flags = -L /home/meteorgan/build/release/lib -lmuduo_base_cpp11 -lmuduo_net_cpp11 -pthread

all: DataServer DataHandler 

DataServer: dataServer.o genNumberExecutor.o averageExecutor.o sortExecutor.o \
	medianExecutor.o freqExecutor.o
	g++ -o DataServer dataServer.o genNumberExecutor.o averageExecutor.o \
		sortExecutor.o medianExecutor.o freqExecutor.o ${lib_flags}

DataHandler: dataHandler.o
	g++ -o DataHandler dataHandler.o ${lib_flags}

genNumberExecutor.o: dataExecutor.h genNumberExecutor.h genNumberExecutor.cpp
	g++ ${CFLAGS} -c genNumberExecutor.cpp

averageExecutor.o: dataExecutor.h averageExecutor.h averageExecutor.cpp
	g++ ${CFLAGS} -c averageExecutor.cpp

sortExecutor.o: dataExecutor.h sortExecutor.h sortExecutor.cpp
	g++ ${CFLAGS} -c sortExecutor.cpp

medianExecutor.o : dataExecutor.h medianExecutor.h medianExecutor.cpp
	g++ ${CFLAGS} -c medianExecutor.cpp

freqExecutor.o: dataExecutor.h freqExecutor.h freqExecutor.cpp
	g++ ${CFLAGS} -c freqExecutor.cpp

dataServer.o: dataServer.cpp dataServer.h
	g++ ${CFLAGS} -c dataServer.cpp

dataHandler.o: dataHandler.h dataHandler.cpp
	g++ ${CFLAGS} -c dataHandler.cpp 


clean: 
	rm *.o DataServer DataHandler
