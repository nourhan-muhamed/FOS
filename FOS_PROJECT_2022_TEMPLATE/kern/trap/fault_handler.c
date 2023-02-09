/*
 * fault_handler.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include "trap.h"
#include <kern/proc/user_environment.h>
#include "../cpu/sched.h"
#include "../disk/pagefile_manager.h"
#include "../mem/memory_manager.h"

#include <kern/trap/fault_handler.h>
#include <kern/disk/pagefile_manager.h>

//2014 Test Free(): Set it to bypass the PAGE FAULT on an instruction with this length and continue executing the next one
// 0 means don't bypass the PAGE FAULT
uint8 bypassInstrLength = 0;

//===============================
// REPLACEMENT STRATEGIES
//===============================
//2020
void setPageReplacmentAlgorithmLRU(int LRU_TYPE)
{
	assert(LRU_TYPE == PG_REP_LRU_TIME_APPROX || LRU_TYPE == PG_REP_LRU_LISTS_APPROX);
	_PageRepAlgoType = LRU_TYPE ;
}
void setPageReplacmentAlgorithmCLOCK(){_PageRepAlgoType = PG_REP_CLOCK;}
void setPageReplacmentAlgorithmFIFO(){_PageRepAlgoType = PG_REP_FIFO;}
void setPageReplacmentAlgorithmModifiedCLOCK(){_PageRepAlgoType = PG_REP_MODIFIEDCLOCK;}
/*2018*/ void setPageReplacmentAlgorithmDynamicLocal(){_PageRepAlgoType = PG_REP_DYNAMIC_LOCAL;}
/*2021*/ void setPageReplacmentAlgorithmNchanceCLOCK(int PageWSMaxSweeps){_PageRepAlgoType = PG_REP_NchanceCLOCK;  page_WS_max_sweeps = PageWSMaxSweeps;}

//2020
uint32 isPageReplacmentAlgorithmLRU(int LRU_TYPE){return _PageRepAlgoType == LRU_TYPE ? 1 : 0;}
uint32 isPageReplacmentAlgorithmCLOCK(){if(_PageRepAlgoType == PG_REP_CLOCK) return 1; return 0;}
uint32 isPageReplacmentAlgorithmFIFO(){if(_PageRepAlgoType == PG_REP_FIFO) return 1; return 0;}
uint32 isPageReplacmentAlgorithmModifiedCLOCK(){if(_PageRepAlgoType == PG_REP_MODIFIEDCLOCK) return 1; return 0;}
/*2018*/ uint32 isPageReplacmentAlgorithmDynamicLocal(){if(_PageRepAlgoType == PG_REP_DYNAMIC_LOCAL) return 1; return 0;}
/*2021*/ uint32 isPageReplacmentAlgorithmNchanceCLOCK(){if(_PageRepAlgoType == PG_REP_NchanceCLOCK) return 1; return 0;}

//===============================
// PAGE BUFFERING
//===============================
void enableModifiedBuffer(uint32 enableIt){_EnableModifiedBuffer = enableIt;}
uint8 isModifiedBufferEnabled(){  return _EnableModifiedBuffer ; }

void enableBuffering(uint32 enableIt){_EnableBuffering = enableIt;}
uint8 isBufferingEnabled(){  return _EnableBuffering ; }

void setModifiedBufferLength(uint32 length) { _ModifiedBufferLength = length;}
uint32 getModifiedBufferLength() { return _ModifiedBufferLength;}

//===============================
// FAULT HANDLERS
//===============================

//Handle the table fault
void table_fault_handler(struct Env * curenv, uint32 fault_va)
{
	//panic("table_fault_handler() is not implemented yet...!!");
	//Check if it's a stack page
	uint32* ptr_table;
#if USE_KHEAP
	{
		ptr_table = create_page_table(curenv->env_page_directory, (uint32)fault_va);
	}
#else
	{
		__static_cpt(curenv->env_page_directory, (uint32)fault_va, &ptr_table);
	}
#endif
}

//Handle the page fault

void page_fault_handler(struct Env * curenv, uint32 fault_va)
{

//	panic("page_fault_handler() is not implemented yet...!!");

	if(env_page_ws_get_size(curenv) < (curenv->page_WS_max_size))		//Placement
	{
		int aligned_sz = ROUNDDOWN(fault_va, PAGE_SIZE);
		struct FrameInfo *ptrrr  = NULL ;
		int ret = allocate_frame(&ptrrr);
		map_frame(curenv->env_page_directory, ptrrr, fault_va, PERM_WRITEABLE | PERM_USER| PERM_PRESENT);
		int rett = pf_read_env_page(curenv, (void *)fault_va);
		bool good = 0;
		if(rett== E_PAGE_NOT_EXIST_IN_PF)
		{
			if(aligned_sz>=USER_HEAP_START && aligned_sz<USER_HEAP_MAX)
			{
				good = 1;
			}
			if(aligned_sz>=USTACKBOTTOM && aligned_sz<USTACKTOP)
			{
				good = 1;
			}
			if(!good)
			{
				panic("ILLEGAL MEMORY ACCESS");
			}
		}
		while(1)
		{
			if(curenv->ptr_pageWorkingSet[curenv->page_last_WS_index].empty)
			{
				env_page_ws_set_entry(curenv, curenv->page_last_WS_index, aligned_sz);
				(curenv->page_last_WS_index)++;
				curenv->page_last_WS_index %= curenv->page_WS_max_size;
				break;
			}
			else
			{
				curenv->page_last_WS_index++;
				curenv->page_last_WS_index %= curenv->page_WS_max_size;
			}
		}
	}
	else	//Re-Placement
	{
		uint32 cur_va;
		int prm;
		bool flag = 0;
		int victim_idx;
		curenv->page_last_WS_index %= curenv->page_WS_max_size;
		for(int i = curenv->page_last_WS_index;i < curenv->page_WS_max_size; i++)
		{
				prm = pt_get_page_permissions(curenv->env_page_directory, curenv->ptr_pageWorkingSet[i].virtual_address);
				cur_va = curenv->ptr_pageWorkingSet[i].virtual_address;
				if(prm & PERM_USED) //used = 1
				{
					//reset used
					pt_set_page_permissions(curenv->env_page_directory, cur_va, 0, PERM_USED);
				}
				else // used = 0
				{
					victim_idx = i;
					flag = 1;
					break;
				}
				if(i == curenv->page_WS_max_size - 1 && !flag)
				{
					i=-1;
				}
		}
		int md_prm = pt_get_page_permissions(curenv->env_page_directory, cur_va);
        uint32 * ptr = NULL;
        int updated;
        struct FrameInfo* md_frame = get_frame_info(curenv->env_page_directory, cur_va, &ptr);
		if(md_prm & PERM_MODIFIED)
		{
			updated = pf_update_env_page(curenv, cur_va, md_frame);
			if(!updated)
			{
				unmap_frame(curenv->env_page_directory, cur_va);
				env_page_ws_clear_entry(curenv, victim_idx);
			}
		}
		else
		{
			unmap_frame(curenv->env_page_directory, cur_va);
			env_page_ws_clear_entry(curenv, victim_idx);
		}
		//placement
		int aligned_sz = ROUNDDOWN(fault_va, PAGE_SIZE);
		int ret = allocate_frame(&md_frame);
		map_frame(curenv->env_page_directory, md_frame, fault_va, PERM_WRITEABLE | PERM_USER | PERM_PRESENT);
		int rett = pf_read_env_page(curenv, (void *)fault_va);
		bool good = 0;
		if(rett== E_PAGE_NOT_EXIST_IN_PF)
		{
			if(aligned_sz>=USER_HEAP_START && aligned_sz<USER_HEAP_MAX)
			{
					good = 1;
			}
			if(aligned_sz>=USTACKBOTTOM && aligned_sz<USTACKTOP)
			{
					good = 1;
			}
			if(!good)
			{
				panic("ILLEGAL MEMORY ACCESS");
			}
		}
		env_page_ws_set_entry(curenv,victim_idx, aligned_sz);
		curenv->page_last_WS_index = (victim_idx + 1) % curenv->page_WS_max_size;
	}
}



void __page_fault_handler_with_buffering(struct Env * curenv, uint32 fault_va)
{
	// Write your code here, remove the panic and write your code
	panic("__page_fault_handler_with_buffering() is not implemented yet...!!");


}
