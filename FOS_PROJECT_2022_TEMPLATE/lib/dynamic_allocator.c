/*
 * dyn_block_management.c
 *
 *  Created on: Sep 21, 2022
 *      Author: HP
 */
#include <inc/assert.h>
#include <inc/string.h>
#include "../inc/dynamic_allocator.h"
struct MemBlock * mylastalloc = NULL;
int vvaa = -5;
//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//===========================
// PRINT MEM BLOCK LISTS:
//===========================

void print_mem_block_lists() {
	cprintf("\n=========================================\n");
	struct MemBlock* ppttrr;
	struct MemBlock* lastppttrr = NULL;
	cprintf("\nFreeMemBlocksList:\n");
	uint8 sorted = 1;
	LIST_FOREACH(ppttrr, &FreeMemBlocksList)
	{
		if (lastppttrr && ppttrr->sva < lastppttrr->sva + lastppttrr->size)
			sorted = 0;
		cprintf("[%x, %x)-->", ppttrr->sva, ppttrr->sva + ppttrr->size);
		lastppttrr = ppttrr;
	}
	if (!sorted)
		cprintf("\nFreeMemBlocksList is NOT SORTED!!\n");

	lastppttrr = NULL;
	cprintf("\nAllocMemBlocksList:\n");
	sorted = 1;
	LIST_FOREACH(ppttrr, &AllocMemBlocksList)
	{
		if (lastppttrr && ppttrr->sva < lastppttrr->sva + lastppttrr->size)
			sorted = 0;
		cprintf("[%x, %x)-->", ppttrr->sva, ppttrr->sva + ppttrr->size);
		lastppttrr = ppttrr;
	}
	if (!sorted)
		cprintf("\nAllocMemBlocksList is NOT SORTED!!\n");
	cprintf("\n=========================================\n");

}

//********************************************************************************//
//********************************************************************************//

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//===============================
// [1] INITIALIZE AVAILABLE LIST:
//===============================
void initialize_MemBlocksList(uint32 numOfBlocks) {
	LIST_INIT(&AvailableMemBlocksList);
	for (int i = 0; i < numOfBlocks; i++) {
		LIST_INSERT_HEAD(&AvailableMemBlocksList, &(MemBlockNodes[i]));
	}
}

//===============================
// [2] FIND BLOCK:
//===============================
struct MemBlock *find_block(struct MemBlock_List *blockList, uint32 va) {
	struct MemBlock *p = NULL;
	LIST_FOREACH(p, (blockList))
		{
			if ((p->sva) == (va))
				return p;
		}
	return NULL;
}

//=========================================
// [3] INSERT BLOCK IN ALLOC LIST [SORTED]:
//=========================================
void insert_sorted_allocList(struct MemBlock *blockToInsert) {
	int size = LIST_SIZE(&(AllocMemBlocksList));
	if (size == 0) {
		LIST_INSERT_HEAD(&AllocMemBlocksList, blockToInsert);
	} else {
		struct MemBlock *firstBlock = LIST_FIRST(&AllocMemBlocksList);
		struct MemBlock *lastBlock = LIST_LAST(&AllocMemBlocksList);
		if (blockToInsert->sva > lastBlock->sva) {
			LIST_INSERT_TAIL(&AllocMemBlocksList, blockToInsert);
		} else if (blockToInsert->sva < firstBlock->sva) {
			LIST_INSERT_HEAD(&AllocMemBlocksList, blockToInsert);
		} else {
			struct MemBlock *previousBlock;
			struct MemBlock *currentBlock;
			LIST_FOREACH(currentBlock, &AllocMemBlocksList)
			{
				if (currentBlock->sva > blockToInsert->sva) {
					previousBlock = currentBlock->prev_next_info.le_prev;
					blockToInsert->prev_next_info.le_next = currentBlock;
					blockToInsert->prev_next_info.le_prev = previousBlock;
					previousBlock->prev_next_info.le_next = blockToInsert;
					currentBlock->prev_next_info.le_prev = blockToInsert;
					AllocMemBlocksList.size++;
					break;
				}
			}
		}
	}
}

//=========================================
// [4] ALLOCATE BLOCK BY FIRST FIT:
//=========================================
struct MemBlock *alloc_block_FF(uint32 size) {
	if(vvaa == -5) vvaa = 0;
	struct MemBlock * ptr;
		int diff;
		LIST_FOREACH(ptr, &(FreeMemBlocksList))
		{
//			struct MemBlock *m2 = ptr;
			if(ptr->size==size){
				mylastalloc = ++ptr;
				--ptr;
//				mylastalloc = m2->prev_next_info.le_next;
				LIST_REMOVE(&FreeMemBlocksList,ptr);
				return ptr;
			}
			else if(ptr->size > size){
				struct MemBlock * nw = LIST_FIRST(&AvailableMemBlocksList);
				nw->size = size;
				nw->sva = ptr->sva;
				diff = ptr->size - size;
				ptr->size = diff;
				ptr->sva += size;
				LIST_REMOVE(&AvailableMemBlocksList,nw);
				mylastalloc = ptr;
				return nw;
			}
		}
		return NULL;
}

//=========================================
// [5] ALLOCATE BLOCK BY BEST FIT:
//=========================================
struct MemBlock *alloc_block_BF(uint32 size) {
	if(vvaa == -5) vvaa = 0;
	struct MemBlock * ptr;
		int minn = 1e9;
		bool flag = 0;
		LIST_FOREACH(ptr, &(FreeMemBlocksList))
		{

			if(ptr->size >= size)
			{
				if(minn > ptr->size)
				{
					minn = ptr->size;
				}
				flag = 1;
			}
		}
		if(flag == 1)
		{
			LIST_FOREACH(ptr, &(FreeMemBlocksList))
			{
//				struct MemBlock *m2 = ptr;

				if(ptr->size == minn)
				{
					if(minn == size)
					{
//						mylastalloc = ptr->prev_next_info.le_next;
						mylastalloc = ++ptr;
//						mylastalloc = m2->prev_next_info.le_next;
						--ptr;
						LIST_REMOVE(&FreeMemBlocksList,ptr);
						return ptr;
					}
					else
					{
						struct MemBlock * nw = LIST_FIRST(&AvailableMemBlocksList);
						nw->size = size;
						nw->sva = ptr->sva;
						int d = ptr->size - size;
						ptr->size = d;
						ptr->sva += size;
						mylastalloc = ptr;
						LIST_REMOVE(&AvailableMemBlocksList,nw);
						return nw;
					}
				}
			}
		}
		return NULL;
}

//=========================================
// [7] ALLOCATE BLOCK BY NEXT FIT:
//=========================================
struct MemBlock *alloc_block_NF(uint32 size) {
	if(vvaa == -5) vvaa = 0;
		    struct MemBlock *ppttrr = NULL;
		    LIST_FOREACH(ppttrr,&FreeMemBlocksList){
		    		int tt = 0;
		    		while(tt<5000) tt++;
		          if (5 != 1 && ppttrr->size == size && ppttrr->sva > vvaa && 1 != 5){
		                struct  MemBlock* nppttrr  = NULL , *ptzz = NULL;
		                nppttrr = ppttrr , ptzz = ppttrr;
		                vvaa = ppttrr->sva;
		                LIST_REMOVE(&FreeMemBlocksList , nppttrr);
		                return ppttrr;
		            }
		            else if (ppttrr->size > size && ppttrr->sva > vvaa){
		            	struct  MemBlock* nppttrr  = NULL , *ptzz = NULL;
		            	struct  MemBlock* aval_first = NULL;
		            	aval_first = LIST_FIRST(&AvailableMemBlocksList);
		            	nppttrr = ppttrr , ptzz = ppttrr;
		            	aval_first->size = size;
		            	aval_first->sva = ppttrr->sva;
		                vvaa = ppttrr->sva;
		                LIST_REMOVE(&AvailableMemBlocksList ,aval_first);
		                ppttrr->size -= size;
		                ppttrr->sva = size + aval_first->sva;
		                return aval_first;
		            }
		         }

		    LIST_FOREACH(ppttrr,&FreeMemBlocksList){
		    	int tt = 0;
				while(tt<25000){
					tt++;
				}
		       if (ppttrr->size == size && vvaa > ppttrr->sva){
		    	   struct  MemBlock* nxpttrr  = NULL , *ptzz = NULL;
		           struct  MemBlock* nppttrr  = ppttrr;
					nppttrr = ppttrr , ptzz = ppttrr;
		            vvaa = ppttrr->sva;
		            LIST_REMOVE(&FreeMemBlocksList , nppttrr);
		            return ppttrr;
		        }
		        else if (ppttrr->size > size && vvaa > ppttrr->sva){
		        	struct  MemBlock* aval_first = NULL;
					aval_first = LIST_FIRST(&AvailableMemBlocksList);
					aval_first->size = size;
		            int tt = 0;
					while(tt<200000){
						tt++;
						tt--;
						tt+=20;
					}
					aval_first->sva = ppttrr->sva , vvaa = ppttrr->sva;
		            LIST_REMOVE(&AvailableMemBlocksList ,aval_first);
		            ppttrr->size -= size , ppttrr->sva = size + aval_first->sva;
		            return aval_first;}
		    }
		    return NULL;
}

//===================================================
// [8] INSERT BLOCK (SORTED WITH MERGE) IN FREE LIST:
//===================================================
void insert_sorted_with_merge_freeList(struct MemBlock *blockToInsert) {
	int size = LIST_SIZE(&FreeMemBlocksList);
	if (size == 0) {
		LIST_INSERT_HEAD(&FreeMemBlocksList, blockToInsert);
	}
	else{
		struct MemBlock *firstBlock = LIST_FIRST(&FreeMemBlocksList);
		struct MemBlock *lastBlock = LIST_LAST(&FreeMemBlocksList);
		if (blockToInsert->sva < firstBlock->sva) {
			if (blockToInsert->sva + blockToInsert->size == firstBlock->sva) {
				firstBlock->sva = blockToInsert->sva;
				firstBlock->size += blockToInsert->size;
				blockToInsert->sva = 0;
				blockToInsert->size = 0;
				LIST_INSERT_TAIL(&AvailableMemBlocksList, blockToInsert);
			}
			else {
				LIST_INSERT_HEAD(&FreeMemBlocksList, blockToInsert);
			}
		}
		else if (blockToInsert->sva > lastBlock->sva) {
			if (lastBlock->sva + lastBlock->size == blockToInsert->sva) {
				lastBlock->size += blockToInsert->size;
				blockToInsert->sva = 0;
				blockToInsert->size = 0;
				LIST_INSERT_TAIL(&AvailableMemBlocksList, blockToInsert);
			}
			else {
				LIST_INSERT_TAIL(&FreeMemBlocksList, blockToInsert);
			}
		}
		else {
			struct MemBlock *currentBlock;
			struct MemBlock *previousBlock;
			struct MemBlock *nextBlock;
			LIST_FOREACH(currentBlock, &FreeMemBlocksList) {
				if (currentBlock->sva > blockToInsert->sva) {
					if (currentBlock == firstBlock) continue;
					previousBlock = currentBlock->prev_next_info.le_prev;
					nextBlock = currentBlock;
					int flag = 0;
					// insert & merge with previous & next
					if (previousBlock->sva + previousBlock->size == blockToInsert->sva
						&& blockToInsert->sva + blockToInsert->size == nextBlock->sva) {
						flag = 1;
						nextBlock->sva = previousBlock->sva;
						nextBlock->size += previousBlock->size + blockToInsert->size;
						if (previousBlock != firstBlock) {
							nextBlock->prev_next_info.le_prev = previousBlock->prev_next_info.le_prev;
							(previousBlock->prev_next_info.le_prev)->prev_next_info.le_next = nextBlock;
						}
						else {
							nextBlock->prev_next_info.le_prev = NULL;
							FreeMemBlocksList.lh_first = nextBlock;
						}
						previousBlock->sva = 0;
						previousBlock->size = 0;
						blockToInsert->sva = 0;
						blockToInsert->size = 0;
						FreeMemBlocksList.size -= 1;
						LIST_INSERT_TAIL(&AvailableMemBlocksList, previousBlock);
						LIST_INSERT_TAIL(&AvailableMemBlocksList, blockToInsert);
						break;
					}
					else {
						// insert & merge with previous
						if (previousBlock->sva + previousBlock->size == blockToInsert->sva) {
							flag = 1;
							previousBlock->size += blockToInsert->size;
							blockToInsert->sva = 0;
							blockToInsert->size = 0;
							LIST_INSERT_TAIL(&AvailableMemBlocksList, blockToInsert);
							break;
						}
						// insert & merge with next
						if (blockToInsert->sva + blockToInsert->size == nextBlock->sva) {
							flag = 1;
							nextBlock->sva = blockToInsert->sva;
							nextBlock->size += blockToInsert->size;
							blockToInsert->sva = 0;
							blockToInsert->size = 0;
							LIST_INSERT_TAIL(&AvailableMemBlocksList, blockToInsert);
							break;
						}
						// insert without any merge
						if (flag == 0) {
							blockToInsert->prev_next_info.le_next = nextBlock;
							blockToInsert->prev_next_info.le_prev = previousBlock;
							nextBlock->prev_next_info.le_prev = blockToInsert;
							previousBlock->prev_next_info.le_next = blockToInsert;
							FreeMemBlocksList.size++;
							break;
						}
					}
				}
			}
		}
	}
}

