


#include "tlb.hpp"

bool TLB::has_free_PTW() {
    for (auto& ptw: &ptw_entries) {
        if (!ptw.valid) {
            // found free slot
            return true;
        }
    }
    return false;
}


void TLB::flush() {
    // TODO: Flush L1TLB
}


// Issue L!TLB read request. 
void TLB::read_L1TLB(uint32_t ptw_id) {
    // Issue read request. If complete, 
    auto translating_vpn = ptw_entries[ptw_id].translating_vpn_num;

    // Mark read status as ongoing
    ptw_entries[ptw_id].l1tlb_read_ongoing[translating_vpn] = true;


    // Issue read request. 
    
    // Mark status as complete when callback function called


    return;
}

void TLB::read_L1D(uint32_t ptw_id) {
    // Similar with read_L1TLB, but issue memory read to L1D
    return;
}




void TLB::do_translate(uint32_t ptw_id) {


    if (!ptw_entries[ptw_id].done) {
        uint32_t vpn_offset = {39, 30, 21, 12};

        auto translating_vpn = ptw_entries[ptw_id].translating_vpn_num;


        Addr_t vpn_n = (ptw_entries[ptw_id].vaddr >> (vpn_offset[translating_vpn])) & 0x1FF;
        Addr_t page_table_offset = ptw_entries[ptw_id].a + RISCV_SV48_PTESIZE * vpn_n;

        if (ptw_entries[ptw_id].pte_found[translating_vpn]) {
            // pte is available
            sv48pte_t entry;
            assert(sizeof(entry) == RISCV_SV48_PTESIZE);
            std::memcpy(&entry, &ptw_entries[ptw_id].ptes[translating_vpn], sizeof(entry));

            // Error condition: RISC-V Privileged Architectures, Section 4.3.2, item 3
            if ((entry.v == 0) || (entry.r == 0 && entry.w == 1) || (entry.rsw != 0)) {
                ptw_entries[ptw_id].done = true;
                ptw_entries[ptw_id].pf_type = INVALID_PTE;
            }

            if (entry.r == 1 || entry.w == 1) {
                // A leaf PTE, see Section 4.3.2, item 5
                // Check permission
                // TODO: Check U bits
                // Note: Wether raise exception on U bits is dependent on current privilege mode, value of 
                //       SUM and MXR fields of mstatus register.
                if ((entry.r >= static_cast<uint64_t>(ptw_entries[ptw_id].permission_r))
                    && (entry.w >= static_cast<uint64_t>(ptw_entries[ptw_id].permission_w))
                    && (entry.x >= static_cast<uint64_t>(ptw_entries[ptw_id].permission_x))) {
                    
                    // Has Access
                    // Check page alignment
                    uint32_t mask = (1 << (translating_vpn_num)) - 1;
                    if (translating_vpn > 0 && ((entry.ppn & mask) != 0)) {
                        // A misaligned superpage. Section 4.3.2 item 6
                        ptw_entries[ptw_id].done = true;
                        ptw_entries[ptw_id].pf_type = PAGE_MISALIGN;

                        goto end;
                    }

                    // Check Access bit. This behavior depends on config registers. See Section 4.3.2, item 7
                    if ((entry.a == 0)) {
                        ptw_entries[ptw_id].done = true;
                        ptw_entries[ptw_id].pf_type = PAGE_ACCESS;

                        goto end;
                    }

                    if ((entry.d == 0 && ptw_entries[ptw_id].is_store)) {
                        ptw_entries[ptw_id].done = true;
                        ptw_entries[ptw_id].pf_type = PAGE_WRITE;

                        goto end;
                    }


                    // Done
                    Addr_t offset = ptw_entries[ptw_id].vaddr & 0xFFF;

                    Addr_t ppn = 0;
                    if (translating_vpn != 0) {
                        // Superpage
                        // ppn = vpn in lower bits
                        Addr_t vpn = ptw_entries[ptw_id].vaddr >> 12;
                        Addr_t ppn_mask = (1 << (translating_vpn_num * 9)) - 1;
                        ppn = vpn & ppn_mask;
                    }
                    uint32_t ppn_position = translating_vpn_num * 9;
                    Addr_t ppn_upper_mask = (static_cast<uint64_t>(-1L) << ppn_position);
                    Addr_t ppn_upper = (entry.ppn & ppn_upper_mask) << ppn_position;
                    ppn = ppn | ppn_upper;

                    Addr_t paddr = (ppn < 12) | offset;

                    ptw_entries[ptw_id].done = true;
                    ptw_entries[ptw_id].pf_type = NO_EXCEPTION;
                    ptw_entries[ptw_id].paddr = paddr;
                } else {
                    // Permission Error
                    ptw_entries[ptw_id].done = true;
                    ptw_entries[ptw_id].pf_type = NO_PERMISSION;

                    goto end;
                }
            } else {
                // Pointer to next level
                // Go to next level
                if (ptw_entries[ptw_id].translating_vpn_num == 0) {
                    // Error: The last level page table is NOT a leaf PTE
                    ptw_entries[ptw_id].done = true;
                    ptw_entries[ptw_id].pf_type = INVALID_PTE;

                    goto end;
                }
                // Section 4.3.2, item 4
                ptw_entries[ptw_id].translating_vpn_num -= 1;
                ptw_entries[ptw_id].a = entry.ppn * RISCV_PAGE_SIZE;
            }
        } else {
            // pte not available
            // Search L1TLB first
            if ((!ptw_entries[ptw_id].l1tlb_read_ongoing[translating_vpn]) && 
                (!ptw_entries[ptw_id].l1tlb_read_complete[translating_vpn])) {
                // L1TLB not searched
                read_L1TLB(ptw_id);
            } else if ((!ptw_entries[ptw_id].mem_read_ongoing[translating_vpn]) && 
                        (!ptw_entries[ptw_id].mem_read_complete[translating_vpn])) {
                read_L1D(ptw_id);
            } else {
                if ((ptw_entries[ptw_id].mem_read_complete[translating_vpn]) && 
                    (ptw_entries[ptw_id].l1tlb_read_complete[translating_vpn])) {
                    // Both TLB and memory searched, but no entry
                    ptw_entries[ptw_id].done = true;
                    ptw_entries[ptw_id].pf_type = ACCESS_FAULT;
                }

            }
        }
    } 

end:
    if (ptw_entries[ptw_id].done) {
        // Done
        // TODO: Pass a pointer or simply pass by stack?
        TLBTranslateResponse resp;

        resp.req_id = ptw_entries[ptw_id].req_id;
        resp.paddr = ptw_entries[ptw_id].paddr;
        resp.pf_type = ptw_entries[ptw_id].pf_type;

        if (ptw_entries[ptw_id].cb) ptw_entries[ptw_id].cb -> call(resp);
    }


}


bool TLB::translate(TLBTranslateRequest* req, CallbackFunction1 *cb) {
    // Allocate ptw

    // Fail if no available PTW
    if (!has_free_PTW()) {
        return false;
    }

    uint32_t ptw_id = 0;
    for (auto& ptw: &ptw_entries) {
        if (!ptw.valid) {
            // found free slot
            ptw.valid = true;
            ptw.done = false;

            // Init
            for (uint32_t i = 0; i < 4; i++) {
                ptw.l1tlb_read_ongoing[i] = false;
                ptw.mem_read_ongoing[i] = false;
                ptw.l1tlb_read_complete[i] = false;
                ptw.mem_read_complete[i] = false;
                ptw.pte_found[i] = false;
            }
            ptw.translating_vpn_num = 3;
            ptw.result = NOT_AVAILABLE;
            ptw.complete_cb = cb;
            ptw.vaddr = req -> vaddr;
            ptw.a = 0;

            ptw.permission_r = req -> permission_r;
            ptw.permission_w = req -> permission_w;
            ptw.permission_x = req -> permission_x;
            ptw.permission_u = req -> permission_u;

            ptw.is_store = req -> is_store;

            ptw.req_id = req -> req_id;

            do_translate(ptw_id);

            // Task dispatched
            return true;
        }
        ptw_id++;
    }

    // Shouldn't go here
    assert(false);
    return false;
}


TLB::find_PTE()

// Function to translate a virtual address to a physical address
uint64_t virt_to_phys(uint64_t vaddr)
{
    // SV48
    uint64_t vpn[4];
    pte_t *p;

    // Split the virtual address into 4 9-bit VPNs
    vpn[0] = (vaddr >> 39) & 0x1ff;
    vpn[1] = (vaddr >> 30) & 0x1ff;
    vpn[2] = (vaddr >> 21) & 0x1ff;
    vpn[3] = (vaddr >> 12) & 0x1ff;

    // Traverse the page table hierarchy to find the physical page
    p = root_page_table + vpn[0];
    p = (pte_t *)(*p & ~0xfffUL);
    p = p + vpn[1];
    p = (pte_t *)(*p & ~0xfffUL);
    p = p + vpn[2];
    p = (pte_t *)(*p & ~0xfffUL);
    p = p + vpn[3];

    // Calculate the physical address
    return ((*p & ~0xfffUL) | (vaddr & 0xfff));
}