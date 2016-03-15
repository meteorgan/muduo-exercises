CFLAGS = -std=c++11 -Wall ${include_dir}
include_dir = -I /home/meteorgan/muduo
lib_flags = -L /home/meteorgan/build/release/lib -lmuduo_base_cpp11 -lmuduo_net_cpp11 -pthread


Memcached: session.o item.o memcached.o
	g++ -o Memcached memcached.o item.o session.o ${lib_flags}

memcached.o: memcached.h memcached.cpp
	g++ ${CFLAGS} -c memcached.cpp 

session.o: session.h session.cpp
	g++ ${CFLAGS} -c session.cpp

item.o: item.h item.cpp
	g++ ${CFLAGS} -c item.cpp

clean:
	rm Memcached *.o
