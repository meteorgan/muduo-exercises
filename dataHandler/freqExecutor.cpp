#include "freqExecutor.h"

#include <boost/algorithm/string.hpp>

std::vector<std::pair<int64_t, int64_t>> FreqExecutor::execute() {
    std::vector<std::pair<int64_t, int64_t>> freqs;

    std::string request = "sort " + std::to_string(batchSize);
    size_t threhold = batchSize / 2;
    while(notFinishedWorkers.size() != 0) {
        for(const auto& worker : notFinishedWorkers) {
            if(workerBuffers[worker].size() < threhold) {
                connections[worker]->send(request);
                --notWorkingSize;
            }
        }
        {
            size_t total = notFinishedWorkers.size();
            std::unique_lock<std::mutex> lock(mt);
            while(notWorkingSize < total)
                cond.wait(lock);
        }

        mergeFreqs();
    }

    return freqs;
}

void FreqExecutor::mergeFreqs() {
}

void FreqExecutor::onMessage(const muduo::net::TcpConnectionPtr& conn,
        muduo::net::Buffer* buffer, muduo::Timestamp time) {
    while(buffer->findCRLF()) {
        const char* crlf = buffer->findCRLF();
        std::string response(buffer->peek(), crlf);
        buffer->retrieveUntil(crlf + 2);

        std::string peer(conn->peerAddress().toIpPort().c_str());
        std::vector<std::string> tokens;
        boost::split(response, tokens, boost::is_any_of(" "));
        bool finished = false;
        if(tokens[0] == "freq") {
            if(tokens[tokens.size()-1] == "end") {
                finished = true; 
            }
            for(size_t i = 1; i < tokens.size(); i+=2) {
                int64_t n = std::stol(tokens[i]);
                int64_t freq = std::stol(tokens[i+1]);
                workerBuffers[peer].push_back(std::make_pair(n, freq));
            }
        }
        else {
            LOG_ERROR << "freq executor receive unknown response: " << response;
            conn->shutdown();
        }

        {
            std::unique_lock<std::mutex> lock(mt);
            if(finished)
                workerStatus[peer] = true;
            ++notWorkingSize;
        }
    }
}
