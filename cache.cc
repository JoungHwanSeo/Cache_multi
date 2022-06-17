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

                    return READ_MISS_INVALID;
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

                bool Is_evict_dirty = false;

                //for loop������ LRU�� ã��
                //�ش� entry�� evict�ϴµ�, �̰� dirty�̸� write back�ؾ���
                if (m_set[idx]->m_entry[LRU_idx].m_dirty == true) {
                    m_num_writebacks++;
                    Is_evict_dirty = true;
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
                
                if (Is_evict_dirty)
                    return READ_MISS_VALID_DIRTY;
                else
                    return READ_MISS_VALID_CLEAN;
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
                return WRITE_HIT;
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
                    return WRITE_MISS_INVALID;
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
                    //evict�ϴ� LRU�� dirty�̸� WB�ؾ���
                    
                }

                if (Is_evict_dirty)
                    return READ_MISS_VALID_DIRTY;
                else
                    return READ_MISS_VALID_CLEAN;
            }
        }
        break;
    case 2: //instruction fetch
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

                    return READ_MISS_INVALID;
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

                bool Is_evict_dirty = false;

                //for loop������ LRU�� ã��
                //�ش� entry�� evict�ϴµ�, �̰� dirty�̸� write back�ؾ���
                if (m_set[idx]->m_entry[LRU_idx].m_dirty == true) {
                    m_num_writebacks++;
                    Is_evict_dirty = true;
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
    //�ٵ� �̰� ����Ϸ��� ��� �� valid��� ������ �־����!!!!!!!

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


///���� ����!!!!!!!
hit cache_c::read(addr_t address, int* evicted_tag_address) {
    int idx_block = address % (m_num_sets * m_line_size);
    int idx = idx_block / (m_line_size);
    int tag = address / (m_num_sets * m_line_size);

    m_num_accesses++; //�����̴ϱ� ++
    m_num_reads++;

    m_set[idx]->MRU_num--;
        
    //hit�� ���
    for (int i = 0; i < c_assoc; i++) {
        if (m_set[idx]->m_entry[i].m_valid == true && m_set[idx]->m_entry[i].m_tag == tag) {
            m_num_hits++;
            //MRU update�ص� �Ǵ� ��Ȳ�̸�
            m_set[idx]->m_entry[i].order = m_set[idx]->MRU_num; //���ҵ� MRU_num�� ��
            return READ_HIT;
        }
    }

    m_num_misses++;
    for (int i = 0; i < c_assoc; i++) {
        if (m_set[idx]->m_entry[i].m_valid == false) { //���� invalid�׸� ������

            m_set[idx]->m_entry[i].m_dirty = false; //read�̹Ƿ� dirty = 0
            m_set[idx]->m_entry[i].m_valid = true; //������ ���������Ƿ� true
            m_set[idx]->m_entry[i].m_tag = tag;

            m_set[idx]->m_entry[i].order = m_set[idx]->MRU_num; //���ҵ� MRU_num�� ��
            return READ_MISS_INVALID;
        }
    }
    //���⼭�� �ش� set�� ��� valid���� �����ǹǷ� find_evict��� ����
    int LRU_idx = find_evict_way(address); // �ش� idx�� set���� LRU ã��
    int evict_tag = m_set[idx]->m_entry[LRU_idx].m_tag;
    //evict�Ǵ� �κ��� clean���� ����
    bool Is_evict_dirty = m_set[idx]->m_entry[LRU_idx].m_dirty;

    m_set[idx]->m_entry[LRU_idx].m_dirty = false;
    m_set[idx]->m_entry[LRU_idx].m_valid = true;
    m_set[idx]->m_entry[LRU_idx].m_tag = tag; //���ο� tag�� �ٲ���

    *evicted_tag_address = evict_tag * (m_num_sets * m_line_size) + (idx * m_line_size); // �Լ��� ���� ���ڿ� ���� ����
    m_set[idx]->m_entry[LRU_idx].order = m_set[idx]->MRU_num; //���ҵ� MRU_num�� ��
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
    m_set[idx]->MRU_num--; //MRU update����
    for (int i = 0; i < c_assoc; i++) {
        if (m_set[idx]->m_entry[i].m_valid == true && m_set[idx]->m_entry[i].m_tag == tag) {
            //���� �ּҿ� ����� ��
            m_num_hits++;
            m_set[idx]->m_entry[i].m_dirty = true;
            m_set[idx]->m_entry[i].order = m_set[idx]->MRU_num; //MRU update
            return WRITE_HIT;
        }
    }
    //miss�� ���
    m_num_misses++;
    for (int i = 0; i < c_assoc; i++) {
        if (m_set[idx]->m_entry[i].m_valid == false) {
            //invalid �ֿ켱������ ä��
            m_set[idx]->m_entry[i].m_dirty = true;
            m_set[idx]->m_entry[i].m_tag = tag;
            m_set[idx]->m_entry[i].m_valid = true;
            m_set[idx]->m_entry[i].order = m_set[idx]->MRU_num; //MRU update
            return WRITE_MISS_INVALID;
        }
    }
    //���⼭�� �ش� set�� ��� valid���� �����ǹǷ� find_evict��� ����
    int LRU_idx = find_evict_way(address); // �ش� set���� LRU wayã��
    int evict_tag = m_set[idx]->m_entry[LRU_idx].m_tag;
    bool Is_evict_dirty = m_set[idx]->m_entry[LRU_idx].m_dirty;
    int evict_address_from_tag = evict_tag * (m_num_sets * m_line_size) + (idx * m_line_size);
    *evicted_tag_address = evict_address_from_tag; // evict�Ǵ� �ּ�(block�� 00������...)

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
        if (m_set[idx]->m_entry[i].m_valid == false) { //invalid�ϳ��� ã����
            return MISS_INVALID;
        }
    }
    //������� ���� ��� valid���� �����
    int LRU_idx = find_evict_way(address);
    bool Is_evicted_dirty = m_set[idx]->m_entry[LRU_idx].m_dirty;
    if (Is_evicted_dirty == true)
        return MISS_VALID_DIRTY;
    else
        return MISS_VALID_CLEAN;
}

hit cache_c::write_back(addr_t address, int* evicted_tag_address) {


    //�����
    //m_num_writes++;
    //

    int idx_block = address % (m_num_sets * m_line_size);
    int idx = idx_block / (m_line_size);
    int tag = address / (m_num_sets * m_line_size);
    for (int i = 0; i < c_assoc; i++) {
        if (m_set[idx]->m_entry[i].m_valid == true && m_set[idx]->m_entry[i].m_tag == tag) {
            //���� �ּҿ� ����� ��
            //m_num_hits++;
            //��跮 �ٲٸ� �ȵ�!
            m_set[idx]->m_entry[i].m_dirty = true;
            return WRITE_HIT;
        }
    }
    //miss�� ���
    for (int i = 0; i < c_assoc; i++) {
        if (m_set[idx]->m_entry[i].m_valid == false) {
            //invalid �ֿ켱������ ä��
            m_set[idx]->m_entry[i].m_dirty = true;
            m_set[idx]->m_entry[i].m_tag = tag;
            m_set[idx]->m_entry[i].m_valid = true;
            return WRITE_MISS_INVALID;
        }
    }
    //���⼭�� �ش� set�� ��� valid���� �����ǹǷ� find_evict��� ����
    int LRU_idx = find_evict_way(address); // �ش� set���� LRU wayã��
    int evict_tag = m_set[idx]->m_entry[LRU_idx].m_tag;
    bool Is_evict_dirty = m_set[idx]->m_entry[LRU_idx].m_dirty;
    int evict_address_from_tag = evict_tag * (m_num_sets * m_line_size) + (idx * m_line_size);
    *evicted_tag_address = evict_address_from_tag; // evict�Ǵ� �ּ�(block�� 00������...)

    m_set[idx]->m_entry[LRU_idx].m_dirty = true;
    m_set[idx]->m_entry[LRU_idx].m_tag = tag;
    m_set[idx]->m_entry[LRU_idx].m_valid = true;
    if (Is_evict_dirty) {
        m_num_writebacks++; // L2������ Write Back...... �³�....?
        return WRITE_MISS_VALID_DIRTY;
    }
    else {
        return WRITE_MISS_VALID_CLEAN;
    }
}

//only_read���� MRU ���ſ���??????????????????????????????????????????????????????
//L1,L2 ��� write miss���� L2�� �ش� �׸��� ����� ������ �� instruction���� �ش� �ּҸ� �����Ѱ� �ƴ�
hit cache_c::only_read(addr_t address, int* evicted_tag_address) {
    int idx_block = address % (m_num_sets * m_line_size);
    int idx = idx_block / (m_line_size);
    int tag = address / (m_num_sets * m_line_size);

    //m_num_accesses++; //�����̴ϱ� ++
    //m_num_reads++;

    //m_set[idx]->MRU_num--;

    //hit�� ���
    for (int i = 0; i < c_assoc; i++) {
        if (m_set[idx]->m_entry[i].m_valid == true && m_set[idx]->m_entry[i].m_tag == tag) {
            //m_num_hits++;
            //MRU update�ص� �Ǵ� ��Ȳ�̸�
            //m_set[idx]->m_entry[i].order = m_set[idx]->MRU_num; //���ҵ� MRU_num�� ��
            return READ_HIT;
        }
    }
    //�ٵ� ��� �̰� ���°Ÿ� L2���� miss�� �� ���°Ŷ� ���� hit�� ���� �� ���� ���µ�

    //m_num_misses++;
    for (int i = 0; i < c_assoc; i++) {
        if (m_set[idx]->m_entry[i].m_valid == false) { //���� invalid�׸� ������

            m_set[idx]->m_entry[i].m_dirty = false; //read�̹Ƿ� dirty = 0
            m_set[idx]->m_entry[i].m_valid = true; //������ ���������Ƿ� true
            m_set[idx]->m_entry[i].m_tag = tag;

            //m_set[idx]->m_entry[i].order = m_set[idx]->MRU_num; //���ҵ� MRU_num�� ��
            return READ_MISS_INVALID;
        }
    }
    //���⼭�� �ش� set�� ��� valid���� �����ǹǷ� find_evict��� ����
    int LRU_idx = find_evict_way(address); // �ش� idx�� set���� LRU ã��
    int evict_tag = m_set[idx]->m_entry[LRU_idx].m_tag;
    //evict�Ǵ� �κ��� clean���� ����
    bool Is_evict_dirty = m_set[idx]->m_entry[LRU_idx].m_dirty;

    m_set[idx]->m_entry[LRU_idx].m_dirty = false;
    m_set[idx]->m_entry[LRU_idx].m_valid = true;
    m_set[idx]->m_entry[LRU_idx].m_tag = tag; //���ο� tag�� �ٲ���

    *evicted_tag_address = evict_tag * (m_num_sets * m_line_size) + (idx * m_line_size); // �Լ��� ���� ���ڿ� ���� ����
    //m_set[idx]->m_entry[LRU_idx].order = m_set[idx]->MRU_num; //���ҵ� MRU_num�� ��
    //������ ����������...............������������

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
            //�ش� �׸��� �����ϸ� invalidate
            m_num_invalidate++; //���� ���� �;� invalidate�� �����!!!!!!!

            m_set[idx]->m_entry[i].m_valid = false;
            if (m_set[idx]->m_entry[i].m_dirty == true) { //dirty�̸� write_back
                m_num_writebacks++; //�ٷ� �޸𸮷� WB.. L2���� ���ص� ok
                m_num_writebacks_to_mem++; //mem���� �ٷΰ��� WB
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
            return true; //invalid����
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
    m_num_accesses++; //���ٸ��� 1����

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
        if (L1_result == READ_HIT) {} //�Ұ� ���� ��!

        else if (L1_result == READ_MISS_INVALID || L1_result == READ_MISS_VALID_CLEAN) {
            //L1�� evict�Ǿ ���� WB���ϹǷ� L2�� �ٽ� �� ���ص� ��(Ȥ�� �׳� ��)
            L2_result = L2_cache->read(address, &evicted_address_2);
            if (L2_result == READ_MISS_VALID_CLEAN || L2_result == READ_MISS_VALID_DIRTY) {
                //���� L2���� evict�� �Ͼ�� ��� �ش� �ּҰ� L1�� �ִ��� Ȯ��
                L1_D_cache->Invalidate(evicted_address_2);

                //Invalidate�� L1_I�� ���ִ� �� ��������...?
                L1_I_cache->Invalidate(evicted_address_2);
            }
            //else�� ��� L2���� evict�Ǵ� �� ����!
        }

        else { //L1�� evicted block�� dirty�� ��,�� WB����� �ϴ� ���
            //�� ��쵵 WB�� ���� ����� �ϴ°� �ƴұ�??????
            //WB�� ���߿� �ϸ� L2�� �о�� ���� WB�� ����� ���� ��������..
            //Inclusivce�� ���� �ٵ� �׷��� L2���� invalidate L1�� �����ϱ� �ϳ�...
            //�� ���� ����� ������

            //�Ḹ �ƴѰ�?
            //

            L2_result = L2_cache->read(address, &evicted_address_2);


            ///�Ḹ L2 hit�ΰŶ� MISS_INVALID�� ���� �ص� ��??
            //�׷���...?

            if (L2_result == READ_HIT || L2_result == READ_MISS_INVALID) {
                WB_result = L2_cache->write_back(evicted_address, &evicted_address_WB); //������ L1 block ������ �����
                if (WB_result == WRITE_MISS_VALID_CLEAN || WB_result == WRITE_MISS_VALID_DIRTY) {
                    //���� WB ��� L2���� �� evict�Ǹ�
                    L1_D_cache->Invalidate(evicted_address_WB);

                    L1_I_cache->Invalidate(evicted_address_WB);
                }
            }

            else if (L2_result == READ_MISS_VALID_CLEAN || L2_result == READ_MISS_VALID_DIRTY) {
                //�ϴ� L2�� read��� evict...
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
        //write�� ���������� �ٽ� �����ؾ� �ҵ�!!!!!!!!!!!!!!!!!
        int L2_access_result;
        int L2_only_read_result;

        L1_result = L1_D_cache->write(address, &evicted_address);
        if (L1_result == WRITE_HIT) {} //���⼭ ��, �� �� ���� L2���� �ش� �׸� �ִٴ� �ǹ�!

        else if (L1_result == WRITE_MISS_INVALID || L1_result == WRITE_MISS_VALID_CLEAN){ //L1�� ���� �����Ƿ� L2���� �����
            //L1�� ���� ������ ������ �ش� �׸��� dirty�� �ƴ϶� WB�ʿ䰡 ����
            L2_access_result = L2_cache->only_access(address);

            if (L2_access_result == HIT) { //L1���� miss�ε� L2���� hit�� ���
                //�׳� ������ �Ǳ� �ϴµ� L2�� �����߰�, hit�̾����� L2 ��Ͽ� ���ܾ���
                //�ƹ�ư L2���� �׸��� �����Ƿ� ok��
            }

            else //L2 ���� ����� miss
            {
                L2_only_read_result = L2_cache->only_read(address, &evicted_address_2);
                //miss���� ���������Ƿ� �� ����� ���� hit�� �ƴ�
                //�ϴ� �о�ͼ� �׸��� �������

                if (L2_only_read_result == READ_MISS_INVALID) { //�о�� ��� L2 evict����
                    //���� �� �� ����!
                }

                else { //�о�� ��� L2 evict ���� ->L1 invalidate
                    L1_D_cache->Invalidate(evicted_address_2);

                    L1_I_cache->Invalidate(evicted_address_2);
                }
            }
        }

        else { //L1�� evict block�� dirty�� WB�������
            L2_access_result = L2_cache->only_access(address);

            if (L2_access_result == HIT) {
                //L1���� evicted block�� address�� L2�� WB�������
                WB_result = L2_cache->write_back(evicted_address, &evicted_address_WB);
                //�� �� L2�� WB��� evict�� ������ Invalidate... ���⼭ L1�� �� dirty�̸� �ٷ� mem���� ��
                if (WB_result == WRITE_MISS_VALID_CLEAN || WB_result == WRITE_MISS_VALID_DIRTY) {
                    L1_D_cache->Invalidate(evicted_address_WB);

                    L1_I_cache->Invalidate(evicted_address_WB);
                }
            }

            else { // L2�� miss�� ��� (L1�� �̹� evict�ǰ� �װ� dirty��....)

                //�ϴ� L1���� evict�� dirty�� L2�� �ֿ켱������ ���� ���ֱ�!!!!!!!!
                WB_result = L2_cache->write_back(evicted_address, &evicted_address_WB);
                if (WB_result == WRITE_MISS_VALID_CLEAN || WB_result == WRITE_MISS_VALID_DIRTY) {
                    L1_D_cache->Invalidate(evicted_address_WB);

                    L1_I_cache->Invalidate(evicted_address_WB);
                }

                //�ϴ� L2�� WB�� ������ �޸𸮿��� �о��, �� L1�� �ִ� �׸��� L2���� �������
                L2_only_read_result = L2_cache->only_read(address, &evicted_address_2);

                //�ϴ� L2 miss���� ���������Ƿ� ����� ���� hit�� �ƴϴ�
                if (L2_only_read_result == READ_MISS_INVALID) {
                    //�� �� ����!!!
                }

                else { //�о�� ��� L2���� evict �� ���� ���� ->L1 invalidate
                    L1_D_cache->Invalidate(evicted_address_2);

                    L1_I_cache->Invalidate(evicted_address_2);
                }
            }
        }
            
        break;

    case 2: //instruction fetch
        L1_result = L1_I_cache->read(address, &evicted_address);
        if (L1_result == READ_HIT) {} //�Ұ� ���� ��!

        else if (L1_result == READ_MISS_INVALID || L1_result == READ_MISS_VALID_CLEAN) {
            //L1�� evict�Ǿ ���� WB���ϹǷ� L2�� �ٽ� �� ���ص� ��(Ȥ�� �׳� ��)
            L2_result = L2_cache->read(address, &evicted_address_2);
            if (L2_result == READ_MISS_VALID_CLEAN || L2_result == READ_MISS_VALID_DIRTY) {
                //���� L2���� evict�� �Ͼ�� ��� �ش� �ּҰ� L1�� �ִ��� Ȯ��
                L1_D_cache->Invalidate(evicted_address_2);

                //Invalidate�� L1_I�� ���ִ� �� ��������...?
                L1_I_cache->Invalidate(evicted_address_2);
            }
            //else�� ��� L2���� evict�Ǵ� �� ����!
        }

        else { //L1�� evicted block�� dirty�� ��,�� WB����� �ϴ� ���
            //�� ��쵵 WB�� ���� ����� �ϴ°� �ƴұ�??????
            //WB�� ���߿� �ϸ� L2�� �о�� ���� WB�� ����� ���� ��������..
            //Inclusivce�� ���� �ٵ� �׷��� L2���� invalidate L1�� �����ϱ� �ϳ�...

            L2_result = L2_cache->read(address, &evicted_address_2);


            ///�Ḹ L2 hit�ΰŶ� MISS_INVALID�� ���� �ص� ��??
            //�׷���...?

            if (L2_result == READ_HIT || L2_result == READ_MISS_INVALID) {
                WB_result = L2_cache->write_back(evicted_address, &evicted_address_WB); //������ L1 block ������ �����
                if (WB_result == WRITE_MISS_VALID_CLEAN || WB_result == WRITE_MISS_VALID_DIRTY) {
                    //���� WB ��� L2���� �� evict�Ǹ�
                    L1_D_cache->Invalidate(evicted_address_WB);

                    L1_I_cache->Invalidate(evicted_address_WB);
                }
            }

            else if (L2_result == READ_MISS_VALID_CLEAN || L2_result == READ_MISS_VALID_DIRTY) {
                //�ϴ� L2�� read��� evict...
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

//hit�̸� true return
/*
* 
* void multi_cache::access(addr_t address, int access_type) {
    m_num_accesses++; //���ٸ��� 1����

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
    case 0: //read     L1_D_cache���
        //�켱 L1�� ����
        //L1�� MRU�������� �ʿ� ���� ��... Direct��

        //�̸� L1�� tag�� dirty���� �޾ƿ�
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
        //L1���� read miss�� ��� idx_1���� miss��
        else {
            L2_result = L2_cache->access_result(address, access_type); //L2�� read
            if (L2_result == READ_HIT) {
                m_num_hits++;
                L2_cache->access(address, access_type); //update
                //L2���� hit�̰� �ش� ���� L1�� ���ָ� ��
                if (L1_result == READ_MISS_VALID) {
                    //���� L1�� �ش� idx�� valid������ �ش� block�� evict
                    //L1�� miss�� �κ��� tag�� dirty���� (�ش� idx�� if���ǵ��� valid)
                    evicted_tag = L1_D_cache->m_set[idx_1]->m_entry[0].m_tag;
                    evicted_dirty = L1_D_cache->m_set[idx_1]->m_entry[0].m_dirty;

                    ///    <L1�� L2���� ������ �� ���� �κ�>
                    L1_D_cache->m_set[idx_1]->m_entry[0].m_dirty = false;
                    //L2���� �������� ���̹Ƿ� clean
                    L1_D_cache->m_set[idx_1]->m_entry[0].m_tag = tag_1;
                    L1_D_cache->m_set[idx_1]->m_entry[0].m_valid = true;
                    //�ش� tag�� ���� ������ idx���� L2���� ã�� ����

                    if (evicted_dirty == true) { //�� ��츸 �ٽ� L2�� ���ư��� ���캽!
                        //evict�Ǵ� L1 block�� dirty�̸� L2�� �������
                        //evict�Ǵ� L1 block�� tag �˾ƾ� ��
                        //evict�Ǵ� L1 block�� idx�� ó�� ���� idx�� ����
                        //L1 miss�̹Ƿ� ���� tag�� ����� tag�� �ٸ�

                        // <L2 ó�� �κ�> L1���� dirty���� block�� L2�� �������
                        bool return_L2_result = L2_cache->access_result(address, 1);
                        if (return_L2_result == WRITE_HIT)
                            L2_cache->access(address, 1, false);
                        else if (return_L2_result == WRITE_MISS_INVALID) {
                            L2_cache->access(address, 1, false);
                        }
                        else { //L2���� evict�ؾ� �ϴ� ���
                            int L2_evict_way = L2_cache->find_evict_way(address);
                            //invalidate logic
                            if (L2_cache->m_set[idx_2]->m_entry[L2_evict_way].m_tag == L1_D_cache->m_set[idx_1]->m_entry[0].m_tag) {
                                L1_D_cache->m_set[idx_1]->m_entry[0].m_valid = false;
                            }
                            L2_cache->access(address, 1, false);
                        }
                        //�ٵ� L2���� evict�ؾ��ϴ� ��쿡�� L1�� ���� invalidate logic�� �ʿ�...
                    }
                    else {
                        //L1���� evicted�Ǵ� �κ��� clean�� ��� �ش� block�� ���� �޸𸮿� ����
                        //�׳� ���ָ� ��
                        //L2�� ���� �� �� �ʿ� ����!... nothing...
                    }
                }
                else {
                    //L1 result���� invalid�� ��� L2���� ������ �� �׳� �ű⿡ ���� ��
                    L1_D_cache->m_set[idx_1]->m_entry[0].m_dirty = false;
                    //L2���� �������� ���̹Ƿ� clean
                    L1_D_cache->m_set[idx_1]->m_entry[0].m_tag = tag;
                    L1_D_cache->m_set[idx_1]->m_entry[0].m_valid = true;
                    //�ش� tag�� ���� ������ idx���� L2���� ã�� ����

                }
            }
            else {
                m_num_misses++; // �� �� miss�̹Ƿ� ��ü miss
                //L2������ Read miss�� ���
                //�ϴ� �޸𸮿��� data �Ѵ� ������
                if (L1_result == READ_MISS_INVALID) {
                    if (L2_result == READ_MISS_INVALID) {
                        L1_D_cache->access(address, access_type, true);
                        L2_cache->access(address, access_type, true);
                    }
                    else { //L2 idx�� ��� valid... �ϳ� evict�ؾ���
                        int L2_evict_way_f = L2_cache->find_evict_way(address);
                        //L2�� evict�׸�� L1�� evict�׸��� ������ L1 invalidate
                    }
                }
                else {
                    //L1�� Read miss valid -> evict�׸� ã�ƾ� ��
                }
            }
        }

        break;
    case 1: //write
        break;
    case 2: //Instruction fetch     L1_I_cache���
        break;
    }
}
* 
bool multi_cache::L1_read(cache_c* L1, cache_c* L2, addr_t address) {
    //�̰Ŵ� �׳�  �ۿ��� ���ִ°� ��������........ ��Ȳ���� ���� read���� ���� ����
    //���� �ȼ��� �̷���
    L1->m_num_accesses++; //�ϴ� L1 �����̹Ƿ�
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

    //�켱 �ش� address���� hit���� �Ǵ�
    if (L1->Is_hit(address, &hit_asso)) {
        L1->m_set[idx_1]->MRU_num--;
        L1->m_set[idx_1]->m_entry[hit_asso].order = L1->m_set[idx_1]->MRU_num;
        L1->m_num_hits++;
        return true; //hit�̶� true return
    }
    else { //miss�� ��� ��
        //L2���� hit
        if (L2_read(L1, L2, address)) {
            //L2���� hit�� �Ǿ� �ű⼭ ���� ���� L1�� �ٽ� ����
            if (L1->Is_Exist_Invalid(address, &invalid_asso)) {
                //���� L1�� �ش� �׸��� invalid
                L1->m_set[idx_1]->m_entry[invalid_asso].m_valid = true;
                L1->m_set[idx_1]->m_entry[invalid_asso].m_tag = tag_1;
                L1->m_set[idx_1]->m_entry[invalid_asso].m_dirty = false;
                //�ϴ� L2�� ���� �״�� ���Ƿ� L1�� clean L2�� dirty������..

                //MRU update
                L1->m_set[idx_1]->MRU_num--;
                L1->m_set[idx_1]->m_entry[invalid_asso].order = L1->m_set[idx_1]->MRU_num;
            }
            else {
                //L1�� �ش� �׸��� valid �� evict�ؾ���...

                //�� evict�Ǵ� �κ��� �� �ٸ� �Լ��� �����???
                L1_evict(L1, L2, address, true);

                //LRU_way = L1->find_evict_way(address); //���� ���� �Ⱦ��� wayã��

            }
        }
        else { //L2������ miss

        }
    }

}

//L1���� miss���⿡ ����� ������
bool multi_cache::L2_read(cache_c* L1, cache_c* L2, addr_t address) {
    L2->m_num_accesses++;//�ϴ� L2 �����̹Ƿ�
    L2->m_num_reads++;

    int idx_block_2 = address % (L2->m_num_sets * L2->m_line_size);
    int idx_2 = idx_block_2 / (L2->m_line_size);
    int tag_2 = address / (L2->m_num_sets * L2->m_line_size);
    int hit_asso;
    int invalid_asso;
    if (L2->Is_hit(address, &hit_asso)) {
        L2->m_set[idx_2]->MRU_num--;
        L2->m_set[idx_2]->m_entry[hit_asso].order = L2->m_set[idx_2]->MRU_num;
        //L2���� ���� ���� L1�� ����� ��

        return true; //hit�̶� true return
    }
}

void multi_cache::L2_write(cache_c* L1, cache_c* L2, addr_t address) {

}

void multi_cache::L1_evict(cache_c* L1, cache_c* L2, addr_t address, bool Is_read) {
    //�⺻������ �� ��Ȳ�� ������ tag�� �����ϴ� tag�� �޶� �ٲ�� �ϴ� ��Ȳ (allocate)
    //�⺻������ ���� valid���� �����!!! dirty ���θ� Ȯ���ϱ�

    int idx_block_1 = address % (L1->m_num_sets * L1->m_line_size);
    int idx_1 = idx_block_1 / (L1->m_line_size);
    int tag_1 = address / (L1->m_num_sets * L1->m_line_size);

    int idx_block_2 = address % (L2->m_num_sets * L2->m_line_size);
    int idx_2 = idx_block_2 / (L2->m_line_size);
    int tag_2 = address / (L2->m_num_sets * L2->m_line_size);

    int evicted_tag;

    if (Is_read) {
        //�ؿ��� �о���µ� �ش� idx�� valid�� �� idx�� �����ϰ� �о�� ������ update

        if (L1->m_set[idx_1]->m_entry[0].m_dirty == true) {
            evicted_tag = L1->m_set[idx_1]->m_entry[0].m_tag; //���� tag
            //�ش� block L2�� �������
            L1->m_set[idx_1]->m_entry[0].m_dirty = false;
            L1->m_set[idx_1]->m_entry[0].m_tag = tag_1;
            L1->m_set[idx_1]->m_entry[0].m_valid = valid;

            ////////////////????
            //evicted tag�� �ش��ϴ� address�� L2�� �����ؾ���...
            int from_tag_address = (L2->m_num_sets * L2->m_line_size) * evicted_tag;

        }
        else {
            //clean�̸� �׳� �����ָ� ��
            L1->m_set[idx_1]->m_entry[0].m_dirty = false;
            L1->m_set[idx_1]->m_entry[0].m_tag = tag_1;
            L1->m_set[idx_1]->m_entry[0].m_valid = valid;
        }



        if (L1->m_set[idx_1]->m_entry[0].m_dirty == true) {
            //�������°� dirty
            evicted_tag = L1->m_set[idx_1]->m_entry[0].m_tag;

        }
        else { // L1���� �������°� clean... �׳� ����� ��
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