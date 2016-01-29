#include "muduo/base/Logging.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/TcpClient.h"

#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>

#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <list>
#include <set>

class QueenClient {
  public:
    QueenClient(muduo::net::EventLoop* loop, 
                const muduo::net::InetAddress& serverAddr) 
    : client(loop, serverAddr, "queenClient"), seq(0) {
      client.setConnectionCallback(boost::bind(&QueenClient::onConnection, this, _1));
      client.setMessageCallback(boost::bind(&QueenClient::onMessage, this, _1, _2, _3));

      client.connect();
    }


    void sendRequest(int size, bool getSolutions) {
      std::string request(""); 
      if(getSolutions)
        request = "request-solutions " + std::to_string(seq) + " " + std::to_string(size);
      else
        request = "request " + std::to_string(seq) + " " + std::to_string(size);
      request += "\r\n";
      seq++;
      conn_->send(request); 

      LOG_INFO << "send request " << request.substr(0, request.size()-2)  << " to server";
    }

  private:
    void onConnection(const muduo::net::TcpConnectionPtr& conn) {
      if(conn->connected()) {
        conn_ = conn;
        sendRequest(14, false);
      }
      else {
        LOG_INFO << conn->peerAddress().toIpPort() << " is down";
        conn->shutdown();
      }
    }

    void onMessage(const muduo::net::TcpConnectionPtr& conn,
                   muduo::net::Buffer* buffer, muduo::Timestamp time) {
      while(buffer->findCRLF()) {
        const char* crlf = buffer->findCRLF();
        std::string line(buffer->peek(), crlf);
        //LOG_INFO << "client receive: " << line;
        buffer->retrieveUntil(crlf+2);

        // solution <id> <solutionNumber>
        if(line.find("solution") == 0) {
          std::vector<std::string> tokens;
          boost::split(tokens, line, boost::is_any_of(" "));
          int taskId = std::stoi(tokens[1]);
          int number = std::stoi(tokens[2]);
          solutionNumber[taskId] = number;
          LOG_INFO << "find " << number << " solution for id: " << taskId;
        }
        else {
          //<id> <solution>
          std::vector<std::string> tokens;
          boost::split(tokens, line, boost::is_any_of(" "));
          int taskId = std::stoi(tokens[0]);
          std::vector<int> solution;
          for(size_t i = 1; i < tokens.size(); ++i) {
            solution.push_back(std::stoi(tokens[i]));
          }
          solutions[taskId].push_back(solution);
        }
      }
    }

    muduo::net::TcpClient client; 
    int seq;
    muduo::net::TcpConnectionPtr conn_;
    std::map<int, int> solutionNumber;
    std::map<int, std::list<std::vector<int>>> solutions;
};

int main(int argc, char** argv) {
  std::string serverIP = "127.0.0.1";
  int serverPort = 9981;
  if(argc >= 2)
    serverIP = std::string(argv[1]);
  if(argc >= 3)
    serverPort = atoi(argv[2]);
  muduo::net::InetAddress serverAddr(serverIP, serverPort);
  muduo::net::EventLoop loop;
  QueenClient client(&loop, serverAddr);

  loop.loop();

  return 0;
}
