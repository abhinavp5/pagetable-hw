#include "config.h" /* see pagtable guidance on this file */
#include "mlpt.h"   /* see pagetable this file */
#include <stddef.h>
#include <stdio.h>



#define SETS 16
#define NUM_IDX_BITS 4
#define WAYS 4

//1 individual TLB entry
typedef struct{
    int index; 
    int valid;
    int tag;
    int ppn;
    int LRU; 
} TLBEntry;

//there are 4 page table entries in a Set because it is a 4 way cache
typedef struct{
    int index; 
    TLBEntry entries[WAYS];
} Set; 

//there are 16 sets In this TLB 
typedef struct{
    Set num_sets[SETS];

} TLB;


//making it a global variable
TLB myTLB; 


/** stub for the purpose of testing tlb_* functions */
size_t translate(size_t va) { 
    if (va < 0x1234000)
        return va + 0x20000;
    else if (va > 0x2000000 && va < 0x2345000)
        return va + 0x100000;
    else
        return -1;
 }


/** invalidate all cache lines in the TLB */
void tlb_clear(){
    //iterating through sets
    for (int set = 0; set< SETS; ++set){
        //iterating through Ways 
        for(int way = 0; way<WAYS; ++way){
            myTLB.num_sets[set].index = -1;
            //iterating through TLB entries in the Way
            for(int entry  = 0 ; entry< NUM_IDX_BITS; ++entry){
                //intializing all entries 
                myTLB.num_sets[set].entries[way].index = -1;
                myTLB.num_sets[set].entries[way].valid = 0;
                myTLB.num_sets[set].entries[way].tag = 0;
                myTLB.num_sets[set].entries[way].ppn = 0;   
                myTLB.num_sets[set].entries[way].LRU = 0; 
            }
        }
    } 
}

/**
 * return 0 if this virtual address does not have a valid
 * mapping in the TLB. Otherwise, return its LRU status: 1
 * if it is the most-recently used, 2 if the next-to-most,
 * etc.
 */
int tlb_peek(size_t va){
    int offset_stripped = va >> POBITS;
    int index = offset_stripped & ((1 << NUM_IDX_BITS)- 1);
    int tag = offset_stripped>> NUM_IDX_BITS;

    //iterating through the sets
    for(int set = 0; set< SETS; ++set){

        //iterating through ways
        for (int way = 0; way< WAYS; ++way){

            //checking if index for a set exists
            if (myTLB.num_sets[set].index == index){

                //checking if tag matches
                if(myTLB.num_sets[set].entries[way].tag == tag){
                    
                    //return LRU value
                    return myTLB.num_sets[set].entries[way].LRU;

                }
            }
        }
    }
    //In the case that the va does not exist in the TLB
    return -1 ;
}



/**
 * If this virtual address is in the TLB, return its
 * corresponding physical address. If not, use
 * `translate(va)` to find that address, store the result
 * in the TLB, and return it. In either case, make its
 * cache line the most-recently used in its set.
 *
 * As an exception, if translate(va) returns -1, do not
 * update the TLB: just return -1.
 */
size_t tlb_translate(size_t va){
    int offset_stripped = va >> POBITS;
    int index = offset_stripped & ((1 << NUM_IDX_BITS)- 1);
    int tag = offset_stripped>> NUM_IDX_BITS;
    int ppn_temp = -2;
    int way_temp = -1;

    //iterating through the sets
    for(int set = 0; set< SETS; ++set){
        //checking if idx match
        if (myTLB.num_sets[set].index == index){
        /*SCENARIO 1TLB HIT*/
            //iterating through the entries
            for (int way =0 ; way< WAYS; ++way){
                //TLB Hit 
                if(myTLB.num_sets[set].entries[way].tag == tag ){
                    way_temp = way;
                    ppn_temp  = translate(va);
                    if(ppn_temp== -1){
                        return ppn_temp;
                    }
                    //updating the LRU to 1 because most recently accesed
                    myTLB.num_sets[set].entries[way].LRU =1 ;
                }
            }
            //updating LRUS in other entries in set
            if(ppn_temp != -2){
                for (int way =0 ; way< WAYS; ++way){
                    //increasing all LRUS by one
                    if ((myTLB.num_sets[set].entries[way].LRU >0) & (way!= way_temp)){
                        myTLB.num_sets[set].entries[way].LRU+=1;
                    }

                }
                return ppn_temp; 
            }
        /*SCENARIO 2 TLB MISS*/
            for(int way = 0; way < WAYS; ++way){
                //performing the eviction
                if (myTLB.num_sets[set].entries[way].LRU == 0 || 
                    myTLB.num_sets[set].entries[way].LRU ==4){
                    way_temp = way;
                    ppn_temp = translate(va);
                    if (ppn_temp ==-1){
                        return ppn_temp;
                    }
                    myTLB.num_sets[set].entries[way].valid = 1;
                    myTLB.num_sets[set].entries[way].tag = tag;
                    myTLB.num_sets[set].entries[way].ppn = ppn_temp;   
                    myTLB.num_sets[set].entries[way].LRU = 1; 
                }
                //updating the LRUS
                if(ppn_temp != -2){
                    for (int way =0 ; way< WAYS; ++way){
                    //increasing all LRUS by one
                    if ((myTLB.num_sets[set].entries[way].LRU >0) & (way!= way_temp)){
                        myTLB.num_sets[set].entries[way].LRU+=1;
                    }

                }
                return ppn_temp; 
            }

            }



        }
    }

}

int main(){
    tlb_clear();
    assert(tlb_peek(0) == 0);
    assert(tlb_translate(0) == 0x0020000);
    assert(tlb_peek(0) == 1);
    assert(tlb_translate(0x200) == 0x20200);
    assert(tlb_translate(0x400) == 0x20400);
    assert(tlb_peek(0) == 1);
    assert(tlb_peek(0x200) == 1);
    assert(tlb_translate(0x2001200) == 0x2101200);
    assert(tlb_translate(0x0005200) == 0x0025200);
    assert(tlb_translate(0x0008200) == 0x0028200);
    assert(tlb_translate(0x0002200) == 0x0022200);
    assert(tlb_peek(0x2001000) == 1);
    assert(tlb_peek(0x0001000) == 0);
    assert(tlb_peek(0x0004000) == 0);
    assert(tlb_peek(0x0005000) == 1);
    assert(tlb_peek(0x0008000) == 1);
    assert(tlb_peek(0x0002000) == 1);
    assert(tlb_peek(0x0000000) == 1);
    tlb_clear();
    assert(tlb_peek(0x2001000) == 0);
    assert(tlb_peek(0x0005000) == 0);
    assert(tlb_peek(0x0008000) == 0);
    assert(tlb_peek(0x0002000) == 0);
    assert(tlb_peek(0x0000000) == 0);
    assert(tlb_translate(0) == 0x20000);
    assert(tlb_peek(0) == 1);

}
