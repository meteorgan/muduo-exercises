#ifndef DATA_HANDLER_H
#define DATA_HANDLER_H

#include "muduo/net/TcpServer.h"

#include <iostream>
#include <vector>
#include <list>

class DataHandler {
    public:
        DataHandler(muduo::net::EventLoop* loop, muduo::net::InetAddress& serverAddr);

        double computeAverage();

        void computeFreq();

        void sortFile();

    private:
        void onConnection(const muduo::net::TcpConnectionPtr& conn);
        void onMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buffer, muduo::Timestamp time);

        int64_t getFileSize(const char* filename);
        void genNumbers(int64_t number, char mode);  // normal/uniform/zipf
        void sortFile(const std::string, const std::string);
        void mergeSortedFiles(const std::vector<std::string>&, const std::string&);
        void readNumbers(std::ifstream&, std::list<int64_t>&, int);
        std::vector<std::string> splitLargeFile(const char* filename);
        void computeFreq(const std::string&, const std::string&);

        muduo::net::TcpServer server;
        std::string filename;
};

#endif
