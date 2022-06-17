/**
 * ECE 430.322: Computer Organization
 * Lab 4: Cache Simulation
 */

#define _CRT_SECURE_NO_WARNINGS

#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include "cache.h"

 /**
  * This function opens a trace file and feeds the trace to your cache
  * @param cache - cache instance to process the trace
  * @param name - trace file name
  */
void process_trace(cache_c* cache, const char* name) {
    std::ifstream trace_file(name); ///
    std::string line;

    int type;
    addr_t address; ///


    if (trace_file.is_open()) {
        while (std::getline(trace_file, line)) {
            std::sscanf(line.c_str(), "%d %llx", &type, &address);
            cache->access(address, type);

        }
    }
}

void process_trace_mul(cache_c* cache, const char* name) {
    std::ifstream trace_file(name); ///
    std::string line;

    int type;
    addr_t address; ///


    if (trace_file.is_open()) {
        while (std::getline(trace_file, line)) {
            std::sscanf(line.c_str(), "%d %llx", &type, &address);
            cache->access(address, type);

        }
    }
}

void process_trace_mul_multi(multi_cache* cache, const char* name) {
    std::ifstream trace_file(name); ///
    std::string line;

    int type;
    addr_t address; ///


    if (trace_file.is_open()) {
        while (std::getline(trace_file, line)) {
            std::sscanf(line.c_str(), "%d %llx", &type, &address);
            cache->access(address, type);

        }
    }
}

////////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "[Usage]: %s <trace> <cache size (in bytes)> <associativity> "
            "<line size (in bytes)>\n", argv[0]);
        return -1;
    }
    //이젠 argv[1]만 사용.. 나머지는 고정!

    /////////////////////////////////////////////////////////////////////////////
    // TODO: compute the number of sets based on the input arguments 
    //       and pass it to the cache instance
    /////////////////////////////////////////////////////////////////////////////

    //int num_sets = 256;  // example: 256 sets

    //argv[3]이 associativity argv[4]가 line(block) size
    //argv[2]가 cache size

    //int num_sets = (atoi(argv[2]) / atoi(argv[3])) / atoi(argv[4]);
    //전체 cache size = block size * associativity * num_set 이므로


    //L1은 1024B, 1 way(direct), 64B linesize -> set 개수 = 16
    //L2는 8096B, 4-way, 64B linesize -> set 개수 =  32
    //cache_c* cc_L1_I = new cache_c("L1_I", 16, 1, 64);
    //cache_c* cc_L1_D = new cache_c("L1_D", 16, 1, 64);
    //cache_c* cc_L2   = new cache_c("L1_I", 32, 4, 64);

    //cache_c* cc = new cache_c("L1", num_sets, atoi(argv[3]), atoi(argv[4]));
    multi_cache* cc = new multi_cache("Multi_Cache", 16, 64 , 32, 64, 4);
    /////////////////////////////////////////////////////////////////////////////

    //process_trace(cc, argv[1]);
    process_trace_mul_multi(cc, argv[1]);
    cc->print_status();

    delete cc;
    return 0;
}
