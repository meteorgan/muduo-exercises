#include "sortExecutor.h"

#include <boost/algorithm/string.hpp>

/**
 * 1. get data number from all workers
 * 2. request batchSize data from all workers
 * 3. wait for data from all workers
 * 4. merge sorted data from all buffers
 * 5. send merged data to one workers
 * 6. request batchSize data from some workers(not finished and buffer threhold is low),
 *    and wait for response
 * 7. return to 4
 */
void SortExecutor::execute() {
    std::string request = "sort " + std::to_string(batchSize) + "\r\n";
    for(auto& conn : connections) {
        conn.second->send(request);
    }

    {
        std::unique_lock<std::mutex> lock(mt);
        total = connections.size();
        workingSize = 0;

        while(workingSize < total) {
            cond.wait(lock);
        }
    }
    int numberOneNode = number / connections.size() + 1;
    while(notFinishedWorkers.size() > 0) {
        mergeNumbers();
        while(outputBuffer.size() >= static_cast<size_t>(batchSize)) {
            std::string message = "sort-results";
            for(int i = 0; i < batchSize; ++i) {
                message += " " + outputBuffer.front();
                outputBuffer.pop_front();
                if(++sendNumber >= numberOneNode)
                    break;
            }
            message += "\r\n";
            std::string id = *currentOutput;
            muduo::net::TcpConnectionPtr& conn = connections[id];
            conn->send(message);
            if(sendNumber >= numberOneNode) {
                std::string end = "sort-results end\r\n";
                conn->send(end);
                ++currentOutput;
                sendNumber = 0;
                
                // all finished
                if(currentOutput == outputBuffer.end()) {
                }
            }
        }
        size_t threhold = batchSize / 2;
        int notWorkingSize = 0;
        std::string requestMore = "sort-more " + std::to_string(batchSize) + "\r\n"; 
        for(auto& worker : notFinishedWorkers) {
            if(workerBuffers[worker].size() < threhold) {
                connections[worker]->send(requestMore);
                ++notWorkingSize;
            }
        }

        {
            std::unique_lock<std::mutex> lock(mt);
            workingSize -= notWorkingSize;
            while(workingSize < total)
                cond.wait(lock);
        }
    }
    // send data left in outputBuffer
}

// merge numbers in all buffers, stop when there is a buffer is empty
// or merge batchSize number
void SortExecutor::mergeNumbers() {
    for(int i = 0; i < batchSize; ++i) {
        std::string target = *notFinishedWorkers.begin();
        int64_t min = std::stol(workerBuffers[target].front());

        for(auto& worker : notFinishedWorkers) {
                int64_t value = std::stol(workerBuffers[worker].front());
                if(value < min) {
                    min = value;
                    target = worker;
                }
        }
        outputBuffer.push_back(workerBuffers[target].front());
        workerBuffers[target].pop_front();
        if(workerBuffers[target].size() == 0)
            break;
    }
}

void SortExecutor::onMessage(const muduo::net::TcpConnectionPtr& conn,
        muduo::net::Buffer* buf, muduo::Timestamp time) {
    while(buf->findCRLF()) {
        const char* crlf = buf->findCRLF();
        std::string response(buf->peek(), crlf);
        buf->retrieveUntil(crlf + 2);
        
        std::string id(conn->peerAddress().toIpPort().c_str());
        bool finished = false;
        std::vector<std::string> tokens;
        boost::split(tokens, response, boost::is_any_of(" "));
        if(tokens[0] == "sort-number") {
            int64_t n = std::stol(tokens[1]);
            number += n;
        }
        else if(tokens[0] == "sort") {
            size_t size = tokens.size();
            if(tokens[size-1] == "end") {     // read all number from this worker
                finished = true;
                size -= 1;
            } 
            for(size_t i = 1; i < size; ++i) {
                workerBuffers[id].push_back(tokens[i]);
            }
        }
        else {
            LOG_ERROR << "sortExecutor receive unknown reponse " << response 
                << " from " << id;
            conn->shutdown();
        }

        {
            std::unique_lock<std::mutex> lock(mt);
            if(finished) {
                --total;
            }
            ++workingSize;
            cond.notify_all();
        }
    }
}
