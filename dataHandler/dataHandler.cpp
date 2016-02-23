#include "dataHandler.h"

#include "muduo/base/Logging.h"

#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>

#include <sys/stat.h>
#include <unistd.h>

#include <list>
#include <fstream>

DataHandler::DataHandler(muduo::net::EventLoop* loop, muduo::net::InetAddress& serverAddr)
    : server(loop, serverAddr, "dataHandler"), filename(""), fileNumber(0),
    sorted(false), hasFreq(false), lastPivot(0), lessFile(""), largeFile("") {
    server.setConnectionCallback(boost::bind(&DataHandler::onConnection, this, _1));
    server.setMessageCallback(boost::bind(&DataHandler::onMessage, this, _1, _2, _3));
}

void DataHandler::start() {
    server.start();
}

void DataHandler::onConnection(const muduo::net::TcpConnectionPtr& conn) {
    std::string client(conn->peerAddress().toIpPort().c_str());
    LOG_INFO << "client " << client << (conn->connected() ? " UP" : " DOWN");
}

void DataHandler::onMessage(const muduo::net::TcpConnectionPtr& conn,
                            muduo::net::Buffer* buffer, muduo::Timestamp time) {
    while(buffer->findCRLF()) {
        const char* crlf = buffer->findCRLF();
        std::string request(buffer->peek(), crlf);
        buffer->retrieveUntil(crlf+2);
        LOG_INFO << "dataHandler receive " << request;

        std::vector<std::string> tokens;
        boost::split(tokens, request, boost::is_any_of(" "));
        if(request.find("gen_numbers") == 0) {       //gen_numbers <number> <mode>
           fileNumber = std::stol(tokens[1]); 
           char mode = tokens[2][0];
           handleGenNumber(conn, fileNumber, mode);
        }
        else if(request.find("sort") == 0) {         
            // sort-results <n1> <n2> ... <n>\r\n
            // sort-results end\r\n
            if(!sortedFile.is_open())
                sortedFile.open(filename + "-sorted");
            if(request.find("sort-results") == 0) {  
                if(tokens[1] == "end")
                    sortedFile.close();
                else {
                    for(size_t i = 1; i < tokens.size(); ++i)
                        sortedFile << tokens[i] << "\n";
                }
            }
            else {                                   // sort
                handleSort(conn);
            }
        }
        else if(request.find("average") == 0) {      // average
            handleAverage(conn);
        }
        else if(request.find("most-freq") == 0) {   // most-freq <number>
            int number = std::stoi(tokens[1]);
            handleFreq(conn, number);
        }
        else if(request.find("split") == 0) {       // split <number>
            int64_t number = std::stol(tokens[1]);
            handleSplit(conn, number);
        }
        else if(request.find("random") == 0) {      // random
            std::ifstream ifs(filename);
            std::vector<int> numbers;
            int size = 100;
            int64_t n;
            while(--size >= 0 && ifs >> n)
                numbers.push_back(n);
            std::sort(numbers.begin(), numbers.end());
            int64_t target = numbers[numbers.size()/2]; 
            std::string line = "random " + std::to_string(target) + "\r\n";
            conn->send(line);
        }
        else {
            LOG_ERROR << "receive bad request: " << request;
            conn->shutdown();
        }
    }
}

void DataHandler::handleGenNumber(const muduo::net::TcpConnectionPtr& conn, int64_t number, char mode) {
    sorted = hasFreq = false;
    filename = std::string(conn->localAddress().toIpPort().c_str()) + "-" 
        + std::to_string(getpid());
    genNumbers(number, mode);
    conn->send("gen_num\r\n");
}

// most-freq <n1, freq1> <n2, freq2> ... <n, freq>\r\n
// ...
// most-freq end\r\n
void DataHandler::handleFreq(const muduo::net::TcpConnectionPtr& conn, int number) {
    if(!hasFreq) {
        computeFreq();
        hasFreq = true;
    }
    if(!freqFile.is_open()) {
        freqFile.open(filename);
    }

    const int batchSize = 100;
    const std::string prefix("most-freq ");
    while(number-- > 0 && !freqFile.eof()) {
        std::string line(prefix);
        int i = 0;
        int64_t n;
        while(++i < batchSize && freqFile >> n) {
            line += " " + std::to_string(n);
        }
        line += "\r\n";
        conn->send(line);
    }
    if(freqFile.eof()) {
        freqFile.close();
        conn->send(prefix + "end\r\n");
    }
}

// sort <n1> <n2> ... <n>\r\n
// ....
// sort end\r\n
void DataHandler::handleSort(const muduo::net::TcpConnectionPtr& conn) {
    sortFile();
    const int batchSize = 100;
    std::string prefix = "sort ";
    std::ifstream ifs(filename + "-sort");
    int i = 0;
    std::string line = prefix;
    int64_t n;
    while(ifs >> n) {
       line += " " + std::to_string(n);
       if(++i == batchSize) {
           line += "\r\n";
           conn->send(line);
           line = prefix;
           i = 0;
       }
    }
    if(i != 0) {
        line += "\r\n";
        conn->send(line);
    }
    conn->send("sort end\r\n");
}

// average <number> <sum>\r\n
void DataHandler::handleAverage(const muduo::net::TcpConnectionPtr& conn) {
    int64_t sum = computeSum();
    std::string line = "average " + std::to_string(fileNumber) 
                    + " " + std::to_string(sum) + "\r\n";
    conn->send(line);
}

std::pair<int64_t, double> DataHandler::computeAverage() {
    int64_t sum = 0L;
    int64_t number = 0L;

    std::ifstream infile(filename);
    int64_t n;
    while(infile >> n) {
        number++;
        sum += n;
    }
    infile.close();

    return std::make_pair(number, double(sum) / number);
}

int64_t DataHandler::computeSum() {
    int64_t sum = 0L;

    std::ifstream infile(filename);
    int64_t n;
    while(infile >> n) {
        sum += n;
    }

    return sum;
}

int64_t DataHandler::getFileSize(const std::string filename) {
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0? stat_buf.st_size : -1;
}

void DataHandler::genNumbers(int64_t number, char mode) {
    std::ofstream ofs(filename, std::ofstream::out);

    std::random_device rd;
    std::mt19937 gen(rd());
    switch(mode) {
        case 'n': { // normal
                      std::normal_distribution<float> dis(RAND_MAX/8192, RAND_MAX/1024);
                      for(int64_t i = 0; i < number; ++i) {
                          int64_t n = static_cast<int64_t>(dis(gen));
                          ofs << n << "\n";
                      }
                      break;
                  }
        case 'u': { // uniform
                      std::uniform_int_distribution<> dis(0, RAND_MAX);
                      for(int64_t i = 0; i < number; ++i) {
                          ofs << dis(gen) << "\n";
                      }
                      break;
                  }
        case 'z': { // zipf
                      std::uniform_int_distribution<> dis(0, RAND_MAX);
                      int index = 1;
                      int64_t left = number;
                      while(left > 0) {
                          int64_t freq = (0.1/index) * number;
                          if(freq == 0)
                              freq = 1;
                          int64_t n = dis(gen);
                          while(freq-- > 0 && left-- > 0) {
                              ofs << n << "\n";
                          }
                          index++;
                      }
                      break;
                  }
        default: {
                     LOG_ERROR << "can not generate file for mode: " << mode;
                     break;
                 }
    }
}

std::vector<std::string> DataHandler::splitLargeFile(const std::string& filename) {
    std::string name(filename);
    std::vector<std::string> files;
    std::ofstream outfiles[10];
    for(int i = 0; i < 10; ++i) {
        std::string n =  name + std::to_string(i);
        files.push_back(n);
        outfiles[i].open(n);
    }

    std::ifstream infile(filename);
    int64_t n;
    while(infile >> n) {
        int mod = n % 10;
        outfiles[mod] << n << "\n";
    }

    return files;
}

void DataHandler::computeFreq(const std::string& input_file, const std::string& output_file) {
    std::map<int64_t, int64_t> freqs;
    std::ifstream infile(input_file);
    int64_t n;
    while(infile >> n) {
        freqs[n]++;
    }

    std::ofstream ofs(output_file, std::ostream::out|std::ofstream::app);
    for(const auto &pair :  freqs) {
        ofs << pair.first << " " << pair.second << "\n";
    } 
}

void DataHandler::computeFreq() {
    int64_t fileSize = getFileSize(filename);
    if(fileSize < fileSizeLimit) {
        std::string name_str = std::string(filename);
        computeFreq(name_str, name_str + "-freq");
    }
    else {
        sortFile();
        std::ifstream ifs(filename + "-sort");
        std::ofstream ofs(filename + "-freq");
        int64_t currentNum = 0;
        int64_t currentFreq = 0;
        int64_t n;
        while(ifs >> n) {
            if(currentNum != n) {
                if(currentFreq != 0)
                    ofs << currentNum << " " << currentFreq << "\n";
                currentNum = n;
                currentFreq = 0;
            }
            ++currentFreq;
        }
    }
}

void DataHandler::readNumbers(std::ifstream &ifs, std::list<int64_t>& numbers, int size) {
    int64_t n;
    numbers.clear();
    while(size-- > 0 && ifs >> n) {
        numbers.push_back(n);
    }
}

std::vector<int64_t> DataHandler::readAllNumbers(const std::string file) {
    std::vector<int64_t> numbers;
    std::ifstream ifs(file);
    int64_t n;
    while(ifs >> n)
        numbers.push_back(n);

    return numbers;
}

void DataHandler::mergeSortedFiles(const std::vector<std::string>& files, const std::string& output) {
    const int read_size = 1000000;
    size_t size = files.size();
    std::vector<std::list<int64_t>> buffers(size);
    std::vector<bool> empty(size, false);
    std::ifstream *sorted_files = new std::ifstream[size];
    for(size_t i = 0; i < size; ++i) {
        sorted_files[i].open(files[i]);
        readNumbers(sorted_files[i], buffers[i], read_size);
        if(buffers[i].size() < read_size)
            empty[i] = true;
    }

    std::ofstream ofs(output);
    bool all_empty = false;
    while(!all_empty) {
        int index = -1;
        all_empty = true;
        for(size_t i = 0; i < size; ++i) {
            if(buffers[i].empty() && !empty[i]) {
                readNumbers(sorted_files[i], buffers[i], read_size);
                if(buffers[i].size() < read_size)
                    empty[i] = true;
            }
            if(!buffers[i].empty()) {
                all_empty = false;
                if(index == -1)
                    index = i;
                if(buffers[i].front() <= buffers[index].front())
                    index = i;
            }
        }
        if(!all_empty) {
            ofs << buffers[index].front() << "\n";
            buffers[index].pop_front();
        }
    }

    delete[] sorted_files;
}

void DataHandler::sortFile(const std::string input, const std::string output) {
    std::vector<int64_t> numbers = readAllNumbers(input);
    std::sort(numbers.begin(), numbers.end());

    std::ofstream ofs(output, std::ostream::out|std::ostream::app);
    for(auto n : numbers)
        ofs << n << "\n";
}

//TODO: recursive handle larger file
void DataHandler::sortFile() {
    if(!sorted) {
        int64_t fileSize = getFileSize(filename);
        assert(fileSize != -1);

        if(fileSize <= fileSizeLimit) {
            sortFile(filename, filename + "-sort");
        }
        else {
            std::vector<std::string> files = splitLargeFile(filename);
            std::vector<std::string> sorted_files;
            for(size_t i = 0; i < files.size(); ++i) {
                std::string f = files[i] + "-sort";
                sortFile(files[i], f);
                sorted_files.push_back(f);
            }
            mergeSortedFiles(sorted_files, filename + "-sort");
            std::for_each(files.begin(), files.end(), [](std::string file) { std::remove(file.c_str()); });
            std::for_each(sorted_files.begin(), sorted_files.end(), [](std::string file) { std::remove(file.c_str()); });
        }
        sorted = true;
    }
}

// split lessNumber one-less more-Number one-more\r\n
void DataHandler::handleSplit(const muduo::net::TcpConnectionPtr& conn, int64_t number) {
    std::vector<int64_t> numbers = splitFile(number);
    std::string data("");
    for(auto &n : numbers)
        data += " " + std::to_string(n);
    conn->send("split " + data + "\r\n");
}

// remove all temp files
std::vector<int64_t> DataHandler::splitFile(int64_t pivot) {
    std::vector<int64_t> results;

    std::string file = (lessFile != "") ? (lastPivot > pivot ? lessFile : largeFile) : filename;
    int64_t fileSize = getFileSize(file);
    if(fileSize <= fileSizeLimit) {
        std::vector<int64_t> numbers = readAllNumbers(filename);
        int index = partition(numbers, pivot, 0, numbers.size()-1);
        int lessNumber = index + 1;
        int64_t oneLess = numbers[index/2];
        int largeNumber = numbers.size() - index - 1;
        int64_t oneLarge= numbers[index+1+largeNumber/2];
        results.push_back(lessNumber);
        results.push_back(oneLess);
        results.push_back(largeNumber);
        results.push_back(oneLarge);
    }
    else {
        std::string prefix = filename + "-" + std::to_string(pivot) + "-";
        std::string newLessFile = prefix + "-less";
        std::string newLargeFile = prefix + "-large";
        std::ifstream ifs(file);
        std::ofstream less(newLessFile);
        std::ofstream large(newLargeFile);
        int64_t lessNumber = 0;
        int64_t oneLess = 0;
        int64_t largeNumber = 0;
        int64_t oneLarge = 0;
        int64_t n;
        while(ifs >> n) {
            if(n <= pivot) {
                less << n << "\n";
                ++lessNumber;
                oneLess = n;
            }
            else  {
                large << n << "\n";
                ++largeNumber;
                oneLarge = n;
            }
        }
        results.push_back(lessNumber);
        results.push_back(oneLess);
        results.push_back(largeNumber);
        results.push_back(oneLarge);

        if(file != filename) {
            std::remove(lessFile.c_str());
            std::remove(largeFile.c_str());
        }
        lessFile = newLessFile;
        largeFile = newLargeFile;
    }
    lastPivot = pivot;

    return results;
}

int DataHandler::partition(std::vector<int64_t>& numbers, int64_t pivot, int64_t start, int64_t end) {
    int64_t s = start;
    while(start < end) {
        while(start <= end && numbers[start] <= pivot)
            ++start;
        while(end >= start && numbers[end] > pivot)
            --end;
        if(start < end) {
            int64_t temp = numbers[start];
            numbers[start] = numbers[end];
            numbers[end] = temp;
        }
    }
    if(s <= end)
        return end;
    else
        return s;
}

int main(int argc, char** argv) {
    std::string serverIP = "127.0.0.1";
    int port = 9981;
    if(argc >= 2)
        serverIP = std::string(argv[1]);
    if(argc >= 3)
        port = atoi(argv[2]);

    muduo::net::EventLoop loop;
    muduo::net::InetAddress serverAddr(serverIP, port);
    DataHandler handler(&loop, serverAddr);
    handler.start();

    loop.loop();

    return 0;
}
