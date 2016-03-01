#include "medianExecutor.h"

#include <boost/algorithm/string.hpp>

int64_t MedianExecutor::execute() {
    int64_t median = 0;
    std::string random = "random\r\n";
    for(auto& conn : connections) {
        conn.second->send(random);
        --workingSize;
    }
    {
        size_t total = connections.size();
        std::unique_lock<std::mutex> lock(mt);
        while(workingSize < total)
            cond.wait(lock);
    }
    int totalNumber = 0;
    std::vector<int64_t> randomValues;
    for(auto& worker : workerBuffers) {
        assert(worker.second.size() == 2);

        randomValues.push_back(worker.second[0]);
        totalNumber += worker.second[1];
        worker.second.clear();
    }
    std::sort(randomValues.begin(), randomValues.end());
    int64_t splitPoint = randomValues[randomValues.size()/2];
    int64_t size = totalNumber / 2 + 1;
    while(true) {
        std::string request = "split " + std::to_string(splitPoint) + "\r\n";
        for(auto& conn : connections) {
            conn.second->send(request);
            --workingSize;
        }
        {
            size_t total = connections.size();
            std::unique_lock<std::mutex> lock(mt);
            while(workingSize < total)
                cond.wait(lock);
        }
        
        int64_t moreCount= 0;
        int64_t lessOrEqualCount = 0;
        std::vector<int64_t> lessOrEqualNumbers;
        std::vector<int64_t> moreNumbers;
        for(auto& worker : workerBuffers) {
            std::vector<int64_t>& values = worker.second;
            assert(values.size() == 4);

            int64_t lessOrEqual = values[0];
            if(lessOrEqual > 0) {
                lessOrEqualNumbers.push_back(values[1]);
                lessOrEqualCount += lessOrEqual;
            }
            int64_t more = values[2];
            if(more > 0) {
                moreNumbers.push_back(values[3]);
                moreCount += more;
            }
            values.clear();
        }
        if(lessOrEqualCount == size) {
            median = splitPoint;
            break;
        }
        else if(lessOrEqualCount > size) {
            std::sort(lessOrEqualNumbers.begin(), lessOrEqualNumbers.end());
            splitPoint = lessOrEqualNumbers[lessOrEqualNumbers.size()/2];
        }
        else {
            size -= lessOrEqualCount;
            std::sort(moreNumbers.begin(), moreNumbers.end());
            splitPoint = moreNumbers[moreNumbers.size()/2];
        }
    }

    for(auto& conn : connections)
        conn.second->send("split end\r\n");

    return median;
}

void MedianExecutor::onMessage(const muduo::net::TcpConnectionPtr& conn, 
        muduo::net::Buffer* buffer, muduo::Timestamp time) {
    while(buffer->findCRLF()) {
        const char* crlf = buffer->findCRLF();
        std::string response(buffer->peek(), crlf);
        buffer->retrieveUntil(crlf+2);

        std::string id(conn->peerAddress().toIpPort().c_str());
        LOG_INFO <<  "MedianExecutor receive: [" << response << "] from " << id;

        std::vector<std::string> tokens;
        boost::split(tokens, response, boost::is_any_of(" "));
        if(tokens[0] == "random" || tokens[0] == "split") {
            for(size_t i = 1; i < tokens.size(); ++i) {
                int64_t n = std::stol(tokens[i]);
                workerBuffers[id].push_back(n);
            }
        }
        else {
            LOG_ERROR << "MedianExecutor receive unknown response: [" << response << "] from " << id;
            conn->shutdown();
        }

        {
            std::unique_lock<std::mutex> lock(mt);
            ++workingSize;
            cond.notify_all();
        }
    }
}
