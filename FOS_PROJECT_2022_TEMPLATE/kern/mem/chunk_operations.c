/*
 * chunk_operations.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include <kern/trap/fault_handler.h>
#include <kern/disk/pagefile_manager.h>
#include "kheap.h"
#include "memory_manager.h"

/******************************/
/*[1] RAM CHUNKS MANIPULATION */
/******************************/

//===============================
// 1) CUT-PASTE PAGES IN RAM:
//===============================
//This function should cut-paste the given number of pages from source_va to dest_va
//if the page table at any destination page in the range is not exist, it should create it
//Hint: use ROUNDDOWN/ROUNDUP macros to align the addresses
int cut_paste_pages(uint32* page_directory, uint32 source_va, uint32 dest_va,
		uint32 num_of_pages) {
	uint32 start_source = ROUNDDOWN(source_va, PAGE_SIZE);
	uint32 start_destination = ROUNDDOWN(dest_va, PAGE_SIZE);
	uint32 sz = num_of_pages * PAGE_SIZE;
	//int x;
	bool out = 0;
	uint32 *p_page_table = NULL;
	for (uint32 i = start_destination; i < start_destination + sz; i +=
	PAGE_SIZE)
	{
		//x = get_page_table(page_directory, i, &p_page_table);
		if (get_frame_info(page_directory, i, &p_page_table) != NULL) {
			out = 1;
			break;
		}
		//		else if(x == TABLE_NOT_EXIST)
		//		{
		//			create_page_table(page_directory , i);
		//		}
	}
	if (out) {
		return -1;
	} else {
		uint32 *ptr_page_table = NULL;
		struct FrameInfo *Frame_Info_source;
		for (uint32 i = start_source, j = start_destination;
				i < sz + start_source; i += PAGE_SIZE, j += PAGE_SIZE) {
			Frame_Info_source = get_frame_info(page_directory, i,
					&ptr_page_table);
			map_frame(page_directory, Frame_Info_source, j,
					pt_get_page_permissions(page_directory, i));
			unmap_frame(page_directory, i);
		}
		return 0;
	}
}

//===============================
// 2) COPY-PASTE RANGE IN RAM:
//===============================
//This function should copy-paste the given size from source_va to dest_va
//if the page table at any destination page in the range is not exist, it should create it
//Hint: use ROUNDDOWN/ROUNDUP macros to align the addresses
int copy_paste_chunk(uint32* page_directory, uint32 source_va, uint32 dest_va,
		uint32 size) {
	uint32 sourceEnd = source_va + size;
	uint32 destEnd = dest_va + size;
	uint32 currentSource = source_va;
	for (uint32 address = dest_va; address < destEnd; address += 4) {
		uint32 *ptr_page_table = NULL;
		struct FrameInfo* dest_frame_info = get_frame_info(page_directory,
				address, &ptr_page_table);
		if (dest_frame_info != 0) {
			int perms = pt_get_page_permissions(page_directory, address);
			int res = perms & PERM_WRITEABLE;
			if (res != PERM_WRITEABLE) {
				// read-only
				return -1;
			}
		}
	}
	for (uint32 address = dest_va; address < destEnd; address += 4) {
		uint32 *ptr_page_table = NULL;
		struct FrameInfo* dest_frame_info = get_frame_info(page_directory,
				address, &ptr_page_table);
		if (dest_frame_info == 0) {
			if (ptr_page_table == NULL) {
				create_page_table(page_directory, address);
			}
			int ret = allocate_frame(&dest_frame_info);
			if (ret == 0) {
				int copied_perm = pt_get_page_permissions(page_directory,
						currentSource);
				map_frame(page_directory, dest_frame_info, address,
						copied_perm);
				pt_set_page_permissions(page_directory, address, PERM_WRITEABLE,
						0);
			}
		} else {
			// page exists
			int copied_perm = pt_get_page_permissions(page_directory,
					currentSource);
			map_frame(page_directory, dest_frame_info, address, copied_perm);
		}
		unsigned char *src = (unsigned char *) currentSource;
		unsigned char *dest = (unsigned char*) address;
		for (int i = 0; i < 4; i++) {
			*dest = *src;
			src++;
			dest++;
		}
		currentSource += 4;
	}
	return 0;
}

//===============================
// 3) SHARE RANGE IN RAM:
//===============================
//This function should share the given size from dest_va with the source_va
//Hint: use ROUNDDOWN/ROUNDUP macros to align the addresses
int share_chunk(uint32* page_directory, uint32 source_va, uint32 dest_va,
		uint32 size, uint32 perms) {
	//TODO: [PROJECT MS2] [CHUNK OPERATIONS] share_chunk
	// Write your code here, remove the panic and write your code
	uint32 alignedSourceStart = ROUNDDOWN(source_va, 4096);
	uint32 alignedSourceEnd = ROUNDUP(source_va + size, 4096);
	uint32 alignedDestStart = ROUNDDOWN(dest_va, 4096);
	uint32 alignedDestEnd = ROUNDUP(dest_va + size, 4096);

	for (uint32 address = alignedDestStart; address < alignedDestEnd; address +=
			4096) {
		uint32 *ptr_page_table = NULL;
		get_page_table(page_directory, address, &ptr_page_table);
		if (ptr_page_table == NULL) {
			create_page_table(page_directory, address);
		}
		struct FrameInfo *ret = get_frame_info(page_directory, address,
				&ptr_page_table);
		if (ret != 0) {
			return -1;
		}
	}

	uint32 currentSource = alignedSourceStart;
	for (uint32 address = alignedDestStart; address < alignedDestEnd; address +=
			4096) {
		uint32 *ptr_page_table = NULL;
		get_page_table(page_directory, address, &ptr_page_table);
		struct FrameInfo *ret = get_frame_info(page_directory, address,
				&ptr_page_table);
		if (ret == 0) {
			struct FrameInfo *ptr_old_frame = NULL;
			uint32 *ptr_old_page_table = NULL;
			get_page_table(page_directory, currentSource, &ptr_old_page_table);
			ptr_old_frame = get_frame_info(page_directory, currentSource,
					&ptr_old_page_table);
			map_frame(page_directory, ptr_old_frame, address, perms);
		}
		currentSource += 4096;
	}
	return 0;

//	panic("share_chunk() is not implemented yet...!!");
}

//===============================
// 4) ALLOCATE CHUNK IN RAM:
//===============================
int allocate_chunk(uint32* page_directory, uint32 va, uint32 size, uint32 perms) {
	int  returnedVal = 0;
	uint32 sva = ROUNDDOWN(va, 4),sva1=0;
	uint32 lastva = ROUNDUP(va + size, 4096);
	sva1 = sva;

	while(sva1 < lastva){
		uint32 * page_table_ptrr;
		page_table_ptrr = NULL;
		int pg_bool = get_page_table(page_directory, sva1, &page_table_ptrr);
		if(page_table_ptrr == NULL)
			create_page_table(page_directory, sva1);
		struct FrameInfo * framee;
		framee = NULL;
		framee = get_frame_info(page_directory, sva1,&page_table_ptrr);
		if (framee != 0)
			return -1;
//		framee->va = sva1;
		sva1 += 4096;
	}

	int allocfff = 5;
	while(sva < lastva){
		uint32 *pt;
		pt = NULL;
		int pg_tbl = get_page_table(page_directory, sva, &pt);
		if (pt == NULL)
			create_page_table(page_directory, sva);
		struct FrameInfo * myallocframe = get_frame_info(page_directory,sva, &pt);
		allocfff = allocate_frame(&myallocframe);
		int mapping = map_frame(page_directory, myallocframe, sva, perms);
		myallocframe->va = sva;
		sva+=4096;
	}
	return returnedVal;
}

/*BONUS*/
//=====================================
// 5) CALCULATE ALLOCATED SPACE IN RAM:
//=====================================
void calculate_allocated_space(uint32* page_directory, uint32 sva, uint32 eva,
		uint32 *num_tables, uint32 *num_pages) {
	const int MAX_TABLE_CNT = ((eva - sva) / PAGE_SIZE);
	uint32 *CALCULATED_TABLES[MAX_TABLE_CNT];
	uint32 lastIndex = 0;
	uint32 alignedStart = ROUNDDOWN(sva, PAGE_SIZE);
	uint32 alignedEnd = ROUNDUP(eva, PAGE_SIZE);
	for (uint32 address = alignedStart; address < alignedEnd; address +=
	PAGE_SIZE) {
		uint32 *ptr_page_table = NULL;
		struct FrameInfo *ret = get_frame_info(page_directory, address,
				&ptr_page_table);
		if (ret != 0) {
			(*num_pages)++;
		}
		if (ptr_page_table != NULL) {
			uint32 isFound = 0;
			for (int i = 0; i < lastIndex; i++) {
				if (CALCULATED_TABLES[i] == ptr_page_table) {
					isFound = 1;
					break;
				}
			}
			if (isFound == 0) {
				CALCULATED_TABLES[lastIndex] = ptr_page_table;
				lastIndex++;
				(*num_tables)++;
			}
		}
	}
}

/*BONUS*/
//=====================================
// 6) CALCULATE REQUIRED FRAMES IN RAM:
//=====================================
// calculate_required_frames:
// calculates the new allocation size required for given address+size,
// we are not interested in knowing if pages or tables actually exist in memory or the page file,
// we are interested in knowing whether they are allocated or not.
uint32 calculate_required_frames(uint32* page_directory, uint32 sva,
		uint32 size) {
	uint32 frames_cnt = 0, start = ROUNDDOWN(sva, PAGE_SIZE), end = ROUNDUP(
			sva + size, PAGE_SIZE), table_start = ROUNDDOWN(sva,
			(PAGE_SIZE * NPTENTRIES)), table_end = ROUNDUP(sva + size,
			(PAGE_SIZE * NPTENTRIES));
	for (uint32 address = start; address < end; address += PAGE_SIZE) {
		uint32 *ptr_page_table = NULL;
		struct FrameInfo *frame = get_frame_info(page_directory, address,
				&ptr_page_table);
		if (frame == 0)
			frames_cnt++;
	}
	for (uint32 address = table_start; address < table_end;
			address += (PAGE_SIZE * NPTENTRIES)) {
		uint32 *ptr_page_table = NULL;
		get_page_table(page_directory, address, &ptr_page_table);
		if (ptr_page_table == NULL)
			frames_cnt++;
	}
	return frames_cnt;
}

//=================================================================================//
//===========================END RAM CHUNKS MANIPULATION ==========================//
//=================================================================================//

/*******************************/
/*[2] USER CHUNKS MANIPULATION */
/*******************************/

//======================================================
/// functions used for USER HEAP (malloc, free, ...)
//======================================================
//=====================================
// 1) ALLOCATE USER MEMORY:
//=====================================
void allocate_user_mem(struct Env* e, uint32 virtual_address, uint32 size) {
// Write your code here, remove the panic and write your code
	panic("allocate_user_mem() is not implemented yet...!!");
}

//=====================================
// 2) FREE USER MEMORY:
//=====================================
void free_user_mem(struct Env* e, uint32 virtual_address, uint32 size) {
//This function should:
//1. Free ALL pages of the given range from the Page File
//2. Free ONLY pages that are resident in the working set from the memory
//3. Removes ONLY the empty page tables (i.e. not used) (no pages are mapped in the table)
	uint32 start_va = ROUNDDOWN(virtual_address, PAGE_SIZE);
	uint32 end_va = ROUNDUP(virtual_address + size, PAGE_SIZE);
	for (uint32 curr_va = start_va; curr_va < end_va; curr_va += PAGE_SIZE) {
		// remove from page file
		pf_remove_env_page(e, curr_va);
		// remove from working set(if exists)
		for (int i = 0; i < e->page_WS_max_size; i++) {
			if (env_page_ws_get_virtual_address(e, i) == curr_va) {
				env_page_ws_clear_entry(e, i);
				unmap_frame(e->env_page_directory, curr_va);
				break;
			}
		}
		unmap_frame(e->env_page_directory, curr_va);
		// check page_table of each page
		uint32 *ptr_page_table = NULL;
		get_page_table(e->env_page_directory, curr_va, &ptr_page_table);
		int doClear = 1;
		if (ptr_page_table != NULL) {
			for (int i = 0; i < 1024; i++) {
				if (ptr_page_table[i] != 0) {
					doClear = 0;
					break;
				}
			}
		}
		if (doClear == 1) {
			kfree(ptr_page_table);
			pd_clear_page_dir_entry(e->env_page_directory, (uint32) curr_va);
		}
	}
//	env_page_ws_print(e);
}


//=====================================
// 2) FREE USER MEMORY (BUFFERING):
//=====================================
void __free_user_mem_with_buffering(struct Env* e, uint32 virtual_address,
		uint32 size) {
// your code is here, remove the panic and write your code
	panic("__free_user_mem_with_buffering() is not implemented yet...!!");

//This function should:
//1. Free ALL pages of the given range from the Page File
//2. Free ONLY pages that are resident in the working set from the memory
//3. Free any BUFFERED pages in the given range
//4. Removes ONLY the empty page tables (i.e. not used) (no pages are mapped in the table)
}

//=====================================
// 3) MOVE USER MEMORY:
//=====================================
void move_user_mem(struct Env* e, uint32 src_virtual_address,
		uint32 dst_virtual_address, uint32 size) {
//TODO: [PROJECT MS3 - BONUS] [USER HEAP - KERNEL SIDE] move_user_mem
//your code is here, remove the panic and write your code
	panic("move_user_mem() is not implemented yet...!!");

// This function should move all pages from "src_virtual_address" to "dst_virtual_address"
// with the given size
// After finished, the src_virtual_address must no longer be accessed/exist in either page file
// or main memory

	/**/
}

//=================================================================================//
//========================== END USER CHUNKS MANIPULATION =========================//
//=================================================================================//

