#include "muduo/base/Logging.h"
#include "muduo/net/EventLoop.h"
#include "muduo/base/ThreadPool.h"
#include "muduo/net/TcpClient.h"

#include "Queen.h"
#include "QueenTask.h"

#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>

#include <vector>

class QueenWorker {
  public:
    QueenWorker(muduo::net::EventLoop* loop, muduo::net::InetAddress& serverAddr,
                int numThread)
    : client(loop, serverAddr, "worker"), numThread(numThread), threadPool() {
      client.setConnectionCallback(boost::bind(&QueenWorker::onConnection, this, _1));
      client.setMessageCallback(boost::bind(&QueenWorker::onMessage, this, _1, _2, _3));
    }

    void start() {
      LOG_INFO << "worker use " << numThread << " compute thread";
      threadPool.start(numThread);
      client.connect();
    }

  private:
    void onConnection(const muduo::net::TcpConnectionPtr& conn) {
      if(conn->connected()) {
        LOG_INFO << "connect to " << conn->peerAddress().toIpPort();
        conn->setTcpNoDelay(true);
        conn->send("ready\r\n");
      }
      else {
        LOG_INFO << conn->peerAddress().toIpPort() << " is down";
        client.disconnect();
      }
    }

    void onMessage(const muduo::net::TcpConnectionPtr& conn, 
                   muduo::net::Buffer* buffer, muduo::Timestamp time) {
      //[solutions] <clientId> <id> <subid> <size> <pos>\r\n
      while(buffer->findCRLF()) {
        const char* crlf = buffer->findCRLF();
        std::string request(buffer->peek(), crlf);
        buffer->retrieveUntil(crlf+2);
        LOG_INFO << "worker receive " << request;

        bool computeSolutions = (request.find("solutions") == 0);
        if(computeSolutions)
          request = request.substr(request.find("solutions"), request.size());
        std::vector<std::string> tokens;
        boost::split(tokens, request, boost::is_any_of(" "));
        if(tokens.size() >= 4) {
          int size = std::stoi(tokens[3]);
          std::vector<int> pos;
          for(size_t i = 4; i < tokens.size(); ++i)
            pos.push_back(std::stoi(tokens[i]));

          QueenTask task(tokens[0], tokens[1], tokens[2], size, pos, computeSolutions);
          threadPool.run(boost::bind(&QueenWorker::processRequest, this, conn, task));
        }
        else {
          LOG_ERROR << "receive bad request " << request << " from " << conn->peerAddress().toIpPort();
        }
      }
    }      

    void processRequest(const muduo::net::TcpConnectionPtr& conn, const QueenTask& task) {
      std::list<std::vector<int>> queens;
      if(task.getPos().size() == 0) {
        queens = solve_queens(task.getSize());
      }
      else {
        queens = complete_queens(task.getPos(), task.getSize());
      }
      int number = queens.size();
      // solutions <clientId> <id> <subid> <number>\r\n
      std::string prefix = task.getClientId() + " " + task.getTaskId() + " " + task.getSubTaskId();
      std::string res = "solutions " + prefix + " " + std::to_string(number) + "\r\n";
      conn->send(res);
      LOG_INFO << "send " << res.substr(0, res.size()-2) << " to server";
      if(task.isComputeSolutions()) {
        for(auto &queen : queens) {
          // one_solution <client_id> <id> <subid> <solution>\r\n
          std::string line = "one_solution " + prefix;
          for(auto &n : queen) {
            line += " " + std::to_string(n);
          }
          line += "\r\n";
          conn->send(line);
        }
      }
    }

    muduo::net::TcpClient client;
    int numThread;
    muduo::ThreadPool threadPool;
};

int main(int argc, char** argv) {
  std::string serverIp = "127.0.0.1";
  int serverPort = 9981;
  int numberThread = 4;
  if(argc >= 3) {
    serverIp = std::string(argv[1]);
    serverPort = atoi(argv[2]);
  }
  if(argc >= 4) 
    numberThread = atoi(argv[3]);

  muduo::net::EventLoop loop;
  muduo::net::InetAddress serverAddr(serverIp, serverPort);
  QueenWorker worker(&loop, serverAddr, numberThread);
  worker.start();
  loop.loop();

  return 0;
}
