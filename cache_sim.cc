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

    int i = 0;  ///지우기

    if (trace_file.is_open()) {
        while (std::getline(trace_file, line)) {
            std::sscanf(line.c_str(), "%d %llx", &type, &address);
            cache->access(address, type);

        }
    }
}

////////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv) {
    if (argc != 5) {
        fprintf(stderr, "[Usage]: %s <trace> <cache size (in bytes)> <associativity> "
            "<line size (in bytes)>\n", argv[0]);
        return -1;
    }

    /////////////////////////////////////////////////////////////////////////////
    // TODO: compute the number of sets based on the input arguments 
    //       and pass it to the cache instance
    /////////////////////////////////////////////////////////////////////////////

    //int num_sets = 256;  // example: 256 sets

    //argv[3]이 associativity argv[4]가 line(block) size
    //argv[2]가 cache size

    int num_sets = (atoi(argv[2]) / atoi(argv[3])) / atoi(argv[4]);
    //전체 cache size = block size * associativity * num_set 이므로

    cache_c* cc = new cache_c("L1", num_sets, atoi(argv[3]), atoi(argv[4]));
    /////////////////////////////////////////////////////////////////////////////

    process_trace(cc, argv[1]);
    cc->print_stats();

    delete cc;
    return 0;
}
