#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

void flush(const uint8_t* addr){
	asm __volatile__("mfence\n\tclflush 0(%0)" : : "r"(addr) :);
}

void flushMem(const uint8_t* base_addr, int size_in_bytes){
	for(int i =0; i<size_in_bytes; i++)
		flush(base_addr+i);
}

uint32_t measure_one_block_access_time(uint64_t addr)
{
	uint32_t cycles;
	asm volatile("mov %1, %%r8\n\t"
			"lfence\n\t"
			"rdtsc\n\t"
			"mov %%eax, %%edi\n\t"
			"mov (%%r8), %%r8\n\t"
			"lfence\n\t"
			"rdtsc\n\t"
			"sub %%edi, %%eax\n\t"
			: "=a"(cycles)
			: "r"(addr)
			: "r8", "edi");
}

int main(){
	/* victim stuff */
	uint8_t byte = 0x41;
	int mode = 0;
	int map_length = 4096*256; // ascii_byte * page_size

	// init common addr stuff
	uint8_t* secret_addr;
	if((secret_addr = (uint8_t*)mmap((void*) 0x7f761e213000, map_length, PROT_READ | PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS|MAP_FIXED, -1, 0)) == -1)
	{
		printf("mmap error\n");
		exit(1);
	} // why does it not like PROT_WRITE
	printf("Original address %lx\n", secret_addr);
	secret_addr = (uint8_t*) (((uintptr_t)secret_addr & 0xFFFFFFFFFFFFF000)+0x1000);
	printf("secret byte and addr: %d, %lx\n", byte, secret_addr);

	// decide victim vs attacker
	char str1[20];
	printf("Victim or attacker? ");
	scanf("%s", str1);

	if(strcmp(str1, "attacker") == 0) mode = 1;
	else if(strcmp(str1, "victim") == 0) mode = 0;

	printf("You are the %s\n", str1);
	/* attacker stuff */
	char oracle[256*4096];
	uint8_t extract = 0x41;
	uint32_t best_cycles = 1000;

	while(1)
	{
		if(!mode){
			//printf("faulty here, mode %d\n", mode);
			asm volatile("movq %0, %%r10\n\t"
					"movb %1, (%%r10)\n\t"
					:
					: "r" (secret_addr), "r" (byte)
					: "%r10"
				    );
			//printf("Mode is %d\n", mode);
		}
		else{
			//printf("mode %d\n", mode);

			asm volatile("movq %1, %%r10\n\t"
				"movb (%%r10), %0\n\t"
				: "=r"(extract)
				: "r"(secret_addr)
				: "%r10"
			    );
			//extract = extract << 12;
			//flush buffer
			for(uint32_t i =0; i<256; i++)
				flush(&oracle[i*4096]);
			oracle[extract*4096];
			for(uint32_t i =0; i<256; i++)
			{
				uint32_t temp_cycles;
				temp_cycles = measure_one_block_access_time((uint64_t)&oracle[i*4096]);
				if(temp_cycles < best_cycles){
					best_cycles = temp_cycles;
					printf("Potential byte %c found at addr %lx\n", i, secret_addr);
				}
				
			}
		}
	}
	return 0;
}
