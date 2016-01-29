#ifndef DATA_HANDLER_H
#define DATA_HANDLER_H

#include "muduo/net/EventLoop.h"
#include "muduo/net/TcpServer.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <list>

class DataHandler {
    public:
        DataHandler(muduo::net::EventLoop* loop, muduo::net::InetAddress& serverAddr);
        void handleGenNumber(const muduo::net::TcpConnectionPtr&, int64_t, char);
        void handleFreq(const muduo::net::TcpConnectionPtr&, int);
        void handleSort(const muduo::net::TcpConnectionPtr&);
        void handleAverage(const muduo::net::TcpConnectionPtr&);
        void handleSplit(const muduo::net::TcpConnectionPtr&, int64_t);

    private:
        void onConnection(const muduo::net::TcpConnectionPtr& conn);
        void onMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buffer, muduo::Timestamp time);

        void readNumbers(std::ifstream&, std::list<int64_t>&, int);
        std::vector<int64_t> readAllNumbers(const std::string filename);
        std::vector<std::string> splitLargeFile(const std::string&);
        int64_t getFileSize(const std::string filename);

        void genNumbers(int64_t number, char mode);  // normal/uniform/zipf

        void sortFile(const std::string, const std::string);
        void mergeSortedFiles(const std::vector<std::string>&, const std::string&);

        std::pair<int64_t, double> computeAverage();

        void computeFreq();
        void computeFreq(const std::string&, const std::string&);

        void sortFile();

        int partition(std::vector<int64_t>& numbers, int64_t pivot, int64_t start, int64_t end);
        std::vector<int64_t> splitFile(int64_t);

        muduo::net::TcpServer server;
        std::string filename;
        bool sorted;
        bool hasFreq;
        std::ifstream freqFile;
        std::ofstream sortedFile;
        const int fileSizeLimit = 10 * 1024 * 1024;
};

#endifvoid 
