#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdalign.h>
#include <assert.h>
#include "config.h"
#include "mlpt.h"

size_t ptbr = 0 ;
/**
 * Given a virtual address, return the physical address.
 * Return a value consisting of all 1 bits
 * if this virtual address does not have a physical address.
 */
size_t translate(size_t va){
    size_t page_offset = va & ((1 << POBITS) - 1);
    size_t vpn = va >> POBITS;
    int vpn_bits_per_level = POBITS - 3;

    //pointer to base page table entry
    size_t * page_table = (size_t *) ptbr; 
    size_t pte;//page table entry
    
    //check for valid bit
    if (ptbr == 0) {
        return ~ 0;
    }

    for (int i = 0 ; i < LEVELS; ++i) {
        int shift_amount = (LEVELS-i-1) * vpn_bits_per_level;
        size_t vpn_for_level = vpn >> shift_amount;
        size_t index = vpn_for_level & ((1 << vpn_bits_per_level) - 1);
        pte = page_table[index];

        //returning all 1s pte is invalid
        if((pte & 1) == 0){
            return ~0;
        }

        if (i< (LEVELS-1)){
            page_table = (size_t *) (pte & ~((1 << POBITS) - 1));
        } else {
            //on the last level of the page table
            size_t ppn = pte >> POBITS;
            size_t pa = (ppn <<POBITS) + page_offset;
            return pa;
        }
    }
    return ~0;
}

/**
 * Use posix_memalign to create page tables and other pages sufficient
 * to have a mapping between the given virtual address and some physical address.
 * If there already is such a page, does nothing.
 **/
void page_allocate(size_t va) {
    int page_size = 1 << POBITS;
    int vpn_bits_per_level = POBITS - 3; 
    size_t vpn = va >> POBITS;  
    size_t *pg_table;
    void *new_page;

    // check valid bit if not then allocate
    if (ptbr == 0) {
        posix_memalign((void **)&ptbr, page_size, page_size);
        // intialziing page table to 0s
        for (int i = 0; i < page_size/sizeof(size_t); ++i) {
            ((size_t *)ptbr)[i] = 0;
        }
    }
    pg_table = (size_t *)ptbr;
    for (int i = 0; i < LEVELS; ++i) {
        int shift_amount = (LEVELS - i - 1) * vpn_bits_per_level;
        size_t vpn_for_level = vpn >> shift_amount;
        size_t mask = (1 << vpn_bits_per_level) - 1;
        size_t index = vpn_for_level & mask;
        size_t pte = pg_table[index];

        //allocating new page table if invalid
        if ((pte & 1) == 0) {
            posix_memalign(&new_page, page_size, page_size);
            // Initialize the new page table to 0
            for (int j = 0; j < page_size/sizeof(size_t); j++) {
                ((size_t *)new_page)[j] = 0;
            }
            pg_table[index] = ((size_t)new_page >> POBITS) << POBITS | 1; // Set the valid bit
        }
        // Move to the next level page table
        pg_table = (size_t *)(pg_table[index] & ~((1 << POBITS) - 1));
    }
}



int main() {

    // 0 pages have been allocated
    assert(ptbr == 0);
    page_allocate(0x456789abcdef);
    // 5 pages have been allocated: 4 page tables and 1 data
    assert(ptbr != 0);

    page_allocate(0x456789abcd00);
    // no new pages allocated (still 5) (because both the addresses map to the same page)
    
    //tranlsate adress to physical adress
    int *p1 = (int *)translate(0x456789abcd00);
    //assigns a value at the physical adress in memory
    *p1 = 0xaabbccdd;
    
    //this should also be in the same allocated page
    short *p2 = (short *)translate(0x456789abcd02);
    printf("%04hx\n", *p2); // prints "aabb\n"

    //checks translate for an unallocated Virtual adress
    assert(translate(0x123456780000) == 0xFFFFFFFFFFFFFFFF);
    
    //now allocating that adress
    page_allocate(0x123456780000);
    // 1 new page allocated (now 6; 4 page table, 2 data)

    //checking to see that the allocation worked
    assert(translate(0x123456780000) != 0xFFFFFFFFFFFFFFFF);
    
    page_allocate(0x123456780000);
    assert(translate(0x123456780000)!= 0xFFFFFFFFFFFFFFFF);
    // 2 new pages allocated (now 8; 5 page table, 3 data)
}
