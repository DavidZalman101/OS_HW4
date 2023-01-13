/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%OS_HW_WET_4%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%~~PART_II~~%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%%%~~LIST~~%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <cstdint>
#include <stdexcept>


#ifndef _BIG_NUMBER_
#define _BIG_NUMBER_ 100000000
#endif

#ifndef _ERROR_SBRK_
#define _ERROR_SBRK_ -1
#endif



class Exception 	: public std::exception {};

class GotNullArgument 	: public Exception 	{};

class ErrorSBRK 	: public Exception 	{};

typedef struct MallocMetadata {
	size_t		size;
	bool		is_free;
        MallocMetadata*	next;	
        MallocMetadata*	prev;	
}MMD;

class LIST_MMD {
private:
	MMD* list_head;
	int  number_of_nodes;
public:
	LIST_MMD() : list_head(NULL) {}
	
	void _insert_		(MMD*  block);

	void _free_		(void* ptr); 

	void* _allocate_block_ 	(size_t size);

	MMD* _get_mmd_		(void* ptr);

};

MMD* LIST_MMD::_get_mmd_(void* ptr)
{
	/* Check argument  */
	if( ptr == NULL )
		throw(GotNullArgument());
	/*
	 *--> [ Allocation i ]
	 *--> [ Meta-Data  i ]
	 * */
	return (MMD*)( (uint8_t*)ptr - sizeof(MMD) );
}

void LIST_MMD::_insert_(MMD* block)
{
	//NOTE: once the 'size' field in the metadata is set for a block in this section in the metadata
	//	it's now going to change! I could optimize it and merge blocks. But those are the demands.
	
	//NOTE: This is a sorted list. Meaning the address of a block_a which comes before block_b is lower
	//	than block_b address.

	/* Check arguments  */
	if( block == NULL )
		throw(GotNullArgument());

	block->next = NULL;
	block->perv = NULL;

	if( number_of_noded == 0 )
		/* This is the first node in the list */
		list_head = block;
	else
	{
		MMD* ptr  = list_head;	
		MMD* prev = NULL;

		/* insert the node to the end of the list */
		/* find the last node (prev) */
		while(ptr)
		{
			prev = ptr;
			ptr = ptr->next;
			
		}	
		prev->next  = block;
		block->prev = prev;
	}
	number_of_nodes++;
}


void LIST_MMD:: _free_(void* ptr)
{
	/* Check arguments  */
	if( ptr == NULL )
		throw(GotNullArgument());
	try
	{
		MMD* block_to_free  = _get_mmd_(ptr);
		block_to_free->free = true;	
	}
	catch(Exception& e)
	{
		throw e;
	}
}


void* LIST_MMD:: _allocate_block_(size_t size)
{
	/* Try and find a block that already exists which is free and his size is >= of 'size'  */	
	MMD* ptr = list_head;
	while(ptr)
	{
		/* Check if we found a memory block which is free and has enough space  */
		if( ptr->is_free == true && ptr->size >= size )
		{
			ptr->is_free = false;
			return ptr;
		}
	}

	/* We didn't find a free block which is big enough  */
	/* We must allocate more memory on the heap         */

	size_t allocation_size = size + sizeof(MMD);
	void* program_break = sbrk(allocation_size);

	/* Check if we succesfully allocated more memory (moved the program break) */
	if( program_break == (void*)_ERROR_SBRK_ )
		throw ErrorSBRK();

	MMD* new_block_to_add = (MMD*)program_break;

	new_block_to_add->size = size;
	new_block_to_add->is_free = false;
	new_block_to_add->next = NULL;
	new_block_to_add->prev = NULL;

	
	try
	{
		_insert_(new_block_to_add);
		return program_break;
	}
	catch(Exception& e)
	{
		throw(e);
	}
	return NULL;
}





/*
 *   ---------------------------------------------------------------------------------------------------- 
 *  |	void* smalloc(size_t_size)									 |
 *  |													 |
 *  |	Searches for a free block with at least 'size' bytes or allocated (sbrk()) one if none are found.|
 *  |													 |
 *  |	Return value:											 |
 *  |			(i).  Success - a pointer to the first allocated block (exluding the meta-data   |
 *  | 					of course.							 |
 *  |			(ii). Failure - 								 |
 *  |					a.	If 'size' is 0 return NULL.				 |
 *  |					b.	If 'size' is more than 10^8, return NULL.		 |
 *  |					c.	If brsk fails in allocating the needed space,return NULL.|
 *  |													 |
 *   ---------------------------------------------------------------------------------------------------- 
 * */

/*
 *   ---------------------------------------------------------------------------------------------------- 
 *  |	void* scalloc(size_t_num, size_t size)							         |
 *  |													 |
 *  |	Searches for a free block with at least 'num' elements, each 'size' bytes that are all set to 0  |
 *  |   or allocates if none are found. In other words, find/allocate 'size' * 'num' bytes and set all   |
 *  |	bytes to 0. 											 |
 *  |													 |
 *  |	Return value:											 |
 *  |			(i).  Success - a pointer to the first byte in the allocted block.               |
 *  |					          							 |
 *  |			(ii). Failure - 								 |
 *  |					a.	If 'size' is 0 return NULL.				 |
 *  |					b.	If 'size' is more than 10^8, return NULL.		 |
 *  |					c.	If brsk fails in allocating the needed space,return NULL.|
 *  |													 |
 *   ---------------------------------------------------------------------------------------------------- 
 * */

/*
 *   ---------------------------------------------------------------------------------------------------- 
 *  |	void sfree(void* p)                 							         |
 *  |													 |
 *  |	Releases the usage of the block that starts with the pointer 'p'.                                |
 *  |   If 'p' is NULL or already released, simply returns.                                              |
 *  |	Presume that all pointers 'p' truly points to the begining of an allocated block.                |
 *  |													 |
 *   ---------------------------------------------------------------------------------------------------- 
 * */

/*
 *   ---------------------------------------------------------------------------------------------------- 
 *  |	void* srealloc(void* oldp, size_t size)							         |
 *  |													 |
 *  |	If 'size' is smaller than or equal to the current block's size, reuse the same block.            |
 *  |   Other wise, find/allocates 'size' bytes for a new space, copies content of oldp into the new     |
 *  |	allocated space and frees the oldp.								 |
 *  |													 |
 *  |	Return value:											 |
 *  |			(i).  Success -                                                                  |
 *  |					a. Retruns pointer to the first byte in the (newly) allocated    |
 *  |  			    		   space.							 |
 *  |     				b. If 'oldp' is NULL, allocate space for 'size' bytes and returns|	
 *  |					   a pointer to it.						 |
 *  |					          							 |
 *  |			(ii). Failure - 								 |
 *  |					a.	If 'size' is 0 return NULL.				 |
 *  |					b.	If 'size' is more than 10^8, return NULL.		 |
 *  |					c.	If brsk fails in allocating the needed space,return NULL.|
 *  |					d.	Do not free 'oldp' if srealloc() falis.			 |
 *  |													 |
 *   ---------------------------------------------------------------------------------------------------- 
 * */

/*
 *   ---------------------------------------------------------------------------------------------------- 
 *  |	size_t _num_free_blocks():          							         |
 *  |													 |
 *  |	Returns the number of allocated blocks in the heap that are currently free.                      |
 *  |													 |
 *   ---------------------------------------------------------------------------------------------------- 
 * */

/*
 *   ---------------------------------------------------------------------------------------------------- 
 *  |	size_t _num_free_bytes():          							         |
 *  |													 |
 *  |	Returns the number of bytes in all allocated blocks in the heap that are currently free,         |
 *  |	exluding the bytes used by the meta-data structs.						 |
 *  |													 |
 *   ---------------------------------------------------------------------------------------------------- 
 * */

/*
 *   ---------------------------------------------------------------------------------------------------- 
 *  |	size_t _num_allocated_bytes():      							         |
 *  |													 |
 *  |	Returns the overall number (free and used) of allocated bytes in the heap, excluding the bytes   |
 *  |   the bytes used by the meta-data structs.							 |
 *  |													 |
 *   ---------------------------------------------------------------------------------------------------- 
 * */

/*
 *   ---------------------------------------------------------------------------------------------------- 
 *  |	size_t _num_meta_data_bytes():      							         |
 *  |													 |
 *  |	Returns the overall number of meta-data bytes currently in the heap.                             |
 *  |													 |
 *   ---------------------------------------------------------------------------------------------------- 
 * */

/*
 *   ---------------------------------------------------------------------------------------------------- 
 *  |	size_t _size_meta_data():           							         |
 *  |													 |
 *  |	Returns the number of bytes of a single meta-data structure in your system.                      |
 *  |													 |
 *   ---------------------------------------------------------------------------------------------------- 
 * */



