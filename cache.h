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

enum hit { //valid인 경우만 evict
    READ_HIT, READ_MISS_INVALID, READ_MISS_VALID_DIRTY, READ_MISS_VALID_CLEAN,
    WRITE_HIT, WRITE_MISS_INVALID, WRITE_MISS_VALID_DIRTY, WRITE_MISS_VALID_CLEAN,
    HIT, MISS, //for only result함수!
    MISS_INVALID, MISS_VALID_DIRTY, MISS_VALID_CLEAN
};

///////////////////////////////////////////////////////////////////
class cache_c
{
public:
    cache_c(std::string name, int num_set, int assoc, int line_size);
    ~cache_c();
    hit access(addr_t address, int access_type, bool MRU_update = true);
    //update가 false면 MRU는 그대로... true이면 정상 접근

    void print_stats(void);

    //해당 address 접근시 hit이면 hit asseo에 해당 way # return
    bool Is_hit(addr_t address, int* hit_asso);

    int find_evict_way(addr_t address);
    //이거 사용하려면 해당 idx의 set이 모두 valid하다는 가정이 있어야함!

    bool Is_Exist_Invalid(addr_t address, int* invalid_asso);
    //해당 idx의 set에 Invalid존재하면 true이고 해당 asso #를 넣어줌

    ////////////////////////새로 시작!!!
    hit read(addr_t address, int* evicted_tag_address);

    hit write(addr_t address, int* evicted_tag_address);

    hit write_back(addr_t address, int* evicted_tag_address); //eviction으로부터 write, 즉 access나 LRU갱신x
    //이건 L2가 사용할 것을 염두에 둠

    hit only_access(addr_t address);
    //access num이랑 hit num/miss num 증가

    hit only_access_more(addr_t address);
    //statistics 변화하지 않고, Valid까지 알려줌

    hit only_read(addr_t address, int* evicted_tag_address);
    //write에서 L2까지 miss일때 씀

    void Invalidate(addr_t address);

    friend class multi_cache;

private:
    std::string m_name;     // cache name
    int m_num_sets;         // number of sets
    int m_line_size;        // cache line size

    int c_assoc;            //몇 way인가... 개인적 추가

    cache_set_c** m_set;    // cache data structure 

    // cache statistics
    int m_num_accesses;
    int m_num_hits;
    int m_num_misses;
    int m_num_reads;
    int m_num_writes;
    int m_num_writebacks;

    int m_num_invalidate;
    int m_num_writebacks_to_mem;

};


class multi_cache {
public:
    multi_cache(std::string name,int L1_num_set,int L1_line_size, int L2_num_set,int L2_line_size, int L2_assoc);
    ~multi_cache();
    void access(addr_t address, int access_type);
    //기본적으로 MRU update..
    void print_status(void);


private:
    cache_c* L1_I_cache;
    cache_c* L1_D_cache;
    cache_c* L2_cache;

    int m_num_sets_1;         // number of sets
    int m_line_size_1;        // cache line size

    int m_num_sets_2;         // number of sets
    int m_line_size_2;        // cache line size

    std::string m_name;

    // cache statistics
    int m_num_accesses;
    int m_num_hits;
    int m_num_misses;
    int m_num_reads;
    int m_num_writes;
    int m_num_writebacks;
};

#endif // !__CACHE_H__ 





/*
///지우기
hit cache_c::access(addr_t address, int access_type, bool MRU_update) {
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
                if (MRU_update) {
                    m_set[idx]->m_entry[LRU_idx].order = m_set[idx]->MRU_num;
                }

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
                if (MRU_update) {
                    m_set[idx]->m_entry[LRU_idx].order = set_MRU_num;
                }
            }
        }
        break;
    case 2: //instruction fetch
        m_num_reads++; // instruction fetch도 일단 read임
        //read랑 똑같나...?

        //해당 set의 MRU number감소
        //m_set[idx]->MRU_num--;

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

                if (MRU_update) {
                    //맨 처음에 감소된 MRU_num.. 이게 제일 작음 (MRU가됨)
                    m_set[idx]->m_entry[LRU_idx].order = m_set[idx]->MRU_num;
                }

            }

        }
        break;
    }
    ////////////////////////////////////////////////////////////////////
}
*/

