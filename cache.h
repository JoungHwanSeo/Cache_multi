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

    int order; // �ֱٿ� ���Ǿ������� ���� ����... ó���� ��� 0���� �ʱ�ȭ

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

    int MRU_num; // �ش� set���� MRU�� num
};

enum hit { //valid�� ��츸 evict
    READ_HIT, READ_MISS_INVALID, READ_MISS_VALID_DIRTY, READ_MISS_VALID_CLEAN,
    WRITE_HIT, WRITE_MISS_INVALID, WRITE_MISS_VALID_DIRTY, WRITE_MISS_VALID_CLEAN,
    HIT, MISS, //for only result�Լ�!
    MISS_INVALID, MISS_VALID_DIRTY, MISS_VALID_CLEAN
};

///////////////////////////////////////////////////////////////////
class cache_c
{
public:
    cache_c(std::string name, int num_set, int assoc, int line_size);
    ~cache_c();
    hit access(addr_t address, int access_type, bool MRU_update = true);
    //update�� false�� MRU�� �״��... true�̸� ���� ����

    void print_stats(void);

    //�ش� address ���ٽ� hit�̸� hit asseo�� �ش� way # return
    bool Is_hit(addr_t address, int* hit_asso);

    int find_evict_way(addr_t address);
    //�̰� ����Ϸ��� �ش� idx�� set�� ��� valid�ϴٴ� ������ �־����!

    bool Is_Exist_Invalid(addr_t address, int* invalid_asso);
    //�ش� idx�� set�� Invalid�����ϸ� true�̰� �ش� asso #�� �־���

    ////////////////////////���� ����!!!
    hit read(addr_t address, int* evicted_tag_address);

    hit write(addr_t address, int* evicted_tag_address);

    hit write_back(addr_t address, int* evicted_tag_address); //eviction���κ��� write, �� access�� LRU����x
    //�̰� L2�� ����� ���� ���ο� ��

    hit only_access(addr_t address);
    //access num�̶� hit num/miss num ����

    hit only_access_more(addr_t address);
    //statistics ��ȭ���� �ʰ�, Valid���� �˷���

    hit only_read(addr_t address, int* evicted_tag_address);
    //write���� L2���� miss�϶� ��

    void Invalidate(addr_t address);

    friend class multi_cache;

private:
    std::string m_name;     // cache name
    int m_num_sets;         // number of sets
    int m_line_size;        // cache line size

    int c_assoc;            //�� way�ΰ�... ������ �߰�

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
    //�⺻������ MRU update..
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
///�����
hit cache_c::access(addr_t address, int access_type, bool MRU_update) {
    ////////////////////////////////////////////////////////////////////
    // TODO: Write the code to implement this function

      //�̰� ���ϴ� logic �´��� Ȯ���ϱ�...///////////////////////////////
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
    int order_; //LRUã�� �� ���
    int LRU_idx; //ã�� LRU�� idx

    m_num_accesses++; //�����ϱ⸸ �ϸ� �ϴ� +1

    if (MRU_update) {
        m_set[idx]->MRU_num--; //�ش� set�� MRU number����
    }

    //���� ����
    switch (access_type) {
    case 0: // read
        m_num_reads++; //�ϴ� read�̹Ƿ� +1


        for (int i = 0; i < c_assoc; i++) {
            //�� idx�� �����ϴ� set���� asso����
            entry = m_set[idx]->m_entry[i];
            if (m_set[idx]->m_entry[i].m_valid == true && m_set[idx]->m_entry[i].m_tag == tag) {
                m_num_hits++;
                hit = true;

                if (MRU_update) {
                    m_set[idx]->m_entry[i].order = m_set[idx]->MRU_num; //���ҵ� MRU_num�� ��
                }
                //order�� ���� ������ MRU...
                //order�� ���� ū�� LRU
                return READ_HIT;
                break;
            }
        }


        //miss�� ���
        if (hit == false) {
            m_num_misses++; //��� set�� ã�Ҵµ��� tag match ����
            //�� read miss
            //�� ��� �ش� set�� data�������� ���̹Ƿ� way�� �ϳ��� valid�Ǿ����
            //miss�� ��� �ֿ켱������ invalid�� ���� ���� ä��
            for (int i = 0; i < c_assoc; i++) {
                entry = m_set[idx]->m_entry[i];
                if (entry.m_valid == false) {
                    //invalid����� �̰� �ֿ켱������ ä��
                    m_set[idx]->m_entry[i].m_dirty = false; //read�̹Ƿ� dirty = 0
                    m_set[idx]->m_entry[i].m_valid = true; //������ ���������Ƿ� true
                    m_set[idx]->m_entry[i].m_tag = tag;
                    if (MRU_update) {
                        m_set[idx]->m_entry[i].order = m_set[idx]->MRU_num;
                    }

                    invalid_exist = true;
                    break;
                }
            }

            //miss�ε� ��� valid�� ���
            if (invalid_exist == false) {
                //���� ��� valid���ٸ� LRU�� ã�Ƽ� �װ� evict�ؾ���
                order_ = m_set[idx]->m_entry[0].order;
                LRU_idx = 0;
                //�� set�� �ִ� entry�� order�� ���� ū �� ã��
                for (int i = 1; i < c_assoc; i++) {
                    tmp_order = m_set[idx]->m_entry[i].order;
                    if (order_ < tmp_order) {
                        order_ = tmp_order;
                        LRU_idx = i;
                    }
                }

                entry = m_set[idx]->m_entry[LRU_idx];

                //for loop������ LRU�� ã��
                //�ش� entry�� evict�ϴµ�, �̰� dirty�̸� write back�ؾ���
                if (m_set[idx]->m_entry[LRU_idx].m_dirty == true) {
                    m_num_writebacks++;
                }
                //���� evict�� dirty �ƴϸ� write back �ʿ�
                //entry = m_set[idx]->m_entry[LRU_idx];
                m_set[idx]->m_entry[LRU_idx].m_dirty = false;
                m_set[idx]->m_entry[LRU_idx].m_valid = true;
                m_set[idx]->m_entry[LRU_idx].m_tag = tag; //���ο� tag�� �ٲ���

                //�� ó���� ���ҵ� MRU_num.. �̰� ���� ���� (MRU����)
                if (MRU_update) {
                    m_set[idx]->m_entry[LRU_idx].order = m_set[idx]->MRU_num;
                }

            }

        }
        break;

    case 1: //write

        //m_set[idx]->MRU_num--;

        set_MRU_num = m_set[idx]->MRU_num; // �ش� set�� MRU num�� ����

        m_num_writes++; //�ϴ� write�̹Ƿ�
        for (int i = 0; i < c_assoc; i++) {
            entry = m_set[idx]->m_entry[i];
            if (entry.m_valid && entry.m_tag == tag) {
                //���࿡ write hit�̸� ���� �ִ� �ּҿ� ���.. WB��� x
                m_num_hits++;
                m_set[idx]->m_entry[i].m_dirty = true;
                if (MRU_update) {
                    m_set[idx]->m_entry[i].order = set_MRU_num; // �ش� entry�� MRU�� �������
                }
                hit = true;
                break;
            }
        }

        //miss�� ���
        if (hit == false) {
            //�ش� idx�� ��� assoã�Ƶ� ������ -> write allocate
            m_num_misses++; //miss����
            for (int i = 0; i < c_assoc; i++) {
                entry = m_set[idx]->m_entry[i];
                if (entry.m_valid == false) {
                    //invalid�߽߰� �ֿ켱������ ä��
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

            //miss�ε� ��� valid�� ��� LRU�� ã�� evict�ؾ��� (allocate)
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
                //for loop������ LRU ã��
                entry = m_set[idx]->m_entry[LRU_idx]; //LRU�� entry

                //evict�ϴ� LRU�� dirty�̸� WB�ؾ���
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
        m_num_reads++; // instruction fetch�� �ϴ� read��
        //read�� �Ȱ���...?

        //�ش� set�� MRU number����
        //m_set[idx]->MRU_num--;

        for (int i = 0; i < c_assoc; i++) {
            //�� idx�� �����ϴ� set���� asso����
            entry = m_set[idx]->m_entry[i];
            if (m_set[idx]->m_entry[i].m_valid == true && m_set[idx]->m_entry[i].m_tag == tag) {
                m_num_hits++;
                hit = true;

                if (MRU_update) {
                    m_set[idx]->m_entry[i].order = m_set[idx]->MRU_num; //���ҵ� MRU_num�� ��
                }
                //order�� ���� ������ MRU...
                //order�� ���� ū�� LRU

                break;
            }
        }


        //miss�� ���
        if (hit == false) {
            m_num_misses++; //��� set�� ã�Ҵµ��� tag match ����
            //�� read miss
            //�� ��� �ش� set�� data�������� ���̹Ƿ� way�� �ϳ��� valid�Ǿ����
            //miss�� ��� �ֿ켱������ invalid�� ���� ���� ä��
            for (int i = 0; i < c_assoc; i++) {
                entry = m_set[idx]->m_entry[i];
                if (entry.m_valid == false) {
                    //invalid����� �̰� �ֿ켱������ ä��
                    m_set[idx]->m_entry[i].m_dirty = false; //read�̹Ƿ� dirty = 0
                    m_set[idx]->m_entry[i].m_valid = true; //������ ���������Ƿ� true
                    m_set[idx]->m_entry[i].m_tag = tag;
                    if (MRU_update) {
                        m_set[idx]->m_entry[i].order = m_set[idx]->MRU_num;
                    }

                    invalid_exist = true;
                    break;
                }
            }

            //miss�ε� ��� valid�� ���
            if (invalid_exist == false) {
                //���� ��� valid���ٸ� LRU�� ã�Ƽ� �װ� evict�ؾ���
                order_ = m_set[idx]->m_entry[0].order;
                LRU_idx = 0;
                //�� set�� �ִ� entry�� order�� ���� ū �� ã��
                for (int i = 1; i < c_assoc; i++) {
                    tmp_order = m_set[idx]->m_entry[i].order;
                    if (order_ < tmp_order) {
                        order_ = tmp_order;
                        LRU_idx = i;
                    }
                }

                entry = m_set[idx]->m_entry[LRU_idx];

                //for loop������ LRU�� ã��
                //�ش� entry�� evict�ϴµ�, �̰� dirty�̸� write back�ؾ���
                if (m_set[idx]->m_entry[LRU_idx].m_dirty == true) {
                    m_num_writebacks++;
                }
                //���� evict�� dirty �ƴϸ� write back �ʿ�
                //entry = m_set[idx]->m_entry[LRU_idx];
                m_set[idx]->m_entry[LRU_idx].m_dirty = false;
                m_set[idx]->m_entry[LRU_idx].m_valid = true;
                m_set[idx]->m_entry[LRU_idx].m_tag = tag; //���ο� tag�� �ٲ���

                if (MRU_update) {
                    //�� ó���� ���ҵ� MRU_num.. �̰� ���� ���� (MRU����)
                    m_set[idx]->m_entry[LRU_idx].order = m_set[idx]->MRU_num;
                }

            }

        }
        break;
    }
    ////////////////////////////////////////////////////////////////////
}
*/

