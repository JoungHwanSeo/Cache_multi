/**
 * ECE 430.322: Computer Organization
 * Lab 4: Cache Simulation
 */


#ifndef __CACHE_H__
#define __CACHE_H__

#include <cstdint>
#include <string>

typedef uint64_t addr_t;

///////////////////////////////////////////////////////////////////
class cache_entry_c
{
public:
    cache_entry_c() {};
    bool   m_valid;    // valid bit for the cacheline
    bool   m_dirty;    // dirty bit 
    addr_t m_tag;      // tag for the line

    int order; // 최근에 사용되었을수록 작은 숫자... 처음엔 모두 0으로 초기화

    friend class cache_c;
};

///////////////////////////////////////////////////////////////////
class cache_set_c
{
public:
    cache_set_c(int assoc);
    ~cache_set_c();

public:
    cache_entry_c* m_entry;  // array of cache entries. 
    int m_assoc;             // number of cache blocks in a cache set
    // TODO: maintain the LRU stack for this set

    int MRU_num; // 해당 set에서 MRU의 num
};

///////////////////////////////////////////////////////////////////
class cache_c
{
public:
    cache_c(std::string name, int num_set, int assoc, int line_size);
    ~cache_c();
    void access(addr_t address, int access_type);
    void print_stats(void);

private:
    std::string m_name;     // cache name
    int m_num_sets;         // number of sets
    int m_line_size;        // cache line size

    int c_assoc;            //몇 way인가... 개인적 추가

    cache_set_c** m_set;    // cache data structure ddddd

    // cache statistics
    int m_num_accesses;
    int m_num_hits;
    int m_num_misses;
    int m_num_reads;
    int m_num_writes;
    int m_num_writebacks;

};

#endif // !__CACHE_H__ 


