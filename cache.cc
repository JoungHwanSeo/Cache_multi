/**
 * ECE 430.322: Computer Organization
 * Lab 4: Cache Simulation
 */

#include "cache.h"
#include <iostream>
#include <string>
#include <cmath>

 /**
  * This allocates an "assoc" number of cache entries per a set
  * @param assoc - number of cache entries in a set
  */
cache_set_c::cache_set_c(int assoc) {
    m_entry = new cache_entry_c[assoc];
    m_assoc = assoc;

    MRU_num = 0;
}

// cache_set_c destructor
cache_set_c::~cache_set_c() {
    delete[] m_entry;
}

/**
 * This constructor initializes a cache structure based on the cache parameters.
 * @param name - cache name; use any name you want
 * @param num_sets - number of sets in a cache
 * @param assoc - number of cache entries in a set
 * @param line_size - cache block (line) size in bytes
 */
cache_c::cache_c(std::string name, int num_sets, int assoc, int line_size) {
    m_name = name;
    m_num_sets = num_sets;
    m_line_size = line_size;

    c_assoc = assoc;

    m_set = new cache_set_c * [m_num_sets];

    for (int ii = 0; ii < m_num_sets; ++ii) {
        m_set[ii] = new cache_set_c(assoc);

        // initialize tag/valid/dirty bits
        for (int jj = 0; jj < assoc; ++jj) {
            m_set[ii]->m_entry[jj].m_valid = false;
            m_set[ii]->m_entry[jj].m_dirty = false;
            m_set[ii]->m_entry[jj].m_tag = 0;
            //맨 처음 order는 모두 0으로 초기화
            m_set[ii]->m_entry[jj].order = 0;
        }
    }

    // initialize stats
    m_num_accesses = 0;
    m_num_hits = 0;
    m_num_misses = 0;
    m_num_writes = 0;
    m_num_writebacks = 0;
}

// cache_c destructor
cache_c::~cache_c() {
    for (int ii = 0; ii < m_num_sets; ++ii) { delete m_set[ii]; }
    delete[] m_set;
}

/**
 * This function looks up in the cache for a memory reference.
 * This needs to update all the necessary meta-data (e.g., tag/valid/dirty)
 * and the cache statistics, depending on a cache hit or a miss.
 * @param address - memory address
 * @param access_type - read (0), write (1), or instruction fetch (2)
 */
void cache_c::access(addr_t address, int access_type) {
    ////////////////////////////////////////////////////////////////////
    // TODO: Write the code to implement this function

      //이거 구하는 logic 맞는지 확인하기...///////////////////////////////
    int idx_block = address % (m_num_sets * m_line_size);
    int idx = idx_block / (m_line_size);
    int tag = address / (m_num_sets * m_line_size);
    /// /////////////////////////////////////////////////////////////////

    //printf("idx : %x\ntag : %x\n\n", idx, tag);

    cache_entry_c entry;

    bool hit = false;
    bool invalid_exist = false;
    int set_MRU_num;


    int tmp_order;
    int order_; //LRU찾을 때 사용
    int LRU_idx; //찾은 LRU의 idx

    m_num_accesses++; //접근하기만 하면 일단 +1

    //접근 시작
    switch (access_type) {
    case 0: // read

        //해당 set의 MRU number감소
        m_set[idx]->MRU_num--;

        m_num_reads++; //일단 read이므로 +1
        for (int i = 0; i < c_assoc; i++) {
            //각 idx에 대응하는 set에서 asso훑음
            entry = m_set[idx]->m_entry[i];
            if (m_set[idx]->m_entry[i].m_valid == true && m_set[idx]->m_entry[i].m_tag == tag) {
                m_num_hits++;
                hit = true;

                m_set[idx]->m_entry[i].order = m_set[idx]->MRU_num; //감소된 MRU_num이 들어감
                //order가 가장 작은게 MRU...
                //order가 가장 큰데 LRU

                break;
            }
        }


        //miss인 경우
        if (hit == false) {
            m_num_misses++; //모든 set을 찾았는데도 tag match 없음
            //즉 read miss
            //이 경우 해당 set에 data가져왔을 것이므로 way중 하나는 valid되어야함
            //miss인 경우 최우선적으로 invalid인 것을 먼저 채움
            for (int i = 0; i < c_assoc; i++) {
                entry = m_set[idx]->m_entry[i];
                if (entry.m_valid == false) {
                    //invalid존재시 이걸 최우선적으로 채움
                    m_set[idx]->m_entry[i].m_dirty = false; //read이므로 dirty = 0
                    m_set[idx]->m_entry[i].m_valid = true; //데이터 가져왔으므로 true
                    m_set[idx]->m_entry[i].m_tag = tag;
                    m_set[idx]->m_entry[i].order = m_set[idx]->MRU_num;

                    invalid_exist = true;
                    break;
                }
            }

            //miss인데 모두 valid인 경우
            if (invalid_exist == false) {
                //만약 모두 valid였다면 LRU를 찾아서 그거 evict해야함
                order_ = m_set[idx]->m_entry[0].order;
                LRU_idx = 0;
                //한 set에 있는 entry의 order중 가장 큰 거 찾기
                for (int i = 1; i < c_assoc; i++) {
                    tmp_order = m_set[idx]->m_entry[i].order;
                    if (order_ < tmp_order) {
                        order_ = tmp_order;
                        LRU_idx = i;
                    }
                }

                entry = m_set[idx]->m_entry[LRU_idx];

                //for loop끝나면 LRU를 찾음
                //해당 entry를 evict하는데, 이게 dirty이면 write back해야함
                if (m_set[idx]->m_entry[LRU_idx].m_dirty == true) {
                    m_num_writebacks++;
                }
                //만약 evict가 dirty 아니면 write back 필요 
                //entry = m_set[idx]->m_entry[LRU_idx];
                m_set[idx]->m_entry[LRU_idx].m_dirty = false;
                m_set[idx]->m_entry[LRU_idx].m_valid = true;
                m_set[idx]->m_entry[LRU_idx].m_tag = tag; //새로운 tag로 바꿔줌

                //맨 처음에 감소된 MRU_num.. 이게 제일 작음 (MRU가됨)
                m_set[idx]->m_entry[LRU_idx].order = m_set[idx]->MRU_num;

            }

        }
        break;

    case 1: //write

        m_set[idx]->MRU_num--;

        set_MRU_num = m_set[idx]->MRU_num; // 해당 set의 MRU num이 감소

        m_num_writes++; //일단 write이므로
        for (int i = 0; i < c_assoc; i++) {
            entry = m_set[idx]->m_entry[i];
            if (entry.m_valid && entry.m_tag == tag) {
                //만약에 write hit이면 원래 있던 주소에 덮어씀.. WB고려 x
                m_num_hits++;
                m_set[idx]->m_entry[i].m_dirty = true;
                m_set[idx]->m_entry[i].order = set_MRU_num; // 해당 entry를 MRU로 만들어줌
                hit = true;
                break;
            }
        }

        //miss인 경우
        if (hit == false) {
            //해당 idx의 모든 asso찾아도 없었음 -> write allocate
            m_num_misses++; //miss증가
            for (int i = 0; i < c_assoc; i++) {
                entry = m_set[idx]->m_entry[i];
                if (entry.m_valid == false) {
                    //invalid발견시 최우선적으로 채움
                    m_set[idx]->m_entry[i].m_dirty = true;
                    m_set[idx]->m_entry[i].m_tag = tag;
                    m_set[idx]->m_entry[i].m_valid = true;
                    m_set[idx]->m_entry[i].order = set_MRU_num;

                    invalid_exist = true;

                    break;
                }
            }

            //miss인데 모두 valid인 경우 LRU를 찾아 evict해야함 (allocate)
            if (invalid_exist == false) {
                order_ = m_set[idx]->m_entry[0].order;
                LRU_idx = 0;
                for (int i = 1; i < c_assoc; i++) {
                    tmp_order = m_set[idx]->m_entry[i].order;
                    if (order_ < tmp_order) {
                        order_ = tmp_order;
                        LRU_idx = i;
                    }
                }
                //for loop끝날시 LRU 찾음
                entry = m_set[idx]->m_entry[LRU_idx]; //LRU의 entry

                //evict하는 LRU가 dirty이면 WB해야함
                if (m_set[idx]->m_entry[LRU_idx].m_dirty == true) {
                    m_num_writebacks++;
                }
                m_set[idx]->m_entry[LRU_idx].m_dirty = true;
                m_set[idx]->m_entry[LRU_idx].m_tag = tag;
                m_set[idx]->m_entry[LRU_idx].m_valid = true;
                m_set[idx]->m_entry[LRU_idx].order = set_MRU_num;
            }
        }
        break;
    case 2: //instruction fetch
        m_num_reads++; // instruction fetch도 일단 read임
        //read랑 똑같나...?

        //해당 set의 MRU number감소
        m_set[idx]->MRU_num--;

        for (int i = 0; i < c_assoc; i++) {
            //각 idx에 대응하는 set에서 asso훑음
            entry = m_set[idx]->m_entry[i];
            if (m_set[idx]->m_entry[i].m_valid == true && m_set[idx]->m_entry[i].m_tag == tag) {
                m_num_hits++;
                hit = true;

                m_set[idx]->m_entry[i].order = m_set[idx]->MRU_num; //감소된 MRU_num이 들어감
                //order가 가장 작은게 MRU...
                //order가 가장 큰데 LRU

                break;
            }
        }


        //miss인 경우
        if (hit == false) {
            m_num_misses++; //모든 set을 찾았는데도 tag match 없음
            //즉 read miss
            //이 경우 해당 set에 data가져왔을 것이므로 way중 하나는 valid되어야함
            //miss인 경우 최우선적으로 invalid인 것을 먼저 채움
            for (int i = 0; i < c_assoc; i++) {
                entry = m_set[idx]->m_entry[i];
                if (entry.m_valid == false) {
                    //invalid존재시 이걸 최우선적으로 채움
                    m_set[idx]->m_entry[i].m_dirty = false; //read이므로 dirty = 0
                    m_set[idx]->m_entry[i].m_valid = true; //데이터 가져왔으므로 true
                    m_set[idx]->m_entry[i].m_tag = tag;
                    m_set[idx]->m_entry[i].order = m_set[idx]->MRU_num;

                    invalid_exist = true;
                    break;
                }
            }

            //miss인데 모두 valid인 경우
            if (invalid_exist == false) {
                //만약 모두 valid였다면 LRU를 찾아서 그거 evict해야함
                order_ = m_set[idx]->m_entry[0].order;
                LRU_idx = 0;
                //한 set에 있는 entry의 order중 가장 큰 거 찾기
                for (int i = 1; i < c_assoc; i++) {
                    tmp_order = m_set[idx]->m_entry[i].order;
                    if (order_ < tmp_order) {
                        order_ = tmp_order;
                        LRU_idx = i;
                    }
                }

                entry = m_set[idx]->m_entry[LRU_idx];

                //for loop끝나면 LRU를 찾음
                //해당 entry를 evict하는데, 이게 dirty이면 write back해야함
                if (m_set[idx]->m_entry[LRU_idx].m_dirty == true) {
                    m_num_writebacks++;
                }
                //만약 evict가 dirty 아니면 write back 필요 
                //entry = m_set[idx]->m_entry[LRU_idx];
                m_set[idx]->m_entry[LRU_idx].m_dirty = false;
                m_set[idx]->m_entry[LRU_idx].m_valid = true;
                m_set[idx]->m_entry[LRU_idx].m_tag = tag; //새로운 tag로 바꿔줌

                //맨 처음에 감소된 MRU_num.. 이게 제일 작음 (MRU가됨)
                m_set[idx]->m_entry[LRU_idx].order = m_set[idx]->MRU_num;

            }

        }
        break;
    }
    ////////////////////////////////////////////////////////////////////
}

/**
 * Print statistics (DO NOT CHANGE)
 */
void cache_c::print_stats() {
    std::cout << "------------------------------" << "\n";
    std::cout << m_name << " Hit Rate: " << (double)m_num_hits / m_num_accesses * 100 << " % \n";
    std::cout << "------------------------------" << "\n";
    std::cout << "number of accesses: " << m_num_accesses << "\n";
    std::cout << "number of hits: " << m_num_hits << "\n";
    std::cout << "number of misses: " << m_num_misses << "\n";
    std::cout << "number of writes: " << m_num_writes << "\n";
    std::cout << "number of writebacks: " << m_num_writebacks << "\n";

}

