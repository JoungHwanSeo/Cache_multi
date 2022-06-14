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
            //�� ó�� order�� ��� 0���� �ʱ�ȭ
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

    //���� ����
    switch (access_type) {
    case 0: // read

        //�ش� set�� MRU number����
        m_set[idx]->MRU_num--;

        m_num_reads++; //�ϴ� read�̹Ƿ� +1
        for (int i = 0; i < c_assoc; i++) {
            //�� idx�� �����ϴ� set���� asso����
            entry = m_set[idx]->m_entry[i];
            if (m_set[idx]->m_entry[i].m_valid == true && m_set[idx]->m_entry[i].m_tag == tag) {
                m_num_hits++;
                hit = true;

                m_set[idx]->m_entry[i].order = m_set[idx]->MRU_num; //���ҵ� MRU_num�� ��
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
                    m_set[idx]->m_entry[i].order = m_set[idx]->MRU_num;

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
                m_set[idx]->m_entry[LRU_idx].order = m_set[idx]->MRU_num;

            }

        }
        break;

    case 1: //write

        m_set[idx]->MRU_num--;

        set_MRU_num = m_set[idx]->MRU_num; // �ش� set�� MRU num�� ����

        m_num_writes++; //�ϴ� write�̹Ƿ�
        for (int i = 0; i < c_assoc; i++) {
            entry = m_set[idx]->m_entry[i];
            if (entry.m_valid && entry.m_tag == tag) {
                //���࿡ write hit�̸� ���� �ִ� �ּҿ� ���.. WB��� x
                m_num_hits++;
                m_set[idx]->m_entry[i].m_dirty = true;
                m_set[idx]->m_entry[i].order = set_MRU_num; // �ش� entry�� MRU�� �������
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
                    m_set[idx]->m_entry[i].order = set_MRU_num;

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
                m_set[idx]->m_entry[LRU_idx].order = set_MRU_num;
            }
        }
        break;
    case 2: //instruction fetch
        m_num_reads++; // instruction fetch�� �ϴ� read��
        //read�� �Ȱ���...?

        //�ش� set�� MRU number����
        m_set[idx]->MRU_num--;

        for (int i = 0; i < c_assoc; i++) {
            //�� idx�� �����ϴ� set���� asso����
            entry = m_set[idx]->m_entry[i];
            if (m_set[idx]->m_entry[i].m_valid == true && m_set[idx]->m_entry[i].m_tag == tag) {
                m_num_hits++;
                hit = true;

                m_set[idx]->m_entry[i].order = m_set[idx]->MRU_num; //���ҵ� MRU_num�� ��
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
                    m_set[idx]->m_entry[i].order = m_set[idx]->MRU_num;

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

