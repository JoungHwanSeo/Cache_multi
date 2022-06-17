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

    m_num_invalidate = 0;
    m_num_writebacks_to_mem = 0;
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
hit cache_c::access(addr_t address, int access_type,bool MRU_update) {
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

    if (MRU_update) {
        m_set[idx]->MRU_num--; //해당 set의 MRU number감소
    }

    //접근 시작
    switch (access_type) {
    case 0: // read
        m_num_reads++; //일단 read이므로 +1

        
        for (int i = 0; i < c_assoc; i++) {
            //각 idx에 대응하는 set에서 asso훑음
            entry = m_set[idx]->m_entry[i];
            if (m_set[idx]->m_entry[i].m_valid == true && m_set[idx]->m_entry[i].m_tag == tag) {
                m_num_hits++;
                hit = true;

                if (MRU_update) {
                    m_set[idx]->m_entry[i].order = m_set[idx]->MRU_num; //감소된 MRU_num이 들어감
                }
                //order가 가장 작은게 MRU...
                //order가 가장 큰데 LRU
                return READ_HIT;
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
                    if (MRU_update) {
                        m_set[idx]->m_entry[i].order = m_set[idx]->MRU_num;
                    }

                    invalid_exist = true;

                    return READ_MISS_INVALID;
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

                bool Is_evict_dirty = false;

                //for loop끝나면 LRU를 찾음
                //해당 entry를 evict하는데, 이게 dirty이면 write back해야함
                if (m_set[idx]->m_entry[LRU_idx].m_dirty == true) {
                    m_num_writebacks++;
                    Is_evict_dirty = true;
                }
                //만약 evict가 dirty 아니면 write back 필요 
                //entry = m_set[idx]->m_entry[LRU_idx];
                m_set[idx]->m_entry[LRU_idx].m_dirty = false;
                m_set[idx]->m_entry[LRU_idx].m_valid = true;
                m_set[idx]->m_entry[LRU_idx].m_tag = tag; //새로운 tag로 바꿔줌
                
                //맨 처음에 감소된 MRU_num.. 이게 제일 작음 (MRU가됨)
                if (MRU_update) {
                    m_set[idx]->m_entry[LRU_idx].order = m_set[idx]->MRU_num;
                }
                
                if (Is_evict_dirty)
                    return READ_MISS_VALID_DIRTY;
                else
                    return READ_MISS_VALID_CLEAN;
            }

        }
        break;

    case 1: //write

        //m_set[idx]->MRU_num--;

        set_MRU_num = m_set[idx]->MRU_num; // 해당 set의 MRU num이 감소

        m_num_writes++; //일단 write이므로
        for (int i = 0; i < c_assoc; i++) {
            entry = m_set[idx]->m_entry[i];
            if (entry.m_valid && entry.m_tag == tag) {
                //만약에 write hit이면 원래 있던 주소에 덮어씀.. WB고려 x
                m_num_hits++;
                m_set[idx]->m_entry[i].m_dirty = true;
                if (MRU_update) {
                    m_set[idx]->m_entry[i].order = set_MRU_num; // 해당 entry를 MRU로 만들어줌
                }
                hit = true;
                return WRITE_HIT;
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
                    if (MRU_update) {
                        m_set[idx]->m_entry[i].order = set_MRU_num;
                    }

                    invalid_exist = true;
                    return WRITE_MISS_INVALID;
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

                bool Is_evict_dirty = false;

                if (m_set[idx]->m_entry[LRU_idx].m_dirty == true) {
                    m_num_writebacks++;
                    Is_evict_dirty = true;
                }
                m_set[idx]->m_entry[LRU_idx].m_dirty = true;
                m_set[idx]->m_entry[LRU_idx].m_tag = tag;
                m_set[idx]->m_entry[LRU_idx].m_valid = true;
                
                if (MRU_update) {
                    m_set[idx]->m_entry[LRU_idx].order = set_MRU_num;
                    //evict하는 LRU가 dirty이면 WB해야함
                    
                }

                if (Is_evict_dirty)
                    return READ_MISS_VALID_DIRTY;
                else
                    return READ_MISS_VALID_CLEAN;
            }
        }
        break;
    case 2: //instruction fetch
        m_num_reads++; //일단 read이므로 +1


        for (int i = 0; i < c_assoc; i++) {
            //각 idx에 대응하는 set에서 asso훑음
            entry = m_set[idx]->m_entry[i];
            if (m_set[idx]->m_entry[i].m_valid == true && m_set[idx]->m_entry[i].m_tag == tag) {
                m_num_hits++;
                hit = true;

                if (MRU_update) {
                    m_set[idx]->m_entry[i].order = m_set[idx]->MRU_num; //감소된 MRU_num이 들어감
                }
                //order가 가장 작은게 MRU...
                //order가 가장 큰데 LRU
                return READ_HIT;
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
                    if (MRU_update) {
                        m_set[idx]->m_entry[i].order = m_set[idx]->MRU_num;
                    }

                    invalid_exist = true;

                    return READ_MISS_INVALID;
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

                bool Is_evict_dirty = false;

                //for loop끝나면 LRU를 찾음
                //해당 entry를 evict하는데, 이게 dirty이면 write back해야함
                if (m_set[idx]->m_entry[LRU_idx].m_dirty == true) {
                    m_num_writebacks++;
                    Is_evict_dirty = true;
                }
                //만약 evict가 dirty 아니면 write back 필요 
                //entry = m_set[idx]->m_entry[LRU_idx];
                m_set[idx]->m_entry[LRU_idx].m_dirty = false;
                m_set[idx]->m_entry[LRU_idx].m_valid = true;
                m_set[idx]->m_entry[LRU_idx].m_tag = tag; //새로운 tag로 바꿔줌

                //맨 처음에 감소된 MRU_num.. 이게 제일 작음 (MRU가됨)
                if (MRU_update) {
                    m_set[idx]->m_entry[LRU_idx].order = m_set[idx]->MRU_num;
                }

                if (Is_evict_dirty)
                    return READ_MISS_VALID_DIRTY;
                else
                    return READ_MISS_VALID_CLEAN;
            }

        }
        break;
    }
    ////////////////////////////////////////////////////////////////////
}

int cache_c::find_evict_way(addr_t address) {
    //근데 이거 사용하려면 모두 다 valid라는 가정이 있어야함!!!!!!!

    int idx_block = address % (m_num_sets * m_line_size);
    int idx = idx_block / (m_line_size);
    int tag = address / (m_num_sets * m_line_size);

    int order_;
    int LRU_idx;
    int tmp_order;

    order_ = m_set[idx]->m_entry[0].order;
    LRU_idx = 0;
    for (int i = 1; i < c_assoc; i++) {
        tmp_order = m_set[idx]->m_entry[i].order;
        if (order_ < tmp_order) {
            order_ = tmp_order;
            LRU_idx = i;
        }
    }

    return LRU_idx;
}


///새로 시작!!!!!!!
hit cache_c::read(addr_t address, int* evicted_tag_address) {
    int idx_block = address % (m_num_sets * m_line_size);
    int idx = idx_block / (m_line_size);
    int tag = address / (m_num_sets * m_line_size);

    m_num_accesses++; //접근이니까 ++
    m_num_reads++;

    m_set[idx]->MRU_num--;
        
    //hit인 경우
    for (int i = 0; i < c_assoc; i++) {
        if (m_set[idx]->m_entry[i].m_valid == true && m_set[idx]->m_entry[i].m_tag == tag) {
            m_num_hits++;
            //MRU update해도 되는 상황이면
            m_set[idx]->m_entry[i].order = m_set[idx]->MRU_num; //감소된 MRU_num이 들어감
            return READ_HIT;
        }
    }

    m_num_misses++;
    for (int i = 0; i < c_assoc; i++) {
        if (m_set[idx]->m_entry[i].m_valid == false) { //만약 invalid항목 있으면

            m_set[idx]->m_entry[i].m_dirty = false; //read이므로 dirty = 0
            m_set[idx]->m_entry[i].m_valid = true; //데이터 가져왔으므로 true
            m_set[idx]->m_entry[i].m_tag = tag;

            m_set[idx]->m_entry[i].order = m_set[idx]->MRU_num; //감소된 MRU_num이 들어감
            return READ_MISS_INVALID;
        }
    }
    //여기서는 해당 set이 모두 valid함이 가정되므로 find_evict사용 가능
    int LRU_idx = find_evict_way(address); // 해당 idx의 set에서 LRU 찾음
    int evict_tag = m_set[idx]->m_entry[LRU_idx].m_tag;
    //evict되는 부분이 clean한지 여부
    bool Is_evict_dirty = m_set[idx]->m_entry[LRU_idx].m_dirty;

    m_set[idx]->m_entry[LRU_idx].m_dirty = false;
    m_set[idx]->m_entry[LRU_idx].m_valid = true;
    m_set[idx]->m_entry[LRU_idx].m_tag = tag; //새로운 tag로 바꿔줌

    *evicted_tag_address = evict_tag * (m_num_sets * m_line_size) + (idx * m_line_size); // 함수에 들어온 인자에 값을 써줌
    m_set[idx]->m_entry[LRU_idx].order = m_set[idx]->MRU_num; //감소된 MRU_num이 들어감
    if (Is_evict_dirty == true) {
        m_num_writebacks++;
        return READ_MISS_VALID_DIRTY;
    }
    else
        return READ_MISS_VALID_CLEAN;
}

hit cache_c::write(addr_t address, int* evicted_tag_address) {
    m_num_accesses++;
    m_num_writes++;
    int idx_block = address % (m_num_sets * m_line_size);
    int idx = idx_block / (m_line_size);
    int tag = address / (m_num_sets * m_line_size);
    m_set[idx]->MRU_num--; //MRU update위함
    for (int i = 0; i < c_assoc; i++) {
        if (m_set[idx]->m_entry[i].m_valid == true && m_set[idx]->m_entry[i].m_tag == tag) {
            //원래 주소에 덮어쓰면 됨
            m_num_hits++;
            m_set[idx]->m_entry[i].m_dirty = true;
            m_set[idx]->m_entry[i].order = m_set[idx]->MRU_num; //MRU update
            return WRITE_HIT;
        }
    }
    //miss인 경우
    m_num_misses++;
    for (int i = 0; i < c_assoc; i++) {
        if (m_set[idx]->m_entry[i].m_valid == false) {
            //invalid 최우선적으로 채움
            m_set[idx]->m_entry[i].m_dirty = true;
            m_set[idx]->m_entry[i].m_tag = tag;
            m_set[idx]->m_entry[i].m_valid = true;
            m_set[idx]->m_entry[i].order = m_set[idx]->MRU_num; //MRU update
            return WRITE_MISS_INVALID;
        }
    }
    //여기서는 해당 set이 모두 valid함이 가정되므로 find_evict사용 가능
    int LRU_idx = find_evict_way(address); // 해당 set에서 LRU way찾음
    int evict_tag = m_set[idx]->m_entry[LRU_idx].m_tag;
    bool Is_evict_dirty = m_set[idx]->m_entry[LRU_idx].m_dirty;
    int evict_address_from_tag = evict_tag * (m_num_sets * m_line_size) + (idx * m_line_size);
    *evicted_tag_address = evict_address_from_tag; // evict되는 주소(block은 00이지만...)

    m_set[idx]->m_entry[LRU_idx].m_dirty = true;
    m_set[idx]->m_entry[LRU_idx].m_tag = tag;
    m_set[idx]->m_entry[LRU_idx].m_valid = true;
    m_set[idx]->m_entry[LRU_idx].order = m_set[idx]->MRU_num; //MRU_update
    if (Is_evict_dirty) {
        m_num_writebacks++;
        return WRITE_MISS_VALID_DIRTY;
    }
    else {
        return WRITE_MISS_VALID_CLEAN;
    }

}

hit cache_c::only_access(addr_t address) {
    int idx_block = address % (m_num_sets * m_line_size);
    int idx = idx_block / (m_line_size);
    int tag = address / (m_num_sets * m_line_size);

    m_num_accesses++;
    for (int i = 0; i < c_assoc; i++) {
        if (m_set[idx]->m_entry[i].m_valid == true && m_set[idx]->m_entry[i].m_tag == tag) {
            m_num_hits++;
            return HIT;
        }
    }
    m_num_misses++;
    return MISS;
}

hit cache_c::only_access_more(addr_t address) {
    int idx_block = address % (m_num_sets * m_line_size);
    int idx = idx_block / (m_line_size);
    int tag = address / (m_num_sets * m_line_size);

    for (int i = 0; i < c_assoc; i++) {
        if (m_set[idx]->m_entry[i].m_valid == true && m_set[idx]->m_entry[i].m_tag == tag) {
            m_num_hits++;
            return HIT;
        }
    }
    
    for (int i = 0; i < c_assoc; i++) {
        if (m_set[idx]->m_entry[i].m_valid == false) { //invalid하나라도 찾으면
            return MISS_INVALID;
        }
    }
    //여기까지 오면 모두 valid함이 보장됨
    int LRU_idx = find_evict_way(address);
    bool Is_evicted_dirty = m_set[idx]->m_entry[LRU_idx].m_dirty;
    if (Is_evicted_dirty == true)
        return MISS_VALID_DIRTY;
    else
        return MISS_VALID_CLEAN;
}

hit cache_c::write_back(addr_t address, int* evicted_tag_address) {


    //지우기
    //m_num_writes++;
    //

    int idx_block = address % (m_num_sets * m_line_size);
    int idx = idx_block / (m_line_size);
    int tag = address / (m_num_sets * m_line_size);
    for (int i = 0; i < c_assoc; i++) {
        if (m_set[idx]->m_entry[i].m_valid == true && m_set[idx]->m_entry[i].m_tag == tag) {
            //원래 주소에 덮어쓰면 됨
            //m_num_hits++;
            //통계량 바꾸면 안됨!
            m_set[idx]->m_entry[i].m_dirty = true;
            return WRITE_HIT;
        }
    }
    //miss인 경우
    for (int i = 0; i < c_assoc; i++) {
        if (m_set[idx]->m_entry[i].m_valid == false) {
            //invalid 최우선적으로 채움
            m_set[idx]->m_entry[i].m_dirty = true;
            m_set[idx]->m_entry[i].m_tag = tag;
            m_set[idx]->m_entry[i].m_valid = true;
            return WRITE_MISS_INVALID;
        }
    }
    //여기서는 해당 set이 모두 valid함이 가정되므로 find_evict사용 가능
    int LRU_idx = find_evict_way(address); // 해당 set에서 LRU way찾음
    int evict_tag = m_set[idx]->m_entry[LRU_idx].m_tag;
    bool Is_evict_dirty = m_set[idx]->m_entry[LRU_idx].m_dirty;
    int evict_address_from_tag = evict_tag * (m_num_sets * m_line_size) + (idx * m_line_size);
    *evicted_tag_address = evict_address_from_tag; // evict되는 주소(block은 00이지만...)

    m_set[idx]->m_entry[LRU_idx].m_dirty = true;
    m_set[idx]->m_entry[LRU_idx].m_tag = tag;
    m_set[idx]->m_entry[LRU_idx].m_valid = true;
    if (Is_evict_dirty) {
        m_num_writebacks++; // L2에서도 Write Back...... 맞나....?
        return WRITE_MISS_VALID_DIRTY;
    }
    else {
        return WRITE_MISS_VALID_CLEAN;
    }
}

//only_read에서 MRU 갱신여부??????????????????????????????????????????????????????
//L1,L2 모두 write miss에서 L2에 해당 항목을 만들기 위함일 뿐 instruction에서 해당 주소를 참조한게 아님
hit cache_c::only_read(addr_t address, int* evicted_tag_address) {
    int idx_block = address % (m_num_sets * m_line_size);
    int idx = idx_block / (m_line_size);
    int tag = address / (m_num_sets * m_line_size);

    //m_num_accesses++; //접근이니까 ++
    //m_num_reads++;

    //m_set[idx]->MRU_num--;

    //hit인 경우
    for (int i = 0; i < c_assoc; i++) {
        if (m_set[idx]->m_entry[i].m_valid == true && m_set[idx]->m_entry[i].m_tag == tag) {
            //m_num_hits++;
            //MRU update해도 되는 상황이면
            //m_set[idx]->m_entry[i].order = m_set[idx]->MRU_num; //감소된 MRU_num이 들어감
            return READ_HIT;
        }
    }
    //근데 사실 이거 쓰는거면 L2에서 miss일 때 쓰는거라 위의 hit인 경우로 갈 리는 없는듯

    //m_num_misses++;
    for (int i = 0; i < c_assoc; i++) {
        if (m_set[idx]->m_entry[i].m_valid == false) { //만약 invalid항목 있으면

            m_set[idx]->m_entry[i].m_dirty = false; //read이므로 dirty = 0
            m_set[idx]->m_entry[i].m_valid = true; //데이터 가져왔으므로 true
            m_set[idx]->m_entry[i].m_tag = tag;

            //m_set[idx]->m_entry[i].order = m_set[idx]->MRU_num; //감소된 MRU_num이 들어감
            return READ_MISS_INVALID;
        }
    }
    //여기서는 해당 set이 모두 valid함이 가정되므로 find_evict사용 가능
    int LRU_idx = find_evict_way(address); // 해당 idx의 set에서 LRU 찾음
    int evict_tag = m_set[idx]->m_entry[LRU_idx].m_tag;
    //evict되는 부분이 clean한지 여부
    bool Is_evict_dirty = m_set[idx]->m_entry[LRU_idx].m_dirty;

    m_set[idx]->m_entry[LRU_idx].m_dirty = false;
    m_set[idx]->m_entry[LRU_idx].m_valid = true;
    m_set[idx]->m_entry[LRU_idx].m_tag = tag; //새로운 tag로 바꿔줌

    *evicted_tag_address = evict_tag * (m_num_sets * m_line_size) + (idx * m_line_size); // 함수에 들어온 인자에 값을 써줌
    //m_set[idx]->m_entry[LRU_idx].order = m_set[idx]->MRU_num; //감소된 MRU_num이 들어감
    //위에거 없애줬어야함...............ㅋㅋㅋㅋㅋㅋ

    if (Is_evict_dirty == true) {
        m_num_writebacks++;
        return READ_MISS_VALID_DIRTY;
    }
    else
        return READ_MISS_VALID_CLEAN;
}

void cache_c::Invalidate(addr_t address) {
    //m_num_invalidate++;
    int idx_block = address % (m_num_sets * m_line_size);
    int idx = idx_block / (m_line_size);
    int tag = address / (m_num_sets * m_line_size);

    bool Is_evict_dirty = false;

    for (int i = 0; i < c_assoc; i++) {
        if (m_set[idx]->m_entry[i].m_valid == true && m_set[idx]->m_entry[i].m_tag == tag) {
            //해당 항목이 존재하면 invalidate
            m_num_invalidate++; //여기 까지 와야 invalidate가 진행됨!!!!!!!

            m_set[idx]->m_entry[i].m_valid = false;
            if (m_set[idx]->m_entry[i].m_dirty == true) { //dirty이면 write_back
                m_num_writebacks++; //바로 메모리로 WB.. L2생각 안해도 ok
                m_num_writebacks_to_mem++; //mem으로 바로가는 WB
                //printf("Invalidate_WB_to mem\n");
            }
            break;
        }
    }
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
    std::cout << "number of reads: " << m_num_reads << "\n";
    std::cout << "number of writes: " << m_num_writes << "\n";
    std::cout << "number of writebacks: " << m_num_writebacks << "\n";

    std::cout << "number of invalidates: " << m_num_invalidate << "\n";
    std::cout << "number of writeback to mem due to Invalidation: " << m_num_writebacks_to_mem << "\n";

}

void cache_c::L1_print(){
    std::cout << "------------------------------" << "\n";
    std::cout << m_name << " Hit Rate: " << (double)m_num_hits / m_num_accesses * 100 << " % \n";
    std::cout << "------------------------------" << "\n";
    std::cout << "number of accesses: " << m_num_accesses << "\n";
    std::cout << "number of hits: " << m_num_hits << "\n";
    std::cout << "number of misses: " << m_num_misses << "\n";
    std::cout << "number of writes: " << m_num_writes << "\n";
    std::cout << "number of writebacks: " << m_num_writebacks << "\n";
    std::cout << "number of invalidates: " << m_num_invalidate << "\n";
    std::cout << "number of writeback to mem due to Invalidation: " << m_num_writebacks_to_mem << "\n";

}

void cache_c::L2_print() {
    std::cout << "------------------------------" << "\n";
    std::cout << m_name << " Hit Rate: " << (double)m_num_hits / m_num_accesses * 100 << " % \n";
    std::cout << "------------------------------" << "\n";
    std::cout << "number of accesses: " << m_num_accesses << "\n";
    std::cout << "number of hits: " << m_num_hits << "\n";
    std::cout << "number of misses: " << m_num_misses << "\n";
    //std::cout << "number of writebacks: " << m_num_writebacks << "\n";
}

bool cache_c::Is_hit(addr_t address, int* hit_asso) {
    int idx_block = address % (m_num_sets * m_line_size);
    int idx = idx_block / (m_line_size);
    int tag = address / (m_num_sets * m_line_size);

    for (int i = 0; i < c_assoc; i++) {
        if (m_set[idx]->m_entry[i].m_valid == true && m_set[idx]->m_entry[i].m_tag == tag) {
            *hit_asso = i;
            return true;
        }
    }
    return false;
}

bool cache_c::Is_Exist_Invalid(addr_t address,int* invalid_asso) {
    int idx_block = address % (m_num_sets * m_line_size);
    int idx = idx_block / (m_line_size);
    int tag = address / (m_num_sets * m_line_size);
    for (int i = 0; i < c_assoc; i++) {
        if (m_set[idx]->m_entry[i].m_valid == false) {
            *invalid_asso = i;
            return true; //invalid존재
        }
    }
    return false;

}



/////////multicache///////////////////////////
multi_cache::multi_cache(std::string name, int L1_num_set, int L1_line_size, int L2_num_set,int L2_line_size, int L2_assoc) {
    m_name = name;
    L1_I_cache = new cache_c("L1_I", L1_num_set, 1, L1_line_size);
    L1_D_cache = new cache_c("L1_D", L1_num_set, 1, L1_line_size);
    L2_cache = new cache_c("L2", L2_num_set, L2_assoc, L2_line_size);

    m_num_sets_1 = L1_num_set;
    m_line_size_1 = L1_line_size;

    m_num_sets_2 = L2_num_set;
    m_line_size_2 = L2_line_size;

    m_num_accesses = 0;
    m_num_hits = 0;
    m_num_misses = 0;
    m_num_writes = 0;
    m_num_writebacks = 0;
}

multi_cache::~multi_cache() {
    delete L1_I_cache;
    delete L1_D_cache;
    delete L2_cache;
}

void multi_cache::access(addr_t address, int access_type) {
    m_num_accesses++; //접근마다 1증가

    int idx_block_1 = address % (m_num_sets_1 * m_line_size_1);
    int idx_1 = idx_block_1 / (m_line_size_1);
    int tag_1 = address / (m_num_sets_1 * m_line_size_1);

    int idx_block_2 = address % (m_num_sets_2 * m_line_size_2);
    int idx_2 = idx_block_2 / (m_line_size_2);
    int tag_2 = address / (m_num_sets_2 * m_line_size_2);

    hit L1_result;
    hit L2_result;
    hit WB_result;

    int evicted_address;
    int evicted_address_2;
    int evicted_address_WB;

    switch (access_type) {
    case 0: //read
        L1_result = L1_D_cache->read(address, &evicted_address);
        if (L1_result == READ_HIT) {} //할거 없음 끝!

        else if (L1_result == READ_MISS_INVALID || L1_result == READ_MISS_VALID_CLEAN) {
            //L1에 evict되어도 딱히 WB안하므로 L2에 다시 뭐 안해도 됨(혹은 그냥 씀)
            L2_result = L2_cache->read(address, &evicted_address_2);
            if (L2_result == READ_MISS_VALID_CLEAN || L2_result == READ_MISS_VALID_DIRTY) {
                //만약 L2에서 evict가 일어나는 경우 해당 주소가 L1에 있는지 확인
                L1_D_cache->Invalidate(evicted_address_2);

                //Invalidate시 L1_I도 봐주는 게 맞을지도...?
                L1_I_cache->Invalidate(evicted_address_2);
            }
            //else의 경우 L2에서 evict되는 것 없음!
        }

        else { //L1의 evicted block이 dirty일 때,즉 WB해줘야 하는 경우
            //이 경우도 WB를 먼저 해줘야 하는거 아닐까??????
            //WB를 나중에 하면 L2로 읽어온 값이 WB로 사라질 수도 있을수도..
            //Inclusivce가 깨짐 근데 그러면 L2에서 invalidate L1에 진행하긴 하네...
            //즉 순서 상관은 없을듯

            //잠만 아닌가?
            //

            L2_result = L2_cache->read(address, &evicted_address_2);


            ///잠만 L2 hit인거랑 MISS_INVALID랑 같이 해도 됨??
            //그런듯...?

            if (L2_result == READ_HIT || L2_result == READ_MISS_INVALID) {
                WB_result = L2_cache->write_back(evicted_address, &evicted_address_WB); //없어진 L1 block 내용을 써야함
                if (WB_result == WRITE_MISS_VALID_CLEAN || WB_result == WRITE_MISS_VALID_DIRTY) {
                    //만약 WB 결과 L2에서 또 evict되면
                    L1_D_cache->Invalidate(evicted_address_WB);

                    L1_I_cache->Invalidate(evicted_address_WB);
                }
            }

            else if (L2_result == READ_MISS_VALID_CLEAN || L2_result == READ_MISS_VALID_DIRTY) {
                //일단 L2의 read결과 evict...
                L1_D_cache->Invalidate(evicted_address_2);

                L1_I_cache->Invalidate(evicted_address_2);
                WB_result = L2_cache->write_back(evicted_address, &evicted_address_WB);
                if (WB_result == WRITE_MISS_VALID_CLEAN || WB_result == WRITE_MISS_VALID_DIRTY) {
                    L1_D_cache->Invalidate(evicted_address_WB);

                    L1_I_cache->Invalidate(evicted_address_WB);
                }
            }

        }
        break;

    case 1: //write
        //write는 전반적으로 다시 검토해야 할듯!!!!!!!!!!!!!!!!!
        int L2_access_result;
        int L2_only_read_result;

        L1_result = L1_D_cache->write(address, &evicted_address);
        if (L1_result == WRITE_HIT) {} //여기서 끝, 할 거 없음 L2에도 해당 항목 있다는 의미!

        else if (L1_result == WRITE_MISS_INVALID || L1_result == WRITE_MISS_VALID_CLEAN){ //L1에 새로 써지므로 L2에도 써야함
            //L1에 새로 써지긴 했으나 해당 항목은 dirty가 아니라서 WB필요가 없음
            L2_access_result = L2_cache->only_access(address);

            if (L2_access_result == HIT) { //L1에서 miss인데 L2에서 hit인 경우
                //그냥 끝내면 되긴 하는데 L2에 접근했고, hit이었음을 L2 기록에 남겨야함
                //아무튼 L2에도 항목은 있으므로 ok임
            }

            else //L2 접근 결과도 miss
            {
                L2_only_read_result = L2_cache->only_read(address, &evicted_address_2);
                //miss에서 시작했으므로 위 결과는 절대 hit은 아님
                //일단 읽어와서 항목은 만들어줌

                if (L2_only_read_result == READ_MISS_INVALID) { //읽어온 결과 L2 evict없음
                    //딱히 할 거 없다!
                }

                else { //읽어온 결과 L2 evict 생김 ->L1 invalidate
                    L1_D_cache->Invalidate(evicted_address_2);

                    L1_I_cache->Invalidate(evicted_address_2);
                }
            }
        }

        else { //L1의 evict block이 dirty로 WB해줘야함
            L2_access_result = L2_cache->only_access(address);

            if (L2_access_result == HIT) {
                //L1에서 evicted block의 address를 L2에 WB해줘야함
                WB_result = L2_cache->write_back(evicted_address, &evicted_address_WB);
                //이 때 L2의 WB결과 evict이 있으면 Invalidate... 여기서 L1이 또 dirty이면 바로 mem으로 감
                if (WB_result == WRITE_MISS_VALID_CLEAN || WB_result == WRITE_MISS_VALID_DIRTY) {
                    L1_D_cache->Invalidate(evicted_address_WB);

                    L1_I_cache->Invalidate(evicted_address_WB);
                }
            }

            else { // L2도 miss인 경우 (L1은 이미 evict되고 그게 dirty임....)

                //일단 L1에서 evict된 dirty를 L2에 최우선적으로 먼저 써주기!!!!!!!!
                WB_result = L2_cache->write_back(evicted_address, &evicted_address_WB);
                if (WB_result == WRITE_MISS_VALID_CLEAN || WB_result == WRITE_MISS_VALID_DIRTY) {
                    L1_D_cache->Invalidate(evicted_address_WB);

                    L1_I_cache->Invalidate(evicted_address_WB);
                }

                //일단 L2에 WB한 다음에 메모리에서 읽어옴, 즉 L1에 있는 항목을 L2에도 만들어줌
                L2_only_read_result = L2_cache->only_read(address, &evicted_address_2);

                //일단 L2 miss에서 시작했으므로 결과가 절대 hit은 아니다
                if (L2_only_read_result == READ_MISS_INVALID) {
                    //할 거 없음!!!
                }

                else { //읽어온 결과 L2에서 evict 된 것이 있음 ->L1 invalidate
                    L1_D_cache->Invalidate(evicted_address_2);

                    L1_I_cache->Invalidate(evicted_address_2);
                }
            }
        }
            
        break;

    case 2: //instruction fetch
        L1_result = L1_I_cache->read(address, &evicted_address);
        if (L1_result == READ_HIT) {} //할거 없음 끝!

        else if (L1_result == READ_MISS_INVALID || L1_result == READ_MISS_VALID_CLEAN) {
            //L1에 evict되어도 딱히 WB안하므로 L2에 다시 뭐 안해도 됨(혹은 그냥 씀)
            L2_result = L2_cache->read(address, &evicted_address_2);
            if (L2_result == READ_MISS_VALID_CLEAN || L2_result == READ_MISS_VALID_DIRTY) {
                //만약 L2에서 evict가 일어나는 경우 해당 주소가 L1에 있는지 확인
                L1_D_cache->Invalidate(evicted_address_2);

                //Invalidate시 L1_I도 봐주는 게 맞을지도...?
                L1_I_cache->Invalidate(evicted_address_2);
            }
            //else의 경우 L2에서 evict되는 것 없음!
        }

        else { //L1의 evicted block이 dirty일 때,즉 WB해줘야 하는 경우
            //이 경우도 WB를 먼저 해줘야 하는거 아닐까??????
            //WB를 나중에 하면 L2로 읽어온 값이 WB로 사라질 수도 있을수도..
            //Inclusivce가 깨짐 근데 그러면 L2에서 invalidate L1에 진행하긴 하네...

            L2_result = L2_cache->read(address, &evicted_address_2);


            ///잠만 L2 hit인거랑 MISS_INVALID랑 같이 해도 됨??
            //그런듯...?

            if (L2_result == READ_HIT || L2_result == READ_MISS_INVALID) {
                WB_result = L2_cache->write_back(evicted_address, &evicted_address_WB); //없어진 L1 block 내용을 써야함
                if (WB_result == WRITE_MISS_VALID_CLEAN || WB_result == WRITE_MISS_VALID_DIRTY) {
                    //만약 WB 결과 L2에서 또 evict되면
                    L1_D_cache->Invalidate(evicted_address_WB);

                    L1_I_cache->Invalidate(evicted_address_WB);
                }
            }

            else if (L2_result == READ_MISS_VALID_CLEAN || L2_result == READ_MISS_VALID_DIRTY) {
                //일단 L2의 read결과 evict...
                L1_D_cache->Invalidate(evicted_address_2);

                L1_I_cache->Invalidate(evicted_address_2);
                WB_result = L2_cache->write_back(evicted_address, &evicted_address_WB);
                if (WB_result == WRITE_MISS_VALID_CLEAN || WB_result == WRITE_MISS_VALID_DIRTY) {
                    L1_D_cache->Invalidate(evicted_address_WB);

                    L1_I_cache->Invalidate(evicted_address_WB);
                }
            }

        }
        break;
    }
}




//////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

//hit이면 true return
/*
* 
* void multi_cache::access(addr_t address, int access_type) {
    m_num_accesses++; //접근마다 1증가

    int idx_block_1 = address % (m_num_sets_1 * m_line_size_1);
    int idx_1 = idx_block_1 / (m_line_size_1);
    int tag_1 = address / (m_num_sets_1 * m_line_size_1);

    int idx_block_2 = address % (m_num_sets_2 * m_line_size_2);
    int idx_2 = idx_block_2 / (m_line_size_2);
    int tag_2 = address / (m_num_sets_2 * m_line_size_2);

    hit L1_result;
    hit L2_result;

    int evicted_tag;
    bool evicted_dirty;

    //int L2_evicted_way;

 

    switch (access_type) {
    case 0: //read     L1_D_cache사용
        //우선 L1에 접근
        //L1은 MRU관리해줄 필요 없긴 함... Direct라서

        //미리 L1의 tag와 dirty정보 받아옴
        //if (L1_D_cache->m_set[idx_1]->m_entry[0].m_valid == true) {
        //    evicted_tag = L1_D_cache->m_set[idx_1]->m_entry[0].m_tag;
        //    evicted_dirty = L1_D_cache->m_set[idx_1]->m_entry[0].m_dirty;
        //}

        bool L2_invalid = false;

        L1_result = L1_D_cache->access_result(address, access_type);

        if (L1_result == READ_HIT) {
            m_num_hits++;
            L1_D_cache->access(address, access_type); //update
        }
        //L1에서 read miss인 경우 idx_1에서 miss임
        else {
            L2_result = L2_cache->access_result(address, access_type); //L2도 read
            if (L2_result == READ_HIT) {
                m_num_hits++;
                L2_cache->access(address, access_type); //update
                //L2에서 hit이고 해당 값을 L1에 써주면 됨
                if (L1_result == READ_MISS_VALID) {
                    //만약 L1의 해당 idx가 valid였으면 해당 block을 evict
                    //L1의 miss된 부분의 tag와 dirty여부 (해당 idx는 if조건따라 valid)
                    evicted_tag = L1_D_cache->m_set[idx_1]->m_entry[0].m_tag;
                    evicted_dirty = L1_D_cache->m_set[idx_1]->m_entry[0].m_dirty;

                    ///    <L1에 L2에서 가져온 것 쓰는 부분>
                    L1_D_cache->m_set[idx_1]->m_entry[0].m_dirty = false;
                    //L2에서 가져오는 것이므로 clean
                    L1_D_cache->m_set[idx_1]->m_entry[0].m_tag = tag_1;
                    L1_D_cache->m_set[idx_1]->m_entry[0].m_valid = true;
                    //해당 tag를 원래 접근한 idx에서 L2에서 찾아 써줌

                    if (evicted_dirty == true) { //이 경우만 다시 L2로 돌아가서 살펴봄!
                        //evict되는 L1 block이 dirty이면 L2에 써줘야함
                        //evict되는 L1 block의 tag 알아야 함
                        //evict되는 L1 block의 idx는 처음 접근 idx와 같음
                        //L1 miss이므로 접근 tag와 저장된 tag가 다름

                        // <L2 처리 부분> L1에서 dirty였던 block을 L2에 써줘야함
                        bool return_L2_result = L2_cache->access_result(address, 1);
                        if (return_L2_result == WRITE_HIT)
                            L2_cache->access(address, 1, false);
                        else if (return_L2_result == WRITE_MISS_INVALID) {
                            L2_cache->access(address, 1, false);
                        }
                        else { //L2에서 evict해야 하는 경우
                            int L2_evict_way = L2_cache->find_evict_way(address);
                            //invalidate logic
                            if (L2_cache->m_set[idx_2]->m_entry[L2_evict_way].m_tag == L1_D_cache->m_set[idx_1]->m_entry[0].m_tag) {
                                L1_D_cache->m_set[idx_1]->m_entry[0].m_valid = false;
                            }
                            L2_cache->access(address, 1, false);
                        }
                        //근데 L2에서 evict해야하는 경우에는 L1을 봐서 invalidate logic도 필요...
                    }
                    else {
                        //L1에서 evicted되는 부분이 clean인 경우 해당 block은 하위 메모리와 같음
                        //그냥 써주면 됨
                        //L2에 따로 뭐 할 필요 없다!... nothing...
                    }
                }
                else {
                    //L1 result에서 invalid인 경우 L2에서 가져온 값 그냥 거기에 쓰면 됨
                    L1_D_cache->m_set[idx_1]->m_entry[0].m_dirty = false;
                    //L2에서 가져오는 것이므로 clean
                    L1_D_cache->m_set[idx_1]->m_entry[0].m_tag = tag;
                    L1_D_cache->m_set[idx_1]->m_entry[0].m_valid = true;
                    //해당 tag를 원래 접근한 idx에서 L2에서 찾아 써줌

                }
            }
            else {
                m_num_misses++; // 둘 다 miss이므로 전체 miss
                //L2에서도 Read miss인 경우
                //일단 메모리에서 data 둘다 가져옴
                if (L1_result == READ_MISS_INVALID) {
                    if (L2_result == READ_MISS_INVALID) {
                        L1_D_cache->access(address, access_type, true);
                        L2_cache->access(address, access_type, true);
                    }
                    else { //L2 idx가 모두 valid... 하나 evict해야함
                        int L2_evict_way_f = L2_cache->find_evict_way(address);
                        //L2의 evict항목과 L1의 evict항목이 같으면 L1 invalidate
                    }
                }
                else {
                    //L1이 Read miss valid -> evict항목 찾아야 함
                }
            }
        }

        break;
    case 1: //write
        break;
    case 2: //Instruction fetch     L1_I_cache사용
        break;
    }
}
* 
bool multi_cache::L1_read(cache_c* L1, cache_c* L2, addr_t address) {
    //이거는 그냥  밖에서 세주는게 편할지도........ 상황따라 같은 read여도 뭐는 세고
    //뭐는 안세고 이래서
    L1->m_num_accesses++; //일단 L1 접근이므로
    L1->m_num_reads++;

    int idx_block_1 = address % (L1->m_num_sets * L1->m_line_size);
    int idx_1 = idx_block_1 / (L1->m_line_size);
    int tag_1 = address / (L1->m_num_sets * L1->m_line_size);

    int idx_block_2 = address % (L2->m_num_sets * L2->m_line_size);
    int idx_2 = idx_block_2 / (L2->m_line_size);
    int tag_2 = address / (L2->m_num_sets * L2->m_line_size);

    int hit_asso;
    int invalid_asso;
    int LRU_way;

    //우선 해당 address에서 hit인지 판단
    if (L1->Is_hit(address, &hit_asso)) {
        L1->m_set[idx_1]->MRU_num--;
        L1->m_set[idx_1]->m_entry[hit_asso].order = L1->m_set[idx_1]->MRU_num;
        L1->m_num_hits++;
        return true; //hit이라서 true return
    }
    else { //miss인 경우 반
        //L2에서 hit
        if (L2_read(L1, L2, address)) {
            //L2에서 hit이 되어 거기서 읽은 값을 L1에 다시 쓰기
            if (L1->Is_Exist_Invalid(address, &invalid_asso)) {
                //만약 L1의 해당 항목이 invalid
                L1->m_set[idx_1]->m_entry[invalid_asso].m_valid = true;
                L1->m_set[idx_1]->m_entry[invalid_asso].m_tag = tag_1;
                L1->m_set[idx_1]->m_entry[invalid_asso].m_dirty = false;
                //일단 L2의 값이 그대로 오므로 L1은 clean L2가 dirty일지라도..

                //MRU update
                L1->m_set[idx_1]->MRU_num--;
                L1->m_set[idx_1]->m_entry[invalid_asso].order = L1->m_set[idx_1]->MRU_num;
            }
            else {
                //L1의 해당 항목이 valid 즉 evict해야함...

                //이 evict되는 부분은 또 다른 함수로 만들기???
                L1_evict(L1, L2, address, true);

                //LRU_way = L1->find_evict_way(address); //가장 오래 안쓰인 way찾음

            }
        }
        else { //L2에서도 miss

        }
    }

}

//L1에서 miss였기에 여기로 내려옴
bool multi_cache::L2_read(cache_c* L1, cache_c* L2, addr_t address) {
    L2->m_num_accesses++;//일단 L2 접근이므로
    L2->m_num_reads++;

    int idx_block_2 = address % (L2->m_num_sets * L2->m_line_size);
    int idx_2 = idx_block_2 / (L2->m_line_size);
    int tag_2 = address / (L2->m_num_sets * L2->m_line_size);
    int hit_asso;
    int invalid_asso;
    if (L2->Is_hit(address, &hit_asso)) {
        L2->m_set[idx_2]->MRU_num--;
        L2->m_set[idx_2]->m_entry[hit_asso].order = L2->m_set[idx_2]->MRU_num;
        //L2에서 읽은 값을 L1에 써줘야 함

        return true; //hit이라서 true return
    }
}

void multi_cache::L2_write(cache_c* L1, cache_c* L2, addr_t address) {

}

void multi_cache::L1_evict(cache_c* L1, cache_c* L2, addr_t address, bool Is_read) {
    //기본적으로 이 상황은 접근한 tag와 존재하는 tag가 달라서 바꿔야 하는 상황 (allocate)
    //기본적으로 전부 valid임은 보장됨!!! dirty 여부만 확인하기

    int idx_block_1 = address % (L1->m_num_sets * L1->m_line_size);
    int idx_1 = idx_block_1 / (L1->m_line_size);
    int tag_1 = address / (L1->m_num_sets * L1->m_line_size);

    int idx_block_2 = address % (L2->m_num_sets * L2->m_line_size);
    int idx_2 = idx_block_2 / (L2->m_line_size);
    int tag_2 = address / (L2->m_num_sets * L2->m_line_size);

    int evicted_tag;

    if (Is_read) {
        //밑에서 읽어오는데 해당 idx가 valid라서 그 idx를 삭제하고 읽어온 값으로 update

        if (L1->m_set[idx_1]->m_entry[0].m_dirty == true) {
            evicted_tag = L1->m_set[idx_1]->m_entry[0].m_tag; //원래 tag
            //해당 block L2에 써줘야함
            L1->m_set[idx_1]->m_entry[0].m_dirty = false;
            L1->m_set[idx_1]->m_entry[0].m_tag = tag_1;
            L1->m_set[idx_1]->m_entry[0].m_valid = valid;

            ////////////////????
            //evicted tag에 해당하는 address로 L2에 접근해야함...
            int from_tag_address = (L2->m_num_sets * L2->m_line_size) * evicted_tag;

        }
        else {
            //clean이면 그냥 지워주면 됨
            L1->m_set[idx_1]->m_entry[0].m_dirty = false;
            L1->m_set[idx_1]->m_entry[0].m_tag = tag_1;
            L1->m_set[idx_1]->m_entry[0].m_valid = valid;
        }



        if (L1->m_set[idx_1]->m_entry[0].m_dirty == true) {
            //지워지는게 dirty
            evicted_tag = L1->m_set[idx_1]->m_entry[0].m_tag;

        }
        else { // L1에서 지워지는게 clean... 그냥 지우면 됨
            L1->m_set[idx_1]->m_entry[0].m_dirty = false;
            L1->m_set[idx_1]->m_entry[0].m_tag = tag_1;
            L1->m_set[idx_1]->m_entry[0].m_valid = valid;
        }
    }
}
*/

void multi_cache::print_status(void) {
    L1_I_cache->L1_print();
    std::cout << std::endl;
    L1_D_cache->L1_print();
    std::cout << std::endl;
    L2_cache->L2_print();
    std::cout << std::endl;
}