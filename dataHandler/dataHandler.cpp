#include "dataHandler.h"

#include "muduo/base/Logging.h"

#include <sys/stat.h>

#include <list>
#include <fstream>

DataHandler::DataHandler(muduo::net::EventLoop* loop, muduo::net::InetAddress& serverAddr)
    : server(loop, serverAddr, "dataHandler"), filename("") {
}

void DataHandler::onConnection(const muduo::net::TcpConnectionPtr& conn) {
}

void DataHandler::onMessage(const muduo::net::TcpConnectionPtr& conn,
                            muduo::net::Buffer* buffer, muduo::Timestamp time) {
}

double DataHandler::computeAverage() {
   int64_t sum = 0L;
    int64_t number = 0L;

    std::ifstream infile(filename);
    int64_t n;
    while(infile >> n) {
        number++;
        sum += n;
    }
    infile.close();

    return double(sum) / number;
}

int64_t DataHandler::getFileSize(const char* filename) {
    struct stat stat_buf;
    int rc = stat(filename, &stat_buf);
    return rc == 0? stat_buf.st_size : -1;
}

void DataHandler::genNumbers(int64_t number, char mode) {
    std::ofstream ofs(filename, std::ofstream::out);

    std::random_device rd;
    std::mt19937 gen(rd());
    switch(mode) {
        case 'n': { // normal
                      std::normal_distribution<float> dis(RAND_MAX/2, RAND_MAX/1024);
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

std::vector<std::string> DataHandler::splitLargeFile(const char* filename) {
    std::string name(filename);
    std::vector<std::string> files;
    std::ofstream outfiles[10];
    for(int i = 0; i < 10; ++i) {
        std::string n =  name + std::to_string(i);
        files.push_back(n);
        outfiles[i] = std::ofstream(n);
    }

    std::ifstream infile(filename);
    int64_t n;
    while(infile >> n) {
        int mod = n % 10;
        outfiles[mod] << n << "\n";
    }

    return files;
}

void DataHandler::computeFreq() {
    int64_t file_size = getFileSize(filename.c_str());
    assert(file_size != -1);
    if(file_size < 1024 * 1024 * 1024) {
        std::string name_str = std::string(filename);
        compute_frequence(name_str, name_str + "-freq");
    }
    else {
        std::vector<std::string> files = splitLargeFile(filename);
        for(const auto &file : files) {
            compute_frequence(file, std::string(filename)+"-freq");
            std::remove(file.c_str());
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

void DataHandler::mergeSortedFiles(const std::vector<std::string>& files, const std::string& output) {
    const int read_size = 1000000;
    size_t size = files.size();
    std::vector<std::list<int64_t>> buffers(size);
    std::vector<bool> empty(size, false);
    std::ifstream *sorted_files = new std::ifstream[size];
    for(size_t i = 0; i < size; ++i) {
        sorted_files[i] = std::ifstream(files[i]);
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
    std::vector<int64_t> numbers;
    std::ifstream ifs(input);
    int64_t n;
    while(ifs >> n)
        numbers.push_back(n);
    std::sort(numbers.begin(), numbers.end());

    std::ofstream ofs(output, std::ostream::out|std::ostream::app);
    for(auto n : numbers)
        ofs << n << "\n";
}

void DataHandler::sortFile() {
    int64_t file_size = getFileSize(filename.c_str());
    assert(file_size != -1);

    std::string file = std::string(filename);
    if(file_size < 10 * 1024 * 1024) {
        sortFile(file, file + "-sort");
    }
    else {
        std::vector<std::string> files = splitLargeFile(filename.c_str());
        std::vector<std::string> sorted_files;
        for(size_t i = 0; i < files.size(); ++i) {
            std::string f = files[i] + "-sort";
            sortFile(files[i], f);
            sorted_files.push_back(f);
        }
        mergeSortedFiles(sorted_files, file + "-sort");
        std::for_each(files.begin(), files.end(), [](std::string file) { std::remove(file.c_str()); });
        std::for_each(sorted_files.begin(), sorted_files.end(), [](std::string file) { std::remove(file.c_str()); });
    }
}

int64_t partition(std::vector<int64_t> &numbers, int64_t start, int64_t end) {
    int64_t s = start;
    size_t index = (end + start)/2;
    int64_t pivot = numbers[index];
    numbers[index] = numbers[start];
    numbers[start] = pivot;
    ++start;
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
    if(s <= end) {
       numbers[s] = numbers[end];
       numbers[end] = pivot;

       return end;
    }
    else
        return s;
}

int64_t find_kth(std::vector<int64_t> &numbers, int64_t start, int64_t end, int64_t k) {
    assert(end - start >= k);
    int64_t index = partition(numbers, start, end);
    int64_t pos = index - start;
    if(pos == k) {
        return numbers[index];
    }
    else if(pos > k) {
        return find_kth(numbers, start, index-1, k);
    }
    else {
        return find_kth(numbers, index+1, end, k-pos-1);
    }
}

int64_t compute_median_in_memory(const char* filename) {
    std::vector<int64_t> numbers;

    std::ifstream infile(filename);
    int64_t n;
    while(infile >> n) {
        numbers.push_back(n);
    }
    infile.close();

    size_t size = numbers.size();
    return find_kth(numbers, 0, size-1, size/2);
}

int main(int argc, char** argv) {

    return 0;
}
