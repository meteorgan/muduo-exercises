CFLAGS = -std=c++11 ${include_dir}
include_dir = -I /home/meteorgan/workspace/CPlusPlus/muduo
lib_flags = -L /home/meteorgan/workspace/CPlusPlus/build/release/lib -lmuduo_base_cpp11 -lmuduo_net_cpp11 -pthread

all: QueenClient QueenServer QueenWorker

QueenClient: QueenClient.cpp
	g++ ${CFLAGS} -o QueenClient QueenClient.cpp ${lib_flags}

QueenServer: QueenServer.o Request.o Queen.o
	g++ -o QueenServer QueenServer.o Request.o Queen.o ${lib_flags}

QueenWorker: QueenWorker.o Queen.o
	g++ -o QueenWorker QueenWorker.o Queen.o ${lib_flags}

Request.o: Request.cpp Request.h
	g++ ${CFLAGS} -c Request.cpp 

Queen.o: Queen.cpp Queen.h
	g++ ${CFLAGS} -c Queen.cpp

QueenServer.o: QueenServer.cpp QueenServer.h
	g++ ${CFLAGS} -c QueenServer.cpp

QueenWorker.o: QueenWorker.cpp QueenTask.h
	g++ ${CFLAGS} -c QueenWorker.cpp

clean: 
	rm *.o QueenClient QueenServer QueenWorker
