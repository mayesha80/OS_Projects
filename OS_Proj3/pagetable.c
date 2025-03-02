#include <stdio.h>
#include <stdlib.h>
#include "vm.h"
#include "API.h"
#include "list.h"

int fifo()
{
		return 0;
}

int lru()
{
		return 0;
}

int clock()
{
		return 0;
}

int clock_pro()
{
		return 0;
}

/*========================================================================*/

int find_replacement()
{
		int PFN;
		if(replacementPolicy == ZERO)  PFN = 0;
		else if(replacementPolicy == FIFO)  PFN = fifo();
		else if(replacementPolicy == LRU) PFN = lru();
		else if(replacementPolicy == CLOCK) PFN = clock();
		else if(replacementPolicy == CLOCK_PRO) PFN = clock_pro();

		return PFN;
}

int pagefault_handler(int pid, int VPN, char reqType)
{
		int PFN;

		// find a free PFN.
		PFN = get_freeframe();
		
		// no free frame available. find a victim using a page replacement algorithm. ;
		if(PFN < 0) {
				PFN = find_replacement();
				/* ---- */
		}
		
		/* ---- */
		return PFN;
}

int get_PFN(int pid, int VPN, char reqType)
{
		/* Read page table entry for (pid, VPN) */
		PTE pte = read_PTE(pid, VPN);

		/* if PTE is valid, it is a page hit. Return physical frame number (PFN) */
		if(pte.valid) {
		/* Mark the page dirty, if it is a write request */
				if(reqType == 'W') {
						pte.dirty = true;
						write_PTE(pid, VPN, pte);
				}
				return pte.PFN;
		}
		
		/* PageFault, if the PTE is invalid. Return -1 */
		return -1;
}

int MMU(int pid, int virtAddr, char reqType, bool *hit)
{
		int PFN, physicalAddr;
		int VPN = 0, offset = 0;
		
		/* calculate VPN and offset
		VPN = ...
		offset = ...
		*/
		
		// read page table to get Physical Frame Number (PFN)
		PFN = get_PFN(pid, VPN, reqType);
		if(PFN >= 0) { // page hit
				stats.hitCount++;
				*hit = true;
		} else { // page miss
				stats.missCount++;
				*hit = false;
				PFN = pagefault_handler(pid, VPN, reqType);
		}

		physicalAddr = (PFN << 8) + offset;
		return physicalAddr;
}

