#include "common.h"
#include "memory.h"
#include <unordered_map>
#include <mutex>
#include <stack>
using namespace std;

//global mutex
mutex& get_mem_mutex() {
    static mutex mem_mutex;
    return mem_mutex;
}

//pointer -> size
inline unordered_map<float*, long>& get_allocated_mem() {
    static unordered_map<float*, long> map;
    return map;
}

//size->pointer
inline unordered_map<long, stack<float*> > &get_freed_mem() {
    static unordered_map<long, stack<float*> > map;
    return map;
}

void free_array(float* pt) {
    //lock
    auto& mutex = get_mem_mutex();
    mutex.lock();

    //get block's size
    auto& am = get_allocated_mem();
    auto& it = am.find(pt);
    assert(it != am.end());
    long size = am[pt];

    //get stack
    auto& fm = get_freed_mem();

    //init
    if (!fm.count(size)) {
        fm[size] = stack<float*>();
    }

    //push into stack
    fm[size].push(pt);

    //unlock
    mutex.unlock();
}


float* alloc_array(long size) {
    //lock
    auto& mutex = get_mem_mutex();
    mutex.lock();

    //get freed block
    auto& map = get_freed_mem();
    auto& it = map.find(size);

    if (it == map.end() || it->second.size() == 0){
        //no freed block
        //cout << "allocate new memory: " << size << endl;;
        const int block_size = 20 * 1024 * 1024;
        const long times = std::min(1000L, std::max(1L, block_size / size));
        auto pt = new float[size*times];
        CHECK(pt != NULL);

        //recored pointers
        auto& am = get_allocated_mem();
        for (int i = 0; i < times; ++i){
            am[pt + i*size] = size;
        }

        mutex.unlock();

        //free unused
        for (int i = 1; i < times; ++i){
            free_array(pt + i*size);
        }

        //return new block
        return pt;
    }
    else {
        //get and pop block
        auto mem = it->second.top();
        it->second.pop();

        //return this block
        mutex.unlock();
        return mem;
    }
}



//=========================== arraykd =============================

int arraykd::new_id(){
    auto& g_mem_mutex = get_mem_mutex();
    static int sid = -1;
    g_mem_mutex.lock();
    ++sid;
    g_mem_mutex.unlock();
    return sid;
}


arraykd::arraykd(int size) {
    init(size);
    this->m_pmeta->dim = 1;
    this->m_pmeta->dimk[0] = size;
    copy_meta();
}

arraykd::arraykd(const std::vector<int> dims) {
    assert(dims.size() != 0);
    assert(dims.size() < 20);
    int size = 1;
    for (int i = 0; i < (int) dims.size(); ++i){
        size *= dims[i];
    }
    init(size);
    this->m_pmeta->dim = (int) dims.size();
    for (int i = 0; i < this->m_pmeta->dim; ++i){
        this->m_pmeta->dimk[i] = (int) dims[i];
    }
    copy_meta();
}



void arraykd::init(int size) {
    const int offset = sizeof(array_meta) / sizeof(float) +1;
    float* pmem = alloc_array(size + offset);
    this->m_pmeta = (array_meta*) pmem;
    this->m_pmeta->data = pmem + offset;
    this->m_pmeta->id = arraykd::new_id();
    this->m_pmeta->size = size;
    this->m_pmeta->counter = 1;
}

arraykd::arraykd(const arraykd& arr) {
    auto& g_mem_mutex = get_mem_mutex();
    this->m_pmeta = arr.m_pmeta;
    g_mem_mutex.lock();
    this->m_pmeta->counter += 1;
    g_mem_mutex.unlock();
    this->copy_meta();
}

arraykd::~arraykd() {
    destroy();
}

void arraykd::destroy(){
    auto& g_mem_mutex = get_mem_mutex();
    g_mem_mutex.lock();
    this->m_pmeta->counter -= 1;
    g_mem_mutex.unlock();
    if (this->m_pmeta->counter == 0) {
        free_array((float*)this->m_pmeta);
    }
}


block_ptr block_factory::get_block(const string& id) {
    if (m_cache.count(id) == 0) {
        m_cache[id] = block::new_block();
    }
    return m_cache[id];
}

vector<block_ptr> block_factory::get_blocks(const vector<string>& ids){
    vector<block_ptr> blocks;
    for (auto& id : ids){
        blocks.push_back(this->get_block(id));
    }
    return blocks;
}
