#include "freqExecutor.h"

#include <boost/algorithm/string.hpp>

const std::function<bool(std::pair<int64_t, int64_t>&, std::pair<int64_t, int64_t>&)> FreqExecutor::compFreq = 
    [](std::pair<int64_t, int64_t>& e1, std::pair<int64_t, int64_t>& e2) {
        return e1.first > e2.first || (e1.first == e2.first && e1.second < e2.second);
    };

std::vector<std::pair<int64_t, int64_t>> FreqExecutor::execute(size_t freqNumber) {
    std::vector<std::pair<int64_t, int64_t>> freqs;

    std::string request = "freq " + std::to_string(batchSize) + "\r\n";
    size_t threhold = batchSize / 2;
    while(notFinishedWorkers.size() != 0) {
        for(const auto& worker : notFinishedWorkers) {
            if(workerBuffers[worker].size() < threhold) {
                connections[worker]->send(request);
                --workingSize;
            }
        }
        {
            size_t total = notFinishedWorkers.size();
            std::unique_lock<std::mutex> lock(mt);
            while(workingSize < total)
                cond.wait(lock);
        }

        mergeFreqs(freqNumber);
    }
    while(topFreqs.size() > 0) {
        const std::pair<int64_t, int64_t>& pair = topFreqs.top();
        freqs.push_back(std::make_pair(pair.second, pair.first));
        topFreqs.pop();
    }

    auto comp = [](std::pair<int64_t, int64_t>& e1, std::pair<int64_t, int64_t>& e2) { 
        return (e1.second > e2.second) || (e1.second == e2.second && e1.first < e2.first); 
    };
    std::sort(freqs.begin(), freqs.end(), comp);

    return freqs;
}

void FreqExecutor::mergeFreqs(size_t freqNumber) {
    while(true) {
        int64_t n = 0;
        int64_t freq = 0;
        std::vector<std::string> targets;
        for(auto& worker : notFinishedWorkers) {
            if(workerBuffers[worker].size() == 0) {
                notFinishedWorkers.erase(worker);
            }
            else {
                targets.push_back(worker);
                n = workerBuffers[worker].front().first;
                freq = workerBuffers[worker].front().second;
                break;
            }
        }
        if(targets.size() == 0)
            break;
        
        for(auto& worker : notFinishedWorkers) {
            if(worker != targets[0]) {
                std::list<std::pair<int64_t, int64_t>>& values = workerBuffers[worker];
                if(values.front().first < n) {
                    targets.clear();
                    targets.push_back(worker);
                    n = values.front().first;
                    freq = values.front().second;
                }
                else if(values.front().first == n) {
                    targets.push_back(worker);
                    freq += values.front().second;
                }
            }
        }

        LOG_INFO << "there is " << topFreqs.size() << " elements in priority queue";
        if(topFreqs.size() < freqNumber) {
            topFreqs.push(std::make_pair(freq, n));
        }
        else if((topFreqs.top().first < freq) 
                || (topFreqs.top().first == freq && topFreqs.top().second > n)) {
            topFreqs.pop();
            topFreqs.push(std::make_pair(freq, n));
        }

        bool can_break = false;
        for(auto& t : targets) {
            workerBuffers[t].pop_front();
            if(workerBuffers[t].size() == 0) {
                can_break =  true;
                if(workerStatus[t]) {
                    notFinishedWorkers.erase(t);
                }
            }
        }
        if(can_break)
            break;
    }
}

void FreqExecutor::onMessage(const muduo::net::TcpConnectionPtr& conn,
        muduo::net::Buffer* buffer, muduo::Timestamp time) {
    while(buffer->findCRLF()) {
        const char* crlf = buffer->findCRLF();
        std::string response(buffer->peek(), crlf);
        buffer->retrieveUntil(crlf + 2);

        std::string peer(conn->peerAddress().toIpPort().c_str());
        std::vector<std::string> tokens;
        boost::split(tokens, response, boost::is_any_of(" "));
        bool finished = false;
        if(tokens[0] == "freq") {
            if(tokens[tokens.size()-1] == "end") {
                finished = true; 
            }
            for(size_t i = 1; i < tokens.size()-1; i+=2) {
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
            ++workingSize;
            cond.notify_all();
        }
    }
}
