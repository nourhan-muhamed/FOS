/*
 * paging_helpers.c
 *
 *  Created on: Sep 30, 2022
 *      Author: HP
 */
#include "memory_manager.h"

/*[2.1] PAGE TABLE ENTRIES MANIPULATION */
inline void pt_set_page_permissions(uint32* page_directory, uint32 virtual_address, uint32 permissions_to_set, uint32 permissions_to_clear)
{
	//TODO: [PROJECT MS2] [PAGING HELPERS] pt_set_page_permissions
	// Write your code here, remove the panic and write your code
	//panic("pt_set_page_permissions() is not implemented yet...!!");
	uint32 *ptr_pt = NULL;
	int x = get_page_table(page_directory,virtual_address , &ptr_pt);
	uint32 *table_entry = &ptr_pt [PTX(virtual_address)];
	if (x == TABLE_NOT_EXIST)
	{
		panic("Invalid va");
	}
	else
	{
		*table_entry |= permissions_to_set;
		permissions_to_clear = ~(permissions_to_clear);
		*table_entry &= permissions_to_clear;

	}
	tlb_invalidate((void *)NULL, (void *)virtual_address);
}

inline int pt_get_page_permissions(uint32* page_directory, uint32 virtual_address )
{
	//TODO: [PROJECT MS2] [PAGING HELPERS] pt_get_page_permissions
	// Write your code here, remove the panic and write your code
	//panic("pt_get_page_permissions() is not implemented yet...!!");
	uint32 *ptr_pt = NULL;
	int x = get_page_table(page_directory,virtual_address , &ptr_pt);
	if (x == TABLE_NOT_EXIST)
	{
		return -1;
	}
	else
	{
		uint32 table_entry = ptr_pt [PTX(virtual_address)];
		uint32 f_num = table_entry << 20;
		f_num >>= 20;
		return f_num;
	}
}

inline void pt_clear_page_table_entry(uint32* page_directory, uint32 virtual_address)
{
	//TODO: [PROJECT MS2] [PAGING HELPERS] pt_clear_page_table_entry
	// Write your code here, remove the panic and write your code
//	panic("pt_clear_page_table_entry() is not implemented yet...!!");
	uint32 *ptr_pt=NULL;
		int ret = get_page_table(page_directory,virtual_address, &ptr_pt);
		uint32 *table_entry = &ptr_pt[PTX(virtual_address)];
		if (ret == TABLE_NOT_EXIST){

			 panic("Invalid va") ;
		}

	     else {

	    	 *table_entry = 0;
	         tlb_invalidate((void *)NULL, (void *)virtual_address);

		      }
}

/***********************************************************************************************/

/*[2.2] ADDRESS CONVERTION*/
inline int virtual_to_physical(uint32* page_directory, uint32 virtual_address)
{
	//TODO: [PROJECT MS2] [PAGING HELPERS] virtual_to_physical
	// Write your code here, remove the panic and write your code
//	panic("virtual_to_physical() is not implemented yet...!!");
	uint32 *ptr_pt=NULL;
	int ret = get_page_table(page_directory,virtual_address, &ptr_pt);

	if (ret == TABLE_NOT_EXIST)
	{
		return -1;
	}
	else
	{
		uint32 table_entry=ptr_pt[PTX(virtual_address)];
		uint32 fNum = table_entry >> 12;
		fNum <<= 12;
		return fNum;
	}
}

/***********************************************************************************************/

/***********************************************************************************************/
/***********************************************************************************************/
/***********************************************************************************************/
/***********************************************************************************************/
/***********************************************************************************************/

///============================================================================================
/// Dealing with page directory entry flags

inline uint32 pd_is_table_used(uint32* page_directory, uint32 virtual_address)
{
	return ( (page_directory[PDX(virtual_address)] & PERM_USED) == PERM_USED ? 1 : 0);
}

inline void pd_set_table_unused(uint32* page_directory, uint32 virtual_address)
{
	page_directory[PDX(virtual_address)] &= (~PERM_USED);
	tlb_invalidate((void *)NULL, (void *)virtual_address);
}

inline void pd_clear_page_dir_entry(uint32* page_directory, uint32 virtual_address)
{
	page_directory[PDX(virtual_address)] = 0 ;
	tlbflush();
}
