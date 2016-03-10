ClientTest: clientTest.o memcachedClient.o
	g++ -o ClientTest clientTest.o memcachedClient.o -lboost_unit_test_framework

memcachedClient.o: memcachedClient.h memcachedClient.cpp
	g++ -std=c++11 -Wall -c memcachedClient.cpp

clientTest.o: memcachedClient.h memcachedClient.cpp clientTest.cpp
	g++ -std=c++11 -Wall -c clientTest.cpp

clean:
	rm *.o ClientTest
