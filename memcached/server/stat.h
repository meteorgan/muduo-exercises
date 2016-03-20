#ifndef MEMCACHED_STAT_H
#define MEMCACHED_STAT_H

#include "muduo/base/ProcessInfo.h"
#include "muduo/base/Timestamp.h"

#include "item.h"

#include <iostream>
#include <sstream>
#include <memory>
#include <map>
#include <mutex>

class MemcachedStat {
    public:
        MemcachedStat()
            : startTime(muduo::ProcessInfo::startTime().secondsSinceEpoch()),
        currItems(0), totalItems(0), bytesUsed(0), currConnections(0), totalConnections(0), 
        cmdGetCount(0), cmdSetCount(0),
        cmdFlushCount(0), cmdTouchCount(0), getHitCount(0), getMissCount(0), 
        deleteHitCount(0), deleteMissCount(0), incrHitCount(0), incrMissCount(0),
        decrHitCount(0), decrMissCount(0), casHitCount(0), casMissCount(0), casBadValCount(0),
        touchHitCount(0), touchMissCount(0) {
        }

        void addCurrItems(int i) { 
            std::lock_guard<std::mutex> lock(mtx);
            currItems += i; 
        }
        void addTotalItems() { 
            std::lock_guard<std::mutex> lock(mtx);
            totalItems++; 
        }

        void addCurrConnections(int i) { 
            std::lock_guard<std::mutex> lock(mtx);
            currConnections += i; 
        }

        void addTotalConnections() { 
            std::lock_guard<std::mutex> lock(mtx);
            totalConnections++; 
        }

        void addCmdGetCount() { 
            std::lock_guard<std::mutex> lock(mtx);
            cmdGetCount++; 
        }

        void addCmdGetHitCount() { 
            std::lock_guard<std::mutex> lock(mtx);
            getHitCount++; 
        }

        void addCmdGetMissCount() { 
            std::lock_guard<std::mutex> lock(mtx);
            getMissCount++; 
        }

        void addCmdSetCount() { 
            std::lock_guard<std::mutex> lock(mtx);
            cmdSetCount++; 
        }

        void addCmdFlushCount() { 
            std::lock_guard<std::mutex> lock(mtx);
            cmdFlushCount++; 
        }

        void addCmdTouchCount() { 
            std::lock_guard<std::mutex> lock(mtx);
            cmdTouchCount++; 
        }

        void addCmdTouchHitCount() { 
            std::lock_guard<std::mutex> lock(mtx);
            touchHitCount++; 
        }

        void addCmdTouchMissCount() { 
            std::lock_guard<std::mutex> lock(mtx);
            touchMissCount++; 
        }

        std::string report() const {
            static const std::string prefix = "STAT ";
            std::lock_guard<std::mutex> lock(mtx);
            std::stringstream fmt;
            fmt << prefix << "pid " << muduo::ProcessInfo::pid() << "\r\n";
            fmt << prefix << "uptime " << muduo::Timestamp::now().secondsSinceEpoch() - startTime << "\r\n";
            fmt << prefix << "time " << muduo::Timestamp::now().secondsSinceEpoch() << "\r\n";
            fmt << prefix << "pointer_size " << sizeof(void*) << "\r\n";
            fmt << prefix << "rusage_user " << muduo::ProcessInfo::cpuTime().userSeconds << "\r\n";
            fmt << prefix << "rusage_system " << muduo::ProcessInfo::cpuTime().systemSeconds << "\r\n";
            fmt << prefix << "curr_connections " << currConnections << "\r\n";
            fmt << prefix << "total_connnections " << totalConnections << "\r\n";
            fmt << prefix << "cmd_get " << cmdGetCount << "\r\n";
            fmt << prefix << "cmd_set " << cmdSetCount << "\r\n";
            fmt << prefix << "cmd_flush " << cmdFlushCount << "\r\n";
            fmt << prefix << "cmd_touch " << cmdTouchCount << "\r\n";
            fmt << prefix << "get_hits " << getHitCount << "\r\n";
            fmt << prefix << "get_misses " << getMissCount << "\r\n";
            fmt << prefix << "delete_hits " << deleteHitCount << "\r\n";
            fmt << prefix << "delete_misses " << deleteMissCount << "\r\n";
            fmt << prefix << "incr_hits " << incrHitCount << "\r\n";
            fmt << prefix << "incr_misses " << incrMissCount << "\r\n";
            fmt << prefix << "decr_hits " << decrHitCount << "\r\n";
            fmt << prefix << "decr_misses " <<  decrMissCount << "\r\n";
            fmt << prefix << "cas_hits " << casHitCount << "\r\n";
            fmt << prefix << "cas_misses " << casMissCount << "\r\n";
            fmt << prefix << "cas_badval " << casBadValCount << "\r\n";
            fmt << prefix << "touch_hits " << touchHitCount << "\r\n";
            fmt << prefix << "touch_misses " << touchMissCount << "\r\n";
            fmt << prefix << "bytes " << bytesUsed << "\r\n";
            fmt << prefix << "curr_items " << currItems << "\r\n";
            fmt << prefix << "total_items " << totalItems << "\r\n";
            
            return fmt.str();
        }

    private:
        uint32_t startTime;
        uint32_t currItems;
        uint32_t totalItems;
        uint64_t bytesUsed;
        uint32_t currConnections;
        uint32_t totalConnections;
        uint64_t cmdGetCount;
        uint64_t cmdSetCount;
        uint64_t cmdFlushCount;
        uint64_t cmdTouchCount;
        uint64_t getHitCount;
        uint64_t getMissCount;
        uint64_t deleteHitCount;
        uint64_t deleteMissCount;
        uint64_t incrHitCount;
        uint64_t incrMissCount;
        uint64_t decrHitCount;
        uint64_t decrMissCount;
        uint64_t casHitCount;
        uint64_t casMissCount;
        uint64_t casBadValCount;
        uint64_t touchHitCount;
        uint64_t touchMissCount;

        mutable std::mutex mtx;
};

#endif
