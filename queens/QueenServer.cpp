#include "QueenServer.h"
#include "Queen.h"

#include "muduo/base/Logging.h"
#include "muduo/net/EventLoop.h"

//#include <google/profiler.h>

#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>


QueenServer::QueenServer(muduo::net::EventLoop* loop,
                         muduo::net::InetAddress& listenAddr)
  : server(loop, listenAddr, "QueenServer"), index(0) {
    server.setConnectionCallback(
        boost::bind(&QueenServer::onConnection, this,  _1));
    server.setMessageCallback(
        boost::bind(&QueenServer::onMessage, this, _1, _2, _3));
}

void QueenServer::start() {
  server.start();
}

void QueenServer::onConnection(const muduo::net::TcpConnectionPtr& conn) {
  if(conn->connected()) {
    LOG_INFO << conn->peerAddress().toIpPort() << " is up";
  }
  else {
    std::string client(conn->peerAddress().toIpPort().c_str());
    if(workers.find(client) != workers.end()) {
      LOG_INFO << "worker: " << client << " is down";
      workers.erase(client);
      activeRequests.erase(client);
    }
    else {
      LOG_INFO << "client: " << client << " is left";
      clients.erase(client);
    }
  }
}

void QueenServer::onMessage(const muduo::net::TcpConnectionPtr& conn,
                            muduo::net::Buffer* buffer,
                            muduo::Timestamp time) {
  while(buffer->findCRLF()) {
    const char* crlf = buffer->findCRLF();
    std::string request(buffer->peek(), crlf);
    buffer->retrieveUntil(crlf + 2);
    processRequest(conn, request); 
  }
}

void QueenServer::processRequest(const muduo::net::TcpConnectionPtr& conn, std::string request) {
  std::string clientId(conn->peerAddress().toIpPort().c_str());
  LOG_INFO << clientId << " receive request: " << request;

  //request <id> <size>
  if(request.find("request") == 0) {   // receive client request
    clients[clientId] = conn;
    size_t first = request.find(" ");
    size_t second = request.find(" ", first + 1);
    std::string taskId(request.begin()+first+1, request.begin()+second);
    int size = std::stoi(std::string(request.begin()+second+1, request.end()));
    bool computeSolutions = (request.find("request-solutions") == 0);
    std::shared_ptr<Request> request(new Request(clientId, taskId, size, computeSolutions));
    LOG_INFO << "requestId " << request->getId();
    requests[request->getId()] = request;
    dispatchTask(request);
  }
  else if(request.find("ready") == 0) {  // receive worker register
    workers[clientId] = conn;
    activeRequests[clientId] = 0;
  }
  else if(request.find("solutions") == 0) { // worker finish a request
    --activeRequests[clientId];
    // solutions <clientId> <taskId> <subId> <number>
    std::vector<std::string> tokens;
    boost::split(tokens, request, boost::is_any_of(" "));
    std::string clientId(tokens[1]);
    std::string taskId(tokens[2]);
    int subtaskId = std::stoi(tokens[3]);
    int number = std::stoi(tokens[4]);
    std::string requestId = clientId + "-" + taskId;
    LOG_INFO << "find solution for " << requestId << " subId: " << subtaskId;
    auto request = requests[requestId];
    request->setSubTaskSolutiosNumber(subtaskId, number);

    if(!request->isComputeSolutions() && request->isTaskFinish())
      sendSolutionToClient(request);
    else if(number == 0 && request->isTaskFinish())
      sendSolutionToClient(request);
  }
  else if(request.find("one_solution") == 0) {  // worker send one solution
    // one_solution <clientId> <taskId> <subId> <solution>
    std::vector<std::string> tokens;
    boost::split(tokens, request, boost::is_any_of(" "));
    std::string clientId(tokens[1]);
    std::string taskId(tokens[2]);
    int subtaskId = std::stoi(tokens[3]);
    std::vector<int> solution;
    for(size_t i = 4; i < tokens.size(); ++i)
      solution.push_back(std::stoi(tokens[i]));
    std::string requestId = clientId + "-" + taskId;
    auto request = requests[requestId];
    requests[requestId]->addSubTaskSolution(subtaskId, solution);
    if(request->isSubTaskFinish(subtaskId) && request->isTaskFinish()) {
      sendSolutionToClient(request);
    }
  } 
 else {
    LOG_INFO << "receive: " << request << " from " << conn->peerAddress().toIpPort();
    conn->send("Bad request\r\n");
    conn->shutdown();
  }
}

void QueenServer::dispatchTask(std::shared_ptr<Request>& request) {
  const std::map<int, std::vector<int>> subtasks = request->splitRequest();
  //[solutions] <clientId> <id> <subid> <size> pos\r\n
  std::string prefix = "";
  if(request->isComputeSolutions())
    prefix = "solutions " + request->getClientId() + " " + request->getTaskId();
  else 
    prefix = request->getClientId() + " " + request->getTaskId();
  for(auto &task : subtasks) {
    std::string line = prefix + " " + std::to_string(task.first) 
                       + " " + std::to_string(request->getRequestSize());
    for(auto &n : task.second) {
      line += " " + std::to_string(n);
    }
    line += "\r\n";

    std::string workerId = findLeastLoadWorker();
    workers[workerId]->send(line);
    ++activeRequests[workerId];
  }
}

std::string QueenServer::findLeastLoadWorker() {
  std::vector<std::string> workers;
  for(auto &pair : activeRequests)
    workers.push_back(pair.first);
  int minIndex = index;
  size_t workerSize = workers.size();
  for(size_t i = 1; i < workerSize; ++i) {
    index = (index + i) % workerSize;   
    if(activeRequests[workers[index]] < activeRequests[workers[minIndex]])
      minIndex = index;
  }

  index = (minIndex + 1) % workerSize;
  return workers[minIndex];
}

void QueenServer::sendSolutionToClient(std::shared_ptr<Request>& request) {
  std::string clientId(request->getClientId());
  std::string requestId(request->getId());
  std::string taskId(request->getTaskId());
  if(clients.find(clientId) != clients.end()) {
    auto client = clients[clientId];
    int solutionNum = request->getSolutionsNumber();
    //solution <id> <solution_number>\r\n
    std::string line = "solution " + taskId + " " + std::to_string(solutionNum) + "\r\n";
    client->send(line);
    //<id> <solution>\r\n
    if(request->isComputeSolutions()) {
      std::list<std::vector<int>> solutions = request->getAllSolutions();
      for(auto &solution : solutions) {
        std::string one_solution = taskId;
        for(auto &n : solution) {
          one_solution += " " + std::to_string(n);
        } 
        one_solution += "\r\n";
        client->send(one_solution);
      }
    }

    requests.erase(requestId);
  }
  else {
    LOG_ERROR << "client " << clientId << " is down, drop all solutions";
    requests.erase(requestId);
  }
}

int main(int argc, char** argv) {
  //std::string program = std::string(argv[0]) + ".prof";
  std::string serverIP = "127.0.0.1";
  int serverPort = 9981;
  if(argc >= 2)
    serverIP = std::string(argv[1]);
  if(argc >= 3)
    serverPort = atoi(argv[2]);
  muduo::net::InetAddress serverAddr(serverIP, serverPort);
  muduo::net::EventLoop loop;

  //ProfilerStart(program.c_str());
  QueenServer server(&loop, serverAddr);
  server.start();

  loop.loop();

  //ProfilerStop();

  return 0;
}
