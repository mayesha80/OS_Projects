#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "vm.h"
#include "API.h"

int replacementPolicy;
int MAX_PFN;
int MAX_VPN;
int MAX_PID;
char *PHYSICAL_MEM;
STATS stats;


int READ_PHYSICAL_MEM(int addr, char *byte)
{
		int frameNo = (addr >> 8);
		if(frameNo < 0 || frameNo >= MAX_PFN) return -1;

		*byte = PHYSICAL_MEM[frameNo];
		return 1;
}

int WRITE_PHYSICAL_MEM(int addr, char byte)
{
		int frameNo = (addr >> 8);
		if(frameNo < 0 || frameNo >= MAX_PFN) return -1;

		PHYSICAL_MEM[frameNo] = byte;

		return 1;
}

void pt_print_stats()
{
		int req = stats.hitCount + stats.missCount;
		int hit = stats.hitCount;
		int miss = stats.missCount;

		printf("Request: %d\n", req);
		printf("Page Hit: %d (%.2f%%)\n", hit, (float) hit*100 / (float)req);
		printf("Page Miss: %d (%.2f%%)\n", miss, (float)miss*100 / (float)req);
		printf("Swap Read: %d\n", stats.swapRead);
		printf("Swap Write: %d\n", stats.swapWrite);

}

int main(int argc, char **argv)
{
		char input[256];
		int pid, virtAddr, ret;
		char reqType, byte;
		bool hit;
		int PFN, offset, physicalAddr;
		FILE *in;

		int i;

		if(argc < 4) {
				fprintf(stderr, "usage: ./vm [# of FRAMES] [REPLACEMENT_POLICY] [INPUT]\n");
				fprintf(stderr, "# of FRAMES: 1 ~ 256\n");
				fprintf(stderr," REPLACEMENT_POLICY: 0 - ZERO, 1 - FIFO, 2 - LRU, 3 - CLOCK, 4 - CLOCK_PRO\n");
				return -1;
		}

		MAX_PFN = atoi(argv[1]);
		replacementPolicy = atoi(argv[2]);

		in = fopen(argv[3], "r");
		if(in == NULL) {
				fprintf(stderr, "File open error: %s\n", argv[3]);
				return 0;
		}

		if(fgets(input, 256, in) == NULL) return 0;
		sscanf(input, "# PAGES: %d, # PROCS: %d", &MAX_VPN, &MAX_PID);

		printf("# PAGES: %d, # FRAMES: %d, # PROCS: %d, Replacement Policy: %d - ", MAX_VPN, MAX_PFN, MAX_PID, replacementPolicy);
		if(replacementPolicy == ZERO) printf("ZERO\n");
		else if(replacementPolicy == FIFO) printf("FIFO\n");
		else if(replacementPolicy == LRU) printf("LRU\n");
		else if(replacementPolicy == CLOCK) printf("CLOCK\n");
		else if(replacementPolicy == CLOCK_PRO) printf("CLOCK_PRO\n");
		else {
				printf("UNKNOWN!\n");
				return -1;
		}

		if(MAX_VPN < 1 || MAX_VPN > 256 || MAX_PFN < 1 || MAX_PFN > 256 || MAX_PID < 1 || MAX_PID > 256) {
				fprintf(stderr, "ERROR:  # PAGE, # FRAME, and # PROCESS should be between 1 and 256\n");
				return -1;
		}
		
		PHYSICAL_MEM = (char*) malloc(sizeof(char) * MAX_PFN);
		init_pagetable();
		init_swap_disk();
		
		while(fgets(input, 256, in))
		{
				if(input[0] == '#' || input[0] == '%') continue;

				ret = sscanf(input, "%d %c 0x%x %c", &pid, &reqType, &virtAddr, &byte);

				if((reqType == 'R' && ret < 3) || (reqType == 'W' && ret < 4))
				{
						printf("Error: invalid input: %s\n", input);
						continue;
				}

				if(pid > (MAX_PID-1)) {
						printf("[pid %d, %c] 0x%x: Invalid PID.\n", pid, reqType, virtAddr);
						continue;
				}
				
		
				// Convert virtual address to physical address using MMU
				physicalAddr = MMU(pid, virtAddr, reqType, &hit);
				PFN = (physicalAddr >> 8);

				if(PFN >= MAX_PFN || PFN < 0 || physicalAddr < 0) {
						printf("[pid %d, %c] 0x%x --> 0x%x: Invalid Physical Address.\n", pid, reqType, virtAddr, physicalAddr);
						return 0;
				}

				if(reqType == 'R') {
						ret = READ_PHYSICAL_MEM(physicalAddr, &byte);
						if(ret < 0) printf("[pid %d, %c] 0x%04x --> 0x%04x: Memory Read Failed.\n", pid, reqType, virtAddr, physicalAddr);
						else {
								printf("[pid %d, %c] 0x%04x --> 0x%04x: %c", pid, reqType, virtAddr, physicalAddr, byte);
								if(hit) printf(" [hit]\n");
								else printf(" [miss]\n");
						}
				}	else if(reqType == 'W')	{
						ret = WRITE_PHYSICAL_MEM(physicalAddr, byte);
						if(ret < 0) printf("[pid %d, %c] 0x%04x --> 0x%04x: %c Memory Write Failed.\n", pid, reqType, virtAddr, physicalAddr, byte);
						else { 
								printf("[pid %d, %c] 0x%04x --> 0x%04x: %c", pid, reqType, virtAddr, physicalAddr, byte);
								if(hit) printf(" [hit]\n");
								else printf(" [miss]\n");
						}
				} else {
						printf("Error: invalid request reqType: %c\n", reqType);
						return 0;
				}
		}
		
		printf("=====================================\n");
		pt_print_stats();
}

