#ifndef MEMCACHED_SESSION_H
#define MEMCACHED_SESSION_H

#include "memcached.h"

#include <boost/bind.hpp>

class Session {
    public:
        Session(Memcached* memServer, muduo::net::TcpConnectionPtr& conn) 
            :server(memServer), currentCommand(""), flags(0), exptime(0), 
            bytesToRead(0), cas(0) {
                conn->setMessageCallback(boost::bind(&Session::onMessage, this, _1, _2, _3));
        }

    private:
        void onMessage(const muduo::net::TcpConnectionPtr& conn,
                muduo::net::Buffer* buffer, muduo::Timestamp time);

        bool isNumber(const std::string& str);
        bool isAllNumber(std::vector<std::string>::const_iterator begin, 
                std::vector<std::string>::const_iterator end);

        bool validateStorageCommand(const std::vector<std::string>& tokens, size_t size, const muduo::net::TcpConnectionPtr& conn);

        const std::string nonExistentCommand = "ERROR\r\n";
        const std::string badFormat = "CLIENT_ERROR bad command line format\r\n";
        const std::string nonNumeric = "CLIENT_ERROR cannot increment or decrement non-numeric value\r\n";
        const std::string invalidExptime = "CLIENT_ERROR invalid exptime argument\r\n";
        const std::string stored = "STORED\r\n";
        const std::string notStored = "NOT_STORED\r\n";
        const std::string exists = "EXISTS\r\n";
        const std::string notFound = "NOT_FOUND\r\n";
        const std::string end = "END\r\n";
        const std::string deleted = "DELETED\r\n";
        const std::string touched = "TOUCHED\r\n";

        Memcached* server;

        std::string currentCommand;
        uint16_t flags;
        uint32_t exptime;
        uint32_t bytesToRead;
        uint64_t cas;
};

#endif
