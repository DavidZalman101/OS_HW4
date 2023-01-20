/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%~~OS_HW_WET_4~~%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%~~PART_II~~%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%%~~INCLUDES~~%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <unistd.h>
#include <cstdint>
#include <stdexcept>
#include <string.h>


/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%%%~~DEFINES~~%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef _BIG_NUMBER_
#define _BIG_NUMBER_ (100000000)
#endif

#ifndef _ERROR_SBRK_
#define _ERROR_SBRK_ (-1)
#endif

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%~~EXCEPTIONS~~%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class Exception 			: public std::exception 	{};

class GotNullArgument 		: public Exception 			{};

class ErrorSBRK 			: public Exception 			{};

class ErrorBlockNotInList	: public Exception			{};

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%%%~~LIST~~%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

// Momory block struct (aka. node in list)
typedef struct MallocMetadata {
		size_t		size;
		bool		is_free;
        MallocMetadata*	next;	
        MallocMetadata*	prev;	
}MMD;

// The list
class LIST_MMD {
private:
	MMD* list_head;
	MMD* list_tail;
	int  number_of_nodes;

	void _insert_(MMD*  block);

	bool _node_is_in_list_(MMD* block);

public:

	LIST_MMD() : list_head(NULL), list_tail(NULL), number_of_nodes(0) {}

	void _free_		(void* ptr); 

	void* _allocate_block_ 	(size_t size);

	MMD* _get_mmd_		(void* ptr);

	/* Choose to make a method instead of having varibles whom need to be taken care of during runtime */
	int _get_num_blocks_free_();
	int _get_num_bytes_free_ ();

	int _get_num_blocks_used_();
	int _get_num_bytes_used_ ();

};

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%~~LIST METHODS IMPLEMENTATIONS~~%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*_____________________________________________________________________________________________________________*/

/*
 * 	Inserts the given block of memory into the end of the list.
 * 	Reminder: This is a sorted list, by size, nodes with similar sizes are sorted by address.
 * 
 *	Return value: None 
 */

void LIST_MMD::_insert_(MMD* block) 
{
	/* Check arguments  */
	if (block == NULL)
		throw GotNullArgument();

	block->next = NULL;
	block->prev = NULL;

	if (number_of_nodes == 0) {
		/* This is the first node in the list */
		list_head = block;
		list_tail = block;
	}
	else {
		/* The list is not empty */
		list_tail->next = block;
		block->prev = list_tail;
		
		list_tail = block;
	}
	number_of_nodes++;
}

/*_____________________________________________________________________________________________________________*/

/*
 *	Assign the block of ptr to be 'free' 
 * 
 * 	Return value: None
 */

void LIST_MMD:: _free_(void* ptr)
{
	try
	{
		/* Check arguments */
		if( ptr == NULL )
		{
			return;
		}
		MMD* block_to_free     = _get_mmd_(ptr);
		block_to_free->is_free = true;	
	}
	catch(Exception& e)
	{
		throw e;
	}
}

/*_____________________________________________________________________________________________________________*/

/*
 *	We iterate over the list of memory nodes with hopes to find a node which contains a size big enough
 *	to the size asked for.
 *	If found: great, announce him as 'not_free' and return his pointer.
 *
 *	Other wise: push the program break 'size' bytes and add him to the list.
 *
 *	Return value: a pointer to the memory (not the MMD*)
 */

void* LIST_MMD:: _allocate_block_(size_t size)
{
	/* Try and find a block that already exists which is free and his size is >= of 'size'  */	
	MMD* ptr = list_head;
	while( ptr != NULL )
	{
		/* Check if we found a memory block which is free and has enough space  */
		if( ptr->is_free == true && ptr->size >= size )
		{
			ptr->is_free = false;
			return (char*)ptr + sizeof(MMD);
		}
		ptr = ptr->next;
	}

	/* We didn't find a free block which is big enough  */
	/* We must allocate more memory on the heap         */


	size_t allocation_size = size + sizeof(MMD);
	void* program_break    = sbrk(allocation_size);

	/* Check if we succesfully allocated more memory (moved the program break) */
	if( program_break == (void*)_ERROR_SBRK_ )
		throw ErrorSBRK();


	MMD* new_block_to_add     = (MMD*)program_break;

	new_block_to_add->size    = size;
	new_block_to_add->is_free = false;
	new_block_to_add->next    = NULL;
	new_block_to_add->prev    = NULL;

	try
	{
		_insert_(new_block_to_add);
		return (char*)new_block_to_add + sizeof(MMD);
	}
	catch(Exception& e)
	{
		throw(e);
	}
	return NULL;
}

/*_____________________________________________________________________________________________________________*/

MMD* LIST_MMD::_get_mmd_(void* ptr)
{
	/* Check argument  */
	if( ptr == NULL )
		throw(GotNullArgument());
	
	return (MMD*)( (char*)ptr - sizeof(MMD) );
}

/*_____________________________________________________________________________________________________________*/

int LIST_MMD::_get_num_blocks_free_()
{
	MMD* ptr     = list_head;
	int  counter = 0;

	while (ptr != NULL) {
		if( ptr->is_free == true )
			counter++;

		ptr = ptr->next;
	}	
	return counter;
}

/*_____________________________________________________________________________________________________________*/

int LIST_MMD::_get_num_bytes_free_ ()
{
	MMD*  ptr    = list_head;
	int  counter = 0;

	while (ptr != NULL) {
		if( ptr->is_free == true )
			counter += (int)ptr->size;

		ptr = ptr->next;
	}	
	return counter;
}

/*_____________________________________________________________________________________________________________*/

int LIST_MMD::_get_num_blocks_used_()
{
	MMD* ptr     = list_head;
	int  counter = 0;

	while (ptr != NULL) {
		if( ptr->is_free == false )
			counter++;
		ptr = ptr->next;
	}	
	return counter;
}

/*_____________________________________________________________________________________________________________*/

int LIST_MMD::_get_num_bytes_used_ ()
{
	MMD*  ptr    = list_head;
	int  counter = 0;

	while( ptr != NULL )
	{
		if( ptr->is_free == false )
			counter += (int)ptr->size;

		ptr = ptr->next;
	}	
	return counter;
}

/*_____________________________________________________________________________________________________________*/

bool LIST_MMD::_node_is_in_list_(MMD* block)
{
	/* Check arguments */
	if( block == NULL )
		throw GotNullArgument();

	MMD* ptr = list_head;
	while (ptr != NULL) {
		if( block == ptr )
			return true;

		ptr = ptr->next;
	}
	return false;
}

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%%~~GLOBAL_LIST~~%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

LIST_MMD memory_list_ = LIST_MMD();

/*
 *   ---------------------------------------------------------------------------------------------------- 
 *  |	void* smalloc(size_t size)									 |
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

void* smalloc(size_t size)
{
	/* Check arguments */
	if( size <= 0 || size > _BIG_NUMBER_ )
		return NULL;

	try
	{
		/* Allocate  */
		void* ptr = memory_list_._allocate_block_(size);

		if( ptr == NULL )
			return NULL;

		return ptr;
	}
	catch(Exception& e)
	{
		return NULL;
	}
	return NULL;
}

/*
 *   ---------------------------------------------------------------------------------------------------- 
 *  |	void* scalloc(size_t num, size_t size)							         |
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

void* scalloc(size_t num, size_t size)
{
	/* Check arguments */
	if( num <= 0 || size > _BIG_NUMBER_ )
		return NULL;	
	
	try
	{
		/* Find/Allocate num*size bytes  */
		void* ptr = smalloc(num * size);
		if( ptr == NULL )
			return NULL;

		/* Zero them up  */
		return memset(ptr,0,num*size);
	}
	catch(Exception& e)
	{
		return NULL;
	}
	return NULL;
}

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

void sfree(void* p)
{
	/* Check arguments */
	if( p == NULL )
		return;

	try
	{
		// Presuming 'p' truly points to a begining of an allocated block
		memory_list_._free_(p);
	}
	catch(Exception& e)
	{
		return;
	}
}

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

void* srealloc(void* oldp, size_t size)
{
	/* Check arguments */
	if( size <= 0 || size > _BIG_NUMBER_ )
		return NULL;

	try
	{
		if( oldp == NULL )
			return smalloc(size);

		/* Find the current size of the block of oldp */
		size_t size_of_oldp_block = memory_list_._get_mmd_(oldp)->size;

		/* Do we really need to allocate a new memory block? */
		//No
		if( size_of_oldp_block >= size )
			return oldp;
		//Yes
		else
		{
			/* Add a new block with 'size' into the list */
			void* newp = smalloc(size);

			if( newp == NULL )
				return NULL;
			
			/* Copy the data to the new block */
			if( memmove(newp, oldp, size_of_oldp_block) != newp )
				return NULL;	
			/* Succesfully allocated+moved the memory */
			/* Free the oldp block  */
			memory_list_._free_(oldp);
			return newp;
		}
	}
	catch(Exception& e)
	{
		return NULL;
	}
	return NULL;
}

/*
 *   ---------------------------------------------------------------------------------------------------- 
 *  |	size_t _num_free_blocks():          							         |
 *  |													 |
 *  |	Returns the number of allocated blocks in the heap that are currently free.                      |
 *  |													 |
 *   ---------------------------------------------------------------------------------------------------- 
 * */

size_t _num_free_blocks()
{
	return memory_list_._get_num_blocks_free_();
}

/*
 *   ---------------------------------------------------------------------------------------------------- 
 *  |	size_t _num_free_bytes():          							         |
 *  |													 |
 *  |	Returns the number of bytes in all allocated blocks in the heap that are currently free,         |
 *  |	exluding the bytes used by the meta-data structs.						 |
 *  |													 |
 *   ---------------------------------------------------------------------------------------------------- 
 * */

size_t _num_free_bytes()
{
	return memory_list_._get_num_bytes_free_();
}


size_t _num_allocated_blocks()
{
	return (memory_list_._get_num_blocks_used_() + memory_list_._get_num_blocks_free_());
}

/*
 *   ---------------------------------------------------------------------------------------------------- 
 *  |	size_t _num_allocated_bytes():      							         |
 *  |													 |
 *  |	Returns the overall number (free and used) of allocated bytes in the heap, excluding the bytes   |
 *  |   the bytes used by the meta-data structs.							 |
 *  |													 |
 *   ---------------------------------------------------------------------------------------------------- 
 * */

size_t _num_allocated_bytes()
{
	return (memory_list_._get_num_bytes_used_() + memory_list_._get_num_bytes_free_());
}

/*
 *   ---------------------------------------------------------------------------------------------------- 
 *  |	size_t _num_meta_data_bytes():      							         |
 *  |													 |
 *  |	Returns the overall number of meta-data bytes currently in the heap.                             |
 *  |													 |
 *   ---------------------------------------------------------------------------------------------------- 
 * */

size_t _num_meta_data_bytes()
{
	return ((memory_list_._get_num_blocks_free_() + memory_list_._get_num_blocks_used_()) * sizeof(MMD));
}

/*
 *   ---------------------------------------------------------------------------------------------------- 
 *  |	size_t _size_meta_data():           							         |
 *  |													 |
 *  |	Returns the number of bytes of a single meta-data structure in your system.                      |
 *  |													 |
 *   ---------------------------------------------------------------------------------------------------- 
 * */

size_t _size_meta_data()
{
	return sizeof(MMD);
}
