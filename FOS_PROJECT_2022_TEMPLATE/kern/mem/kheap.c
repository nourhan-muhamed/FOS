#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"

//==================================================================//
//==================================================================//
//NOTE: All kernel heap allocations are multiples of PAGE_SIZE (4KB)//
//==================================================================//
//==================================================================//

void initialize_dyn_block_system()
{
	//TODO: [PROJECT MS2] [KERNEL HEAP] initialize_dyn_block_system
	// your code is here, remove the panic and write your code
//	kpanic_into_prompt("initialize_dyn_block_system() is not implemented yet...!!");

	//[1] Initialize two lists (AllocMemBlocksList & FreeMemBlocksList) [Hint: use LIST_INIT()]
	LIST_INIT(&AllocMemBlocksList);
	LIST_INIT(&FreeMemBlocksList);
	#if STATIC_MEMBLOCK_ALLOC
		//	DO NOTHING
	#else
		/*[2] Dynamically allocate the array of MemBlockNodes
		 * 	remember to:
		 * 		1. set MAX_MEM_BLOCK_CNT with the chosen size of the array
		 * 		2. allocation should be aligned on PAGE boundary
		 * 	HINT: can use alloc_chunk(...) function
		 */
	MAX_MEM_BLOCK_CNT = NUM_OF_KHEAP_PAGES;
	MemBlockNodes = (struct MemBlock *) KERNEL_HEAP_START;
	uint32 total_size = ROUNDUP((sizeof(struct MemBlock) * MAX_MEM_BLOCK_CNT),PAGE_SIZE);
	allocate_chunk(ptr_page_directory, KERNEL_HEAP_START,total_size, PERM_WRITEABLE);
	#endif
	//[3] Initialize AvailableMemBlocksList by filling it with the MemBlockNodes
	initialize_MemBlocksList(MAX_MEM_BLOCK_CNT);
	struct MemBlock * av = LIST_FIRST(&AvailableMemBlocksList);
	LIST_REMOVE(&AvailableMemBlocksList, av);
	av->size = ((KERNEL_HEAP_MAX - KERNEL_HEAP_START) - total_size);
	av->sva = (KERNEL_HEAP_START + total_size); // NOT STARTING FROM THE KERNEL HEAP: START AFTER MEM BLOCK NODES
	//[4] Insert a new MemBlock with the remaining heap size into the FreeMemBlocksList
	LIST_INSERT_HEAD(&FreeMemBlocksList, av);
}

void* kmalloc(unsigned int size) {
	unsigned int aligned_size = ROUNDUP(size, PAGE_SIZE);
	struct MemBlock * ret = NULL;
	if (isKHeapPlacementStrategyFIRSTFIT()) {
		ret = alloc_block_FF(aligned_size);
	} else if (isKHeapPlacementStrategyBESTFIT()) {
		ret = alloc_block_BF(aligned_size);
	} else if (isKHeapPlacementStrategyNEXTFIT()) {
		ret = alloc_block_NF(aligned_size);
	}
	if (ret != NULL) {
		int allocated = allocate_chunk(ptr_page_directory, ret->sva,
				aligned_size, PERM_WRITEABLE);
		if (allocated == 0) {
			insert_sorted_allocList(ret);
			return (void *) ret->sva;
		}
	}
	return NULL;
}

void kfree(void* virtual_address)
{
	//TODO: [PROJECT MS2] [KERNEL HEAP] kfree
	// Write your code here, remove the panic and write your code
//	panic("kfree() is not implemented yet...!!");
	struct MemBlock *block = NULL;
	block = find_block(&AllocMemBlocksList,
			(uint32) virtual_address);
	if (block != NULL) {
		LIST_REMOVE(&AllocMemBlocksList, block);
		uint32 end = block->sva + block->size;
		for (uint32 add = block->sva; add < end; add++) {
			unmap_frame(ptr_page_directory, add);
		}
		insert_sorted_with_merge_freeList(block);
	}

}

unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//TODO: [PROJECT MS2] [KERNEL HEAP] kheap_virtual_address
	// Write your code here, remove the panic and write your code
//	panic("kheap_virtual_address() is not implemented yet...!!");

	//return the virtual address corresponding to given physical_address
	//refer to the project presentation and documentation for details
	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================
	struct FrameInfo *p_frame_info ;
	p_frame_info = to_frame_info(physical_address);
	return p_frame_info->va;
}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	//TODO: [PROJECT MS2] [KERNEL HEAP] kheap_physical_address
	// Write your code here, remove the panic and write your code
//	panic("kheap_physical_address() is not implemented yet...!!");
	//return the physical address corresponding to given virtual_address
	//refer to the project presentation and documentation for details
	return virtual_to_physical(ptr_page_directory,virtual_address);
}


void kfreeall()
{
	panic("Not implemented!");

}

void kshrink(uint32 newSize)
{
	panic("Not implemented!");
}

void kexpand(uint32 newSize)
{
	panic("Not implemented!");
}




//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// krealloc():

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to kmalloc().
//	A call with new_size = zero is equivalent to kfree().

void *krealloc(void *virtual_address, uint32 new_size)
{
	//TODO: [PROJECT MS2 - BONUS] [KERNEL HEAP] krealloc
	// Write your code here, remove the panic and write your code
	panic("krealloc() is not implemented yet...!!");
}
