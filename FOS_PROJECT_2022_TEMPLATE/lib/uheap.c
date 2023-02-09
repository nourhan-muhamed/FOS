#include <inc/lib.h>

//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

int FirstTimeFlag = 1;
void InitializeUHeap()
{
	if(FirstTimeFlag)
	{
		initialize_dyn_block_system();
		cprintf("DYNAMIC BLOCK SYSTEM IS INITIALIZED\n");
#if UHP_USE_BUDDY
		initialize_buddy();
		cprintf("BUDDY SYSTEM IS INITIALIZED\n");
#endif
		FirstTimeFlag = 0;
	}
}

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//=================================
// [1] INITIALIZE DYNAMIC ALLOCATOR:
//=================================
void initialize_dyn_block_system() {
	//[1] Initialize two lists (AllocMemBlocksList & FreeMemBlocksList) [Hint: use LIST_INIT()]
	LIST_INIT(&AllocMemBlocksList);
	LIST_INIT(&FreeMemBlocksList);
	//[2] Dynamically allocate the array of MemBlockNodes at VA USER_DYN_BLKS_ARRAY
	//	  (remember to set MAX_MEM_BLOCK_CNT with the chosen size of the array)
	MAX_MEM_BLOCK_CNT = NUM_OF_UHEAP_PAGES;
	MemBlockNodes = (struct MemBlock *) USER_DYN_BLKS_ARRAY;
	uint32 total_size = ROUNDUP((sizeof(struct MemBlock) * MAX_MEM_BLOCK_CNT),
			PAGE_SIZE);
	sys_allocate_chunk(USER_DYN_BLKS_ARRAY, total_size,
	PERM_WRITEABLE | PERM_USER);
	//[3] Initialize AvailableMemBlocksList by filling it with the MemBlockNodes
	initialize_MemBlocksList(MAX_MEM_BLOCK_CNT);
	//[4] Insert a new MemBlock with the heap size into the FreeMemBlocksList
	struct MemBlock * av = LIST_FIRST(&AvailableMemBlocksList);
	LIST_REMOVE(&AvailableMemBlocksList, av);
	av->size = USER_HEAP_MAX - USER_HEAP_START;
	av->sva = USER_HEAP_START;
	LIST_INSERT_HEAD(&FreeMemBlocksList, av);
}

//=================================
// [2] ALLOCATE SPACE IN USER HEAP:
//=================================

void* malloc(uint32 size) {
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	if (size == 0)
		return NULL;
	//==============================================================
	//==============================================================

	// Steps:
	//	1) Implement FF strategy to search the heap for suitable space
	//		to the required allocation size (space should be on 4 KB BOUNDARY)
	//	2) if no suitable space found, return NULL
	// 	3) Return pointer containing the virtual address of allocated space,
	//
	//Use sys_isUHeapPlacementStrategyFIRSTFIT()... to check the current strategy
	int newSize = ROUNDUP(size, PAGE_SIZE);
	struct MemBlock * pt = NULL;
	if (sys_isUHeapPlacementStrategyFIRSTFIT()) {
		pt = alloc_block_FF(newSize);
		if (pt != NULL) {
			insert_sorted_allocList(pt);
		}
	}

	if (pt == NULL)
		return NULL;
	return (void *) pt->sva;
}

//=================================
// [3] FREE SPACE FROM USER HEAP:
//=================================
// free():
//	This function frees the allocation of the given virtual_address
//	To do this, we need to switch to the kernel, free the pages AND "EMPTY" PAGE TABLES
//	FROM main memory AND free pages from page file then switch back to the user again.
//
//	We can use sys_free_user_mem(uint32 virtual_address, uint32 size); which
//		switches to the kernel mode, calls free_user_mem() in
//		"kern/mem/chunk_operations.c", then switch back to the user mode here
//	the free_user_mem function is empty, make sure to implement it.
void free(void* virtual_address) {
	//TODO: [PROJECT MS3] [USER HEAP - USER SIDE] free
	// your code is here, remove the panic and write your code
//	panic("free() is not implemented yet...!!");

	//you should get the size of the given allocation using its address
	//you need to call sys_free_user_mem()
	//refer to the project presentation and documentation for details
	struct MemBlock *block = NULL;
	block = find_block(&AllocMemBlocksList, (uint32) virtual_address);
	if (block != NULL) {
		uint32 size = block->size;
		LIST_REMOVE(&AllocMemBlocksList, block);
		sys_free_user_mem((uint32)virtual_address, size);
		insert_sorted_with_merge_freeList(block);
	}
}


//=================================
// [4] ALLOCATE SHARED VARIABLE:
void* smalloc(char *sharedVarName, uint32 size, uint8 isWritable){
	InitializeUHeap();
	if (size == 0) return NULL;
	unsigned int my4kbSize = ROUNDUP(size, 4096);
	struct MemBlock * myMEMptr = NULL;
	if(sys_isUHeapPlacementStrategyFIRSTFIT())
		myMEMptr = alloc_block_FF(my4kbSize);
	if(myMEMptr == NULL) return NULL;
	void * voidSVA = (void *)myMEMptr->sva;
	int createSharedReturnedVal = sys_createSharedObject(sharedVarName,size,isWritable,voidSVA);
	if(createSharedReturnedVal == E_NO_SHARE || createSharedReturnedVal == E_SHARED_MEM_EXISTS)
		return NULL;
	return (void *)myMEMptr->sva;
}
// [5] SHARE ON ALLOCATED SHARED VARIABLE:
void* sget(int32 ownerEnvID, char *sharedVarName){
	InitializeUHeap();
	int sharedObjSize = sys_getSizeOfSharedObject(ownerEnvID,sharedVarName);
	if(sharedObjSize == E_SHARED_MEM_NOT_EXISTS)
		return NULL;
	uint32 my4kbSize = ROUNDUP(sharedObjSize , PAGE_SIZE);
	struct MemBlock * myMEMptr = NULL;
	if(sys_isUHeapPlacementStrategyFIRSTFIT())
		myMEMptr = alloc_block_FF(my4kbSize);
	if(myMEMptr == NULL) return NULL;
	void * voidSVA = (void *)myMEMptr->sva;
	int getSharedReturnedVal = sys_getSharedObject(ownerEnvID , sharedVarName,voidSVA);
	if(getSharedReturnedVal == E_SHARED_MEM_NOT_EXISTS)
		return NULL;
	return voidSVA;
}


//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//


//=================================
// REALLOC USER SPACE:
//=================================
//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to malloc().
//	A call with new_size = zero is equivalent to free().

//  Hint: you may need to use the sys_move_user_mem(...)
//		which switches to the kernel mode, calls move_user_mem(...)
//		in "kern/mem/chunk_operations.c", then switch back to the user mode here
//	the move_user_mem() function is empty, make sure to implement it.
void *realloc(void *virtual_address, uint32 new_size)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	//==============================================================
	// [USER HEAP - USER SIDE] realloc
	// Write your code here, remove the panic and write your code
	panic("realloc() is not implemented yet...!!");
}


//=================================
// FREE SHARED VARIABLE:
//=================================
//	This function frees the shared variable at the given virtual_address
//	To do this, we need to switch to the kernel, free the pages AND "EMPTY" PAGE TABLES
//	from main memory then switch back to the user again.
//
//	use sys_freeSharedObject(...); which switches to the kernel mode,
//	calls freeSharedObject(...) in "shared_memory_manager.c", then switch back to the user mode here
//	the freeSharedObject() function is empty, make sure to implement it.

void sfree(void* virtual_address)
{
	//TODO: [PROJECT MS3 - BONUS] [SHARING - USER SIDE] sfree()

	// Write your code here, remove the panic and write your code
	panic("sfree() is not implemented yet...!!");
}




//==================================================================================//
//========================== MODIFICATION FUNCTIONS ================================//
//==================================================================================//
void expand(uint32 newSize)
{
	panic("Not Implemented");

}
void shrink(uint32 newSize)
{
	panic("Not Implemented");

}
void freeHeap(void* virtual_address)
{
	panic("Not Implemented");
}
