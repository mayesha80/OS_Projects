#include <stdio.h>
#include <stdlib.h>
#include "vm.h"
#include "API.h"
#include "list.h"
#include <stdbool.h>

#define MAX_FRAMES 256
#define HOT_RATIO 0.5

// Data structures for FIFO
int fifo_position = 0;

// Data structures for LRU
struct Node *lruHead = NULL;

// Data structures for CLOCK
int clockHand = 0;
int *clockReferenceBits = NULL;

// Data Structures for CLOCK-Pro
int clockProCold[MAX_FRAMES] = {0}; // Array to store cold pages
int clockProEvicted[MAX_FRAMES] = {0}; // Non-resident pages in test period
int referenceBits[MAX_FRAMES] = {0};  // Reference bits for each page
int hot_pages_count = 0;

int HAND_HOT = 0;
int HAND_COLD = 0;
int HAND_EVICTION = 0; // Keeps track of the number of frames

// Forward declarations
void promote_to_hot(int PFN);
void demote_from_hot(int PFN);

int fifo() {
    int frame_to_replace = fifo_position;
    fifo_position = (fifo_position + 1) % MAX_PFN;
    return frame_to_replace;
}

int lru()
{
    if (lruHead == NULL) {
        return -1; // No frames to replace
    }
    struct Node *tailNode = lruHead;
    while (tailNode->next != NULL) {
        tailNode = tailNode->next;
    }
    int PFN = tailNode->data;
    lruHead = list_remove_tail(lruHead);
    return PFN;
}

void add_to_lru(int PFN)
{
    lruHead = list_insert_head(lruHead, PFN);
}

void update_lru(int PFN)
{
    lruHead = list_remove(lruHead, PFN);
    lruHead = list_insert_head(lruHead, PFN);
}

int clock()
{
    while (true) {
        if (clockReferenceBits[clockHand] == 0) {
            int PFN = clockHand;
            clockReferenceBits[clockHand] = 1;
            clockHand = (clockHand + 1) % MAX_PFN;
            return PFN;
        } else {
            clockReferenceBits[clockHand] = 0;
            clockHand = (clockHand + 1) % MAX_PFN;
        }
      
    }
}
void add_to_clock(int PFN)
{
    if (PFN >= 0 && PFN < MAX_PFN) {
        clockReferenceBits[PFN] = 1;
    }
}
//handle cold hand operations
bool handle_cold_hand() {
    if (clockProCold[HAND_COLD] == 1) {
        if (referenceBits[HAND_COLD] == 0 || hot_pages_count >= (MAX_FRAMES * HOT_RATIO)) {
            int evictedPage = HAND_COLD;
            clockProCold[HAND_COLD] = 0;
            clockProEvicted[HAND_COLD] = 1;
            HAND_COLD = (HAND_COLD + 1) % MAX_PFN;
            return true;
        }
        promote_to_hot(HAND_COLD);
    }
    HAND_COLD = (HAND_COLD + 1) % MAX_PFN;
    return false;
}

//handle hot hand operations
void handle_hot_hand() {
    if (clockProCold[HAND_HOT] == 0) {
        return;
    }
    
    if (referenceBits[HAND_HOT] == 1) {
        referenceBits[HAND_HOT] = 0;
        printf("Clearing reference bit for Hot page: %d\n", HAND_HOT);
    } else {
        if (hot_pages_count > (MAX_FRAMES * HOT_RATIO)) {
            demote_from_hot(HAND_HOT);
            printf("Demoting Hot page to Cold: %d\n", HAND_HOT);
        }
    }
    HAND_HOT = (HAND_HOT + 1) % MAX_PFN;
}

// handle eviction hand operations
void handle_eviction_hand() {
    if (clockProEvicted[HAND_EVICTION] == 1) {
        // Check if evicted page has been accessed again
        if (referenceBits[HAND_EVICTION] == 1) {
            // Re-promote evicted page to cold
            clockProEvicted[HAND_EVICTION] = 0;
            clockProCold[HAND_EVICTION] = 1;
            referenceBits[HAND_EVICTION] = 0; // Clear reference bit
            printf("Re-promoting evicted page to Cold: %d\n", HAND_EVICTION);
        } else {
            // Remove the evicted page permanently if not accessed again
            clockProEvicted[HAND_EVICTION] = 0;
            printf("Removing evicted page permanently: %d\n", HAND_EVICTION);
        }
    }
    HAND_EVICTION = (HAND_EVICTION + 1) % MAX_PFN;
}

// Function to promote a page to hot
void promote_to_hot(int PFN) {
    // Remove from cold and promote to hot
    clockProCold[PFN] = 0;
    referenceBits[PFN] = 1;
    hot_pages_count++;
}

// Function to demote a page from hot
void demote_from_hot(int PFN) {
    // Remove from hot and add to cold
    clockProCold[PFN] = 1;
    hot_pages_count--;
}

int clock_pro() {
    int i;
    // First pass: look for non-referenced cold pages
    for ( i = 0; i < MAX_PFN; i++) {
        int current = (HAND_COLD + i) % MAX_PFN;
        if (clockProCold[current] == 1 && referenceBits[current] == 0) {
            clockProCold[current] = 0;
            clockProEvicted[current] = 1;
            HAND_COLD = (current + 1) % MAX_PFN;
            return current;
        }
    }
    // Second pass: clear reference bits and look again
    for (i = 0; i < MAX_PFN; i++) {
        int current = (HAND_COLD + i) % MAX_PFN;
        if (clockProCold[current] == 1) {
            referenceBits[current] = 0;
            if (hot_pages_count < (MAX_FRAMES * HOT_RATIO)) {
                clockProCold[current] = 0;
                clockProEvicted[current] = 1;
                HAND_COLD = (current + 1) % MAX_PFN;
                return current;
            }
        }
    }

    return HAND_COLD;
}

/*========================================================================*/

int find_replacement()
{
    int PFN;
    if (replacementPolicy == ZERO) {
        PFN = 0;  // Always replace frame 0 for ZERO policy
    } else if (replacementPolicy == FIFO) {
        PFN = fifo();
    } else if (replacementPolicy == LRU) {
        PFN = lru();
    } else if (replacementPolicy == CLOCK) {
        PFN = clock();
    } else if (replacementPolicy == CLOCK_PRO) {
        PFN = clock_pro();
    }

    return PFN;
}

int pagefault_handler(int pid, int VPN, char reqType)
{
    int PFN;

    // Find a free PFN.
    PFN = get_freeframe();
    //printf("Free PFN %d\n", PFN);
    // No free frame available; find a victim using a page replacement algorithm.
    if (PFN < 0) {
        PFN = find_replacement();  // This should return 0 if the policy is ZERO
       // printf("Replacing PFN %d\n", PFN);
        IPTE ipte = read_IPTE(PFN);

        // Only invalidate if the frame was in use
        if (ipte.pid != -1) {
            PTE victim_pte = read_PTE(ipte.pid, ipte.VPN);
            victim_pte.valid = false;  // Invalidate the victim page
            write_PTE(ipte.pid, ipte.VPN, victim_pte);
            
            // If the victim page is dirty, write it to the swap disk
            if (victim_pte.dirty) {
                swap_out(ipte.pid, ipte.VPN, PFN);
            }
        }
    }
    
    // Bring the page in from the swap disk.
    swap_in(pid, VPN, PFN);

    // Update the page table.
    PTE new_pte;
    new_pte.PFN = PFN;
    new_pte.valid = true;
    new_pte.dirty = (reqType == 'W') ? true : false;
    write_PTE(pid, VPN, new_pte);
    
    // Update the inverted page table.
    IPTE new_ipte;
    new_ipte.pid = pid;
    new_ipte.VPN = VPN;
    write_IPTE(PFN, new_ipte);
    
    // Add the frame to the FIFO queue for replacement management, only for FIFO policy.
    if (replacementPolicy == FIFO) {
       
    }

    // Add the frame to the LRU list for replacement management, only for LRU policy.
    if (replacementPolicy == LRU) {
        add_to_lru(PFN);
    }

    // Initialize the reference bit for CLOCK policy.
    if (replacementPolicy == CLOCK) {
        add_to_clock(PFN);
    }
    
    if (replacementPolicy == CLOCK_PRO) {
        clockProCold[PFN] = 1;  // New pages start as cold
        referenceBits[PFN] = (reqType == 'W') ? 1 : 0;  // Set reference bit for writes
    }
    
    return PFN;
}

int get_PFN(int pid, int VPN, char reqType)
{
    // Read page table entry for (pid, VPN)
    PTE pte = read_PTE(pid, VPN);

    // If PTE is valid, it is a page hit. Return physical frame number (PFN)
    if (pte.valid) {
        // Mark the page dirty, if it is a write request
        if (reqType == 'W') {
            pte.dirty = true;
            write_PTE(pid, VPN, pte);
        }
        // Update the LRU list for LRU replacement management
        if (replacementPolicy == LRU) {
            update_lru(pte.PFN);
        }
        // Update the reference bit for CLOCK replacement management
        if (replacementPolicy == CLOCK) {
            add_to_clock(pte.PFN);
        }
        if (replacementPolicy == CLOCK_PRO) {
            referenceBits[pte.PFN] = 1; // Set reference bit on access
        }

        return pte.PFN;
    }
    
    // PageFault, if the PTE is invalid. Return -1
    return -1;
}

int MMU(int pid, int virtAddr, char reqType, bool *hit)
{
    if(replacementPolicy == CLOCK && clockReferenceBits == NULL) {
        clockReferenceBits = (int*) malloc(sizeof(int) * MAX_PFN);
        int i;
        for(i = 0; i < MAX_PFN; ++i) {
            clockReferenceBits[i] = 0;
        }
    }
    if (replacementPolicy == CLOCK_PRO) {
    hot_pages_count = 0;
    int i;
    for (i = 0; i < MAX_PFN; i++) {
        clockProCold[i] = 0;
        clockProEvicted[i] = 0;
        referenceBits[i] = 0;
    }
}

    int PFN, physicalAddr;
    int VPN = (virtAddr >> 8);
    int offset = virtAddr & 0xFF;
    
    // Read page table to get Physical Frame Number (PFN)
    PFN = get_PFN(pid, VPN, reqType);
   // printf("PFN %d %d\n", VPN, PFN);
    if (PFN >= 0) { // Page hit
        stats.hitCount++;
        *hit = true;
    } else { // Page miss
        stats.missCount++;
        *hit = false;
        PFN = pagefault_handler(pid, VPN, reqType);
    }

    physicalAddr = (PFN << 8) + offset;  // Construct the physical address
    return physicalAddr;
}

