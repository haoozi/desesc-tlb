

#include <vector>
#include <algorithm>

#include "callback.hpp"

#define RISCV_PAGE_SIZE    4096
#define RISCV_SV48_PTESIZE 8

typedef uint64_t pte_t;
typedef uint64_t id_t;

// SV48 Page Table Entry Format
struct sv48pte_t {
    uint64_t rsw: 10;
    uint64_t ppn: 38;
    uint64_t rsw2: 9;
    uint64_t xd: 1;
    uint64_t w: 1;
    uint64_t r: 1;
    uint64_t x: 1;
    uint64_t u: 1;
    uint64_t g: 1;
    uint64_t a: 1;
    uint64_t d: 1;
    uint64_t soft_dirty: 1;
    uint64_t rsw3: 2;
    uint64_t v: 1;
};

// TLB translate callback, accept 1 parameter 
// (has type of TLBTranslateResponse)
typedef TLBTranslateCallback CallbackFunction1<TLBTranslateResponse>;

enum PageFaultType {
    NOT_AVAILABLE,
    NO_EXCEPTION,
    ACCESS_FAULT,
    INVALID_PTE,
    NO_PERMISSION,
    PAGE_MISALIGN,
    PAGE_ACCESS,
    PAGE_WRITE
}

class TLBTranslateRequest{
public:
    uint64_t req_id;
    Addr_t vaddr;
    bool permission_r;
    bool permission_w;
    bool permission_x;
    bool permission_u;
    bool is_store;
};


class TLBTranslateResponse {
public:
    uint64_t req_id;
    Addr_t paddr;
    PageFaultType pf_type;
};


class TLB {
    class PTWEntry {
        bool valid;
        bool done;
        bool l1tlb_read_ongoing[4];
        bool l1tlb_read_complete[4];
        bool mem_read_ongoing[4];
        bool mem_read_complete[4];
        bool pte_found[4];
        pte_t ptes[4];

        // Indicating which VPN section is translating
        // Start from 3 until 0
        // Refer to RISC-V Privileged Architectures, SV48
        uint32_t translating_vpn_num;

        Addr_t vaddr;
        Addr_t paddr;

        // Intermediate variable
        Addr_t a;

        bool permission_r;
        bool permission_w;
        bool permission_x;
        bool permission_u;

        bool is_store;

        uint64_t req_id;


        PageFaultType pf_type;
        CallbackFunction1 *complete_cb;


        PTWEntry(){
            valid = false;
            done = false;
            pf_type = NOT_AVAILABLE;
        };
    };
private:
    pte_t *root_page_table;
    // This base address is in simulated address space
    Addr_t page_table_base_addr;

    MemObj *L1D;
    Cache  *L1TLB;
    // cache  *L2TLB;

    int nPTW;
    std::vector<PTWEntry> *ptw_entries;

    // Send request to tlb or memory
    void read_L1TLB(uint32_t ptw_id);
    void read_L1D(uint32_t ptw_id);

    // worker
    void TLB::do_translate(uint32_t ptw_id);

public:
    TLB(uint32_t nPTW, pte_t *root_page_table) {
        // TODO: build memory system
        L1TLB = new CCache(this );
        
        ptw_entries = new std::vector<PTWEntry>(nPTW);
    }
    bool has_free_PTW();

    // Accept an request and a callback function that takes TLBTranslateResponse as parameter
    bool translate(TLBTranslateRequest* req, CallbackFunction1 *cb);
    void flush();
}