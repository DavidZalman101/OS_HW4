/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%OS_HW_WET_4%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%~~PART_III~~%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%%~~INCLUDES~~%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <unistd.h>
#include <cstdint>
#include <stdexcept>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%%%~~DEFINES~~%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef _BIG_NUMBER_
#define _BIG_NUMBER_ 100000000
#endif

#ifndef _ERROR_SBRK_
#define _ERROR_SBRK_ -1
#endif

#ifndef _COOKIE_BOUND_
#define _COOKIE_BOUND_ 10000
#endif

#ifndef _MMAP_MIN_SIZE_
#define _MMAP_MIN_SIZE_ 131072
#endif

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%~~EXCEPTIONS~~%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class Exception 		: public std::exception {};

class GotNullArgument 	: public Exception 		{};

class ErrorSBRK 		: public Exception 		{};

class CookieError		: public Exception 		{};

class MmapError			: public Exception 		{};




// GLOBAL COOKIE
int global_cookie = rand() % _COOKIE_BOUND_;



/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%%%~~LIST~~%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

// Momory block struct (aka. node in list)
typedef struct MallocMetadata {
	int 		_cookie_;
	size_t		size;
	bool		is_free;
        MallocMetadata*	next;	
        MallocMetadata*	prev;	
}MMD;

// The list
class LIST_MMD {
private:

	int  number_of_nodes_free;
	int  number_of_nodes_used;
	int  number_of_nodes_mmap;

public:

	MMD* list_free_head;
	MMD* list_used_head;
	MMD* list_mmap_head;

	MMD* wilderness_node;


	void _insert_free_list_	(MMD*  block);

	void _insert_used_list_	(MMD*  block);

	void _insert_mmap_list_	(MMD*  block);

	void _remove_free_list_	(MMD*  block);

	void _remove_used_list_	(MMD*  block);
	
	void _remove_mmap_list_ (MMD* node);

	//void _remove_mmap_list_	(MMD*  block);

	void _update_wilderness_();

	bool _node_is_in_list_free_ (MMD* node);

	bool _node_is_in_list_used_ (MMD* node);

	bool _node_is_in_list_mmap_ (MMD* node);

	MMD* _get_left_neighbor_(MMD* node);

	MMD* _get_right_neighbor_(MMD* node);

	MMD* get_wilderness_node();

	MMD* _merge_nodes_(MMD* node_left, MMD* node_right);

	LIST_MMD() : list_free_head(NULL),    list_used_head(NULL),    list_mmap_head(NULL),   wilderness_node(NULL),
       	         number_of_nodes_free(0), number_of_nodes_used(0), number_of_nodes_mmap(0) {}

	void _free_		(void* ptr); 

	MMD* _allocate_block_ 	(size_t size);

	MMD* _get_mmd_		(void* ptr);

	/* Choose to make a method instead of having varibles whom need to be taken care of during runtime */
	int _get_num_blocks_free_();

	int _get_num_bytes_free_ ();

	int _get_num_blocks_used_();

	int _get_num_bytes_used_ ();

};

// Implementation of the List methods


// Changes: the insert method was changed such so the list will be sorted
void LIST_MMD::_insert_free_list_(MMD* block)
{
	//NOTE: This is a sorted list (by size)
	//	nodes equal by size are sorted by address

	/* Check arguments  */
	if( block == NULL )
		throw(GotNullArgument());

	block->next = NULL;
	block->prev = NULL;

	int block_size = block->size;

	if( number_of_nodes_free == 0 )
		/* This is the first node in the list */
		list_free_head = block;
	else
	{
		MMD* following  = list_free_head;
		MMD* prev	= NULL;

		/* Insert the node while keeping the list sorted */
		while( following != NULL )
		{
			if( following->size < block_size )
			{
				prev       = following;
				following  = following->next;
			}
			else if( following->size == block_size )
			{	
				while( following != NULL )
				{
					if( following < block && following->size == block_size )
					{
						prev       = following;
						following  = following->next;
					}
					else
						break;
				}
				break;
			}
		}

		/* Insert the node */
		prev->next 	= block;
		following->prev = block;

		block->prev 	= prev;
		block->next 	= following;
	}

	block->_cookie_ = global_cookie;
	block->is_free  = true;
	number_of_nodes_free++;
}

void LIST_MMD::_insert_used_list_(MMD* block)
{
	/* Check arguments */
	if( block == NULL )
		throw(GotNullArgument());

	/* Insert the block to the head of the list */
	block->next 	= NULL;
	block->prev 	= NULL;

	MMD* ptr 		= list_used_head;

	list_used_head  = block;
	block->next 	= ptr;
	block->is_free  = false;

	if( ptr != NULL )
		ptr->prev = list_free_head;
	
	number_of_nodes_used++;
}

void LIST_MMD::_insert_mmap_list_(MMD* block)
{
	if( block == NULL )
		throw(GotNullArgument());

	block->next 	= NULL;
	block->prev 	= NULL;

	MMD* ptr 		= list_mmap_head;

	list_mmap_head 	= block;
	block->next 	= ptr;
	block->is_free  = false;

	if( ptr != NULL )
		ptr->prev = block;
	
	number_of_nodes_mmap++;
}

bool LIST_MMD::_node_is_in_list_free_(MMD* node)
{
	/* Check arguments */
	if( node == NULL )
		throw(GotNullArgument());

	MMD* ptr = list_free_head;
	while( ptr != NULL )
	{
		if( ptr == node )
			return true;	
		ptr = ptr->next;
	}
	return false;
}

bool LIST_MMD::_node_is_in_list_used_(MMD* node)
{
	if( node == NULL )
		throw(GotNullArgument());

	MMD* ptr = list_used_head;
	while( ptr != NULL )
	{
		if( ptr == node )
			return true;	
		ptr = ptr->next;
	}
	return false;
}

bool LIST_MMD::_node_is_in_list_mmap_(MMD* node)
{
	if( node == NULL )
		throw(GotNullArgument());

	MMD* ptr = list_mmap_head;
	while( ptr != NULL )
	{
		if( ptr == node )
			return true;	
		ptr = ptr->next;
	}
	return false;
}

void LIST_MMD::_remove_free_list_(MMD* node)
{
	if( node == NULL || number_of_nodes_free == 0 )
		throw(GotNullArgument());

	// Special case: node is the head of the list
	if( node == list_free_head )
	{
		list_free_head = node->next;
		if( list_free_head != NULL )
			list_free_head->prev = NULL;
	}	
	else
	{
		MMD* node_prev = node->prev;
		MMD* node_next = node->next;

		node_prev->next = node_next;
		node_next->prev = node_prev;
	}
	number_of_nodes_free--;
}

void LIST_MMD::_remove_used_list_(MMD* node)
{
	if( node == NULL || number_of_nodes_used == 0 )
		throw(GotNullArgument());

	// Special case: node is the head of the list
	if( node == list_used_head )
	{
		list_used_head = node->next;
		if( list_used_head != NULL )
			list_used_head->prev = NULL;
	}	
	else
	{
		MMD* node_prev = node->prev;
		MMD* node_next = node->next;

		node_prev->next = node_next;
		node_next->prev = node_prev;
	}
	number_of_nodes_used--;
}

void LIST_MMD::_remove_mmap_list_(MMD* node)
{
	if( node == NULL || number_of_nodes_mmap == 0 )
		throw(GotNullArgument());

	// Special case: node is the head of the list
	if( node == list_mmap_head )
	{
		list_mmap_head = node->next;
		if( list_mmap_head != NULL )
			list_mmap_head->prev = NULL;
	}	
	else
	{
		MMD* node_prev = node->prev;
		MMD* node_next = node->next;

		node_prev->next = node_next;
		node_next->prev = node_prev;
	}
	number_of_nodes_mmap--;
}

MMD* LIST_MMD::_get_left_neighbor_(MMD* node)
{
	if( node == NULL )
		throw(GotNullArgument());

	MMD* ptr = memory_list_.list_free_head;

	while( ptr != NULL )
	{
		if( ptr + sizeof(MMD) + ptr->size == node )
			return ptr;
		ptr = ptr->next;
	}
	return NULL;
}

MMD* LIST_MMD::_get_right_neighbor_(MMD* node)
{
	if( node == NULL )
		throw(GotNullArgument());

	MMD* ptr = memory_list_.list_free_head;

	while( ptr != NULL )
	{
		if( node + sizeof(MMD) + node->size == ptr )
			return ptr;
		ptr = ptr->next;
	}
	return NULL;
}

MMD* LIST_MMD::get_wilderness_node()
{
	_update_wilderness_();
	return wilderness_node;
}

void LIST_MMD::_update_wilderness_()
{
	MMD* ptr_free = list_free_head;
	while( ptr_free != NULL )
	{
		if( ptr_free > wilderness_node )
			wilderness_node = ptr_free;
	}

	MMD* ptr_used = list_used_head;
	while( ptr_used != NULL )
	{
		if( ptr_used > wilderness_node )
			wilderness_node = ptr_used;
	}
}


//NOTE: The method takes into account that both nodes are not in the list
MMD* LIST_MMD::_merge_nodes_(MMD* node_left, MMD* node_right)
{
	if( node_left == NULL || node_right == NULL )
		throw(GotNullArgument());

	MMD* merged 		= node_left;
	merged->size 	   += (sizeof(MMD) + node_right->size);
	merged->prev 		= NULL;
	merged->next 		= NULL;
	merged->_cookie_ 	= global_cookie;
	merged->is_free 	= true;

	return merged;	
}

void LIST_MMD:: _free_(void* ptr)
{
	/* Check arguments  */
	if( ptr == NULL )
		throw(GotNullArgument());
		
	/* Check cookie */
	if(  _get_mmd_(ptr)->_cookie_ != global_cookie )
		throw(CookieError());

	try
	{
		MMD* block_to_free = _get_mmd_(ptr);

		/* Check if the block was allocated by mmap  */
		if( block_to_free->size >= _MMAP_MIN_SIZE_ )
		{
			_remove_mmap_list_(block_to_free);
			munmap(block_to_free,block_to_free->size + sizeof(MMD));
			return;
		}

		/* Remove the node from the used list */
		_remove_used_list_(block_to_free);

		/* Reset his parameters */
		block_to_free->next    = NULL;
		block_to_free->prev    = NULL;
		block_to_free->is_free = true;	

		/* Insert him to the free list */
		_insert_free_list_(block_to_free);
	}
	catch(Exception& e)
	{
		throw e;
	}
}

MMD* LIST_MMD::_allocate_block_(size_t size)
{
	if( size <= 0 || size > _BIG_NUMBER_ )
		throw(GotNullArgument());

	/* Check if size is too big for heap */
	if(  size >= _MMAP_MIN_SIZE_ )
	{
		MMD* new_block = (MMD*)mmap(NULL, size + sizeof(MMD), PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0);
		if( new_block == MAP_FAILED )
			throw(MmapError());
		
		new_block->is_free = false;
		new_block->size = size;
		new_block->next = NULL;
		new_block->prev = NULL;
		new_block->_cookie_ = global_cookie;
		_insert_mmap_list_(new_block);

		return new_block;
	}


	/* Try and find a block from the free list */	
	MMD* ptr = list_free_head;

	/* Remember, this is a sorted list by size */
	while(ptr)
	{
		/* Check if we found a memory block which  has enough space  */
		if( ptr->size >= size )
		{
			_remove_free_list_(ptr);
			ptr->is_free = false;
			ptr->next = NULL;
			ptr->prev = NULL; 
			ptr->_cookie_ = global_cookie;
			_insert_used_list_(ptr);				
			return ptr;
		}
		ptr = ptr->next;
	}

	/* We didn't find a free block which is big enough  */

	/* Check if 'wilderlness exists and could be enlarged enough  */
	_update_wilderness_();

	size_t allocation_size = 0;

	if( wilderness_node->is_free = true )
		/* wildernes is free, lets enlarge him just enough */
		size_t allocation_size = size - wilderness_node->size + sizeof(MMD); 
	else
		/* wilderness is not free, lets enlarge the heap by size */
		allocation_size = size + sizeof(MMD);

	/* push the heap */
	void* program_break    = sbrk(allocation_size);

	/* Check if we succesfully allocated more memory (moved the program break) */
	if( program_break == (void*)_ERROR_SBRK_ )
		throw ErrorSBRK();

	MMD* new_block_to_add     = (MMD*)program_break;

	new_block_to_add->size     = size;
	new_block_to_add->_cookie_ = global_cookie;
	new_block_to_add->is_free  = false;
	new_block_to_add->next     = NULL;
	new_block_to_add->prev     = NULL;

	try
	{
		_insert_used_list_(new_block_to_add);
		return (MMD*)program_break;
	}
	catch(Exception& e)
	{
		throw(e);
	}
	return NULL;
}

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

int LIST_MMD::_get_num_blocks_free_()
{
	MMD* ptr     = list_free_head;
	int  counter = 0;

	while( ptr != NULL )
	{
		if( ptr->is_free == true )
			counter++;
		ptr++;
	}	
	return counter;
}

int LIST_MMD::_get_num_bytes_free_ ()
{
	MMD*  ptr    = list_free_head;
	int  counter = 0;

	while( ptr != NULL )
	{
		if( ptr->is_free == true )
			counter += (int)ptr->size;
		ptr++;
	}	
	return counter;

}

int LIST_MMD::_get_num_blocks_used_()
{
	MMD* ptr     = list_used_head;
	int  counter = 0;

	while( ptr != NULL )
	{
		if( ptr->is_free == false )
			counter++;
		ptr++;
	}	
	return counter;
}

int LIST_MMD::_get_num_bytes_used_ ()
{
	MMD*  ptr    = list_used_head;
	int  counter = 0;

	while( ptr != NULL )
	{
		if( ptr->is_free == false )
			counter += (int)ptr->size;
		ptr++;
	}	
	return counter;
}

// GLOBAL LIST
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

void smalloc_helper(MMD* ptr, size_t size)
{
	/* Check arguments */
	if( size <= 0 || size > _BIG_NUMBER_ || ptr == NULL )
		throw(GotNullArgument());
	
	/* Check if block was allocated by mmap */
	if( size >= _MMAP_MIN_SIZE_ ) 
		return;

	MMD* block_ = (MMD*)ptr;

	/* Check if the block chosen is 'too big' and should be split */
	size_t size_rem = block_->size - size;
	if (size_rem < 128 )
		return;

	/* Is 'large enough' - lets split */

	/* update the size of the 'current allocation'  */
	//NOTE: we don't have to do anything else with the allocated block, he will stay in the used list
	block_->size = size;

	/* Create the remained block */
	MMD* block_rem 	   	= ( block_ + sizeof(MMD) + size );

	block_rem->_cookie_ = global_cookie;
	block_rem->size     = ( size_rem - sizeof(MMD) );
	block_rem->next     = NULL;
	block_rem->prev     = NULL;
	block_rem->is_free  = true;

	/* Insert him to the free block list */
	try
	{
		memory_list_._insert_free_list_(block_rem);
	}
	catch( Exception& e )
	{
		throw(e);
	}
}

void* smalloc(size_t size)
{
	/* Check arguments */
	if( size <= 0 || size > _BIG_NUMBER_ )
		return NULL;

	try
	{
		/* Allocate  */
		MMD* ptr = memory_list_._allocate_block_(size);

		if( ptr == NULL )
			return NULL;

		/* Check if needs 'spliting' */
		smalloc_helper(ptr,size);

		return ( (uint8_t*)ptr + sizeof(MMD) );
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


void sfree_helper(MMD* block)
{
	/* Check arguments */
	if( block == NULL )
		throw(GotNullArgument());

	/* Go over the free list and look for neighbors to merge with */
	MMD* left_neighbor  = NULL;
	MMD* right_neighbor = NULL;

	MMD* current = block;
	MMD* ptr     = memory_list_.list_free_head;

	while( ptr != NULL )
	{
		/* Check for left nieghbor */
		if( ptr + sizeof(MMD) + ptr->size == current )
			left_neighbor = ptr;

		/* Check for right niehgbor */
		else if( current + sizeof(MMD) + current->size == ptr )
			right_neighbor = ptr;
		
		ptr = ptr->next;
	}

	MMD* merged = NULL;

	/* Check if left neighbor should be merged */
	if( left_neighbor != NULL )
	{
		/* remove left_neighbor from the list */
		memory_list_._remove_free_list_(left_neighbor);

		/* merge the left_neighbor with current */
		merged = memory_list_._merge_nodes_(left_neighbor,current);
		if( merged == NULL )
			return;
	}
	
	/* Check if right neighbor should be merged */
	if( right_neighbor != NULL )
	{
		/* remove right_neighbor from the list */
		memory_list_._remove_free_list_(right_neighbor);

		/* merge the right with current/merged */
		if( merged != NULL )
		{
			merged = memory_list_._merge_nodes_(merged, right_neighbor);
			if( merged == NULL )
				return;
		}
		else
		{
			merged = memory_list_._merge_nodes_(current, right_neighbor);
			if( merged == NULL )
				return;
		}
	}

	/* Insert the merged blocks (if found) */
	if( merged != NULL )
	{
		/* Remove the current block from the free list */
		memory_list_._remove_free_list_(block);

		/* Insert the merged block */
		memory_list_._insert_free_list_(merged);	
	}
}

void sfree(void* p)
{
	/* Check arguments */
	if( p == NULL )
		return;

	try
	{
		/* Free the block */
		memory_list_._free_(p);

		/* Try and merge him */
		sfree_helper(memory_list_._get_mmd_(p));
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

void* srealloc_helper_mmap_block(void* oldp, size_t size)
{
	MMD* original_block = memory_list_._get_mmd_(oldp);
	if( original_block == NULL )
		return NULL;


	size_t size_of_oldp_block = original_block->size;

	/* Allocate a new block with 'size' size */
	MMD* new_block = (MMD*)mmap(NULL, size + sizeof(MMD), PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0);
	if( new_block == MAP_FAILED )
		return NULL;
	new_block->is_free = false;
	new_block->size = size;
	new_block->next = NULL;
	new_block->prev = NULL;
	new_block->_cookie_ = global_cookie;

	//Copy the data from the original block to the new block 
	if( memmove(new_block + sizeof(MMD), oldp, size_of_oldp_block) != new_block )
		return NULL;	

	/* Remove the old block from the mmap list  */
	memory_list_._remove_mmap_list_(original_block);

	/* Insert the new block to the mmap list */
	memory_list_._insert_mmap_list_(new_block);

	/* Delete\Clear the old block */
	if( munmap(original_block, size_of_oldp_block + sizeof(MMD)) != 0 )
		return NULL;
	return new_block + sizeof(MMD);
}

void*  srealloc_helper_wilderness(void* oldp, size_t size)
{
	/* Check arguments */
	if( oldp == NULL || size <= 0 || size > _BIG_NUMBER_ )
		throw GotNullArgument();

	/* Get the block for oldp */
	MMD* original_block = memory_list_._get_mmd_(oldp);

	/* remove him from the used list*/
	memory_list_._remove_used_list_(original_block);

	/* Check if the left  neighbor is free */
	MMD* left_neighbor = memory_list_._get_left_neighbor_(memory_list_._get_mmd_(oldp));
	
	MMD* merged = NULL;

	if( left_neighbor != NULL )
	{
		/* He exists! */
		/* remove the left neighbor from the list */
		memory_list_._remove_free_list_(left_neighbor);

		/* merge the nodes */
		merged = memory_list_._merge_nodes_(left_neighbor,original_block);
	}

	/* Check if we have enough space */
	if( merged->size < size )
	{
		/* We don't have enogh space */
		/* Enlarge the wilderness */
		size_t rem = size - merged->size;
		void* program_break = sbrk(rem);

		if( program_break == (void*)_ERROR_SBRK_ )
			throw ErrorSBRK();

		MMD* rem_node = (MMD*)program_break;
		rem_node->is_free = true;
		rem_node->next = NULL;
		rem_node->prev = NULL;
		rem_node->size = rem - sizeof(MMD);
		rem_node->_cookie_ = global_cookie;

		/* merge the nodes */
		merged = memory_list_._merge_nodes_(merged, rem_node);
		if( merged == NULL )
			return;
	}	
	/* We have enought space */
	/* Copy the data to merge */
	if( memmove(merged + sizeof(MMD), oldp, original_block->size) != merged + sizeof(MMD) )
		return;
	/* Insert to the used list */
	memory_list_._insert_used_list_(merged);

	return merged + sideof(MMD);
}


void* srealloc(void* oldp, size_t size)
{
	/* Check arguments */
	if( size <= 0 || size > _BIG_NUMBER_ || oldp == NULL )
		return NULL;


	try
	{
		MMD* original_block = memory_list_._get_mmd_(oldp);
		if( original_block == NULL )
			return NULL;

		size_t size_of_oldp_block = original_block->size;

		// a.
		/* Check if the current size is enough */
		if( size_of_oldp_block >= size )
			return;		
		
		/*  Check if we are dealing with a mmap block */
		if( size_of_oldp_block >= _MMAP_MIN_SIZE_ )
		{
			return srealloc_helper_mmap_block(oldp, size);
		}
		/* Check if we are the wilderness node */
		else if( memory_list_.get_wilderness_node() == original_block ) 
		{
			return srealloc_helper_wilderness(oldp, size);
		}
		/* We are just a node in the heap (not the wilderness node) */
		else
		{
			//TODO: continue from here
		}



		
		
		




	}
	catch(Exception& e)
	{
		return NULL;
	}
	



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
			void* newp = smalloc(size);
			if( newp == NULL )
				return NULL;
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
	return memory_list_._get_num_bytes_used_();
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
	return memory_list_._get_num_blocks_free_() * sizeof(MMD);
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

// SORTED LIST   []<-->[ ]<-->[  ]<-->[    ]<-->[     ]<-->...<-->[       ]<-->NULL
// DONE - Changed the method '_insert_'
/*
 *   ---------------------------------------------------------------------------------------------------- 
 *  |	Challenge 0 (Memory utilization):								 |
 *  |   Searching for an empty block in ascending order will cuase internal fragmentation. To mitigate   |
 *  |   this problem, we shall allocate the smallest block possible that fits the requested memory, that |
 *  |   way the internal fragmentation caused by this allocation will be as small as possible. 		 |
 *  | 													 |
 *  |	Solution: Change you current implementation, such that you maintain a sorted list (by size in    |
 *  |   ascending order, and if sizes are equal, then sord by the memory address) of all the free memory |
 *  |   such that allocations will use the 'tightest' fit possible i.e. the memory region with the       |
 *  |   minimal size that can fit the requested allocation.						 |
 *  | 													 |
 *   ---------------------------------------------------------------------------------------------------- 
 * */

// 		[]<-->...<-->[required size | extra size]<-->[Too big]<-->...<-->NULL
//	=>
// 		[]<-->...[extra size]<-->...<-->...<-->NULL
/*
 *   ---------------------------------------------------------------------------------------------------- 
 *  |   Challenge 1 (Memory utilization):								 |
 *  |   If we reuse freed memory sectors with bigger sizes than required, we'll be wasting memory 	 |
 *  |	(internal fragmentation).									 |
 *  |													 |
 *  |	Solution: Implement a function that 'smalloc()' will use, such that if pre-allocated block is    |
 *  |	reused and is large enough, the function will cut the block into two smaller blocks with two	 |
 *  |	separate meta-data structs. One will serve the current allocation, and another will remain 	 |
 *  |	unused for later (marked free and added to the list).						 |
 *  |	Definition of 'large enough': After splitting, the remaining block (the one that is not used)	 |
 *  |	has at least 128 bytes of free memory, excluding the size of your meta-data structure.		 |
 *  |	Note: Once again, you are not requested to find the 'best' free block for this section, but the  |
 *  |	first block that satisfies the allocation defined above.					 |
 *  |													 |
 *   ---------------------------------------------------------------------------------------------------- 
 * */



// 		[]<-->...[neighbor to the right]<-->[neighbor to the righr]<-->...<-->NULL
//	=>
// 		[]<-->...[merged neighbors]<-->...<-->NULL
/*
 *   ---------------------------------------------------------------------------------------------------- 
 *  |	Challenge 2 (Memory utilization):								 |
 *  |	Many allocations and de-allocations might cause two adjacent blocks to be free, but separate.	 |
 *  |													 |
 *  |	Solution: Implement a function that 'sfree()' will use, such that if one adjacent block (next or |
 *  |	previous) was free, the function will automatically combine free blocks (the current one and the |
 *  |	adjacent one) into one large free block. On the corner case where both the next and previous 	 |
 *  |	blocks are free, you should combine all 3 of them into one large block.				 |
 *  |													 |
 *   ---------------------------------------------------------------------------------------------------- 
 * */

// No node is big enough?
// Is wilderness a free block?
// Enlarge him 
/*
 *   ---------------------------------------------------------------------------------------------------- 
 *  |	Challenge 3 (Memory utilization):								 |
 *  |	Define the 'Wilderness' chunk as the topmost allocated chunk. Let's presume this chunk is free,  |
 *  |	and all others are full. It is possible that the new allocation requested is bigger than the 	 |
 *  |	wilderness block, thus requiring us to call 'sbrk()' once more - but now, it is easier to simply | 
 *  |	enlarge the wilderness block, saving us an addition of a meta-data structures.			 |
 *  |													 |
 *  |	Solution: Change you current implementation, such that if:					 |
 *  |		1. A new request has arrived, and no free memory chunk was found bit enough.		 |
 *  |		2. And the wilderness chunk is free.							 |
 *  |													 |
 *  |	Then enlarge the wilderness chunk enough to store the new requset.				 |
 *  |													 |
 *   ---------------------------------------------------------------------------------------------------- 
 * */

// size >= 128kb?
// Use mmap?...
/*
 *   ---------------------------------------------------------------------------------------------------- 
 *  |	Challege 4 (large allocations):									 |
 *  |	Recall from our first discussion that modern dynamic memory managers not only use sbrk() but 	 |
 *  |	also 'mmap()'. This process helps reduce the negative effect of memory fragmentation when large	 | 
 *  |	blocks of memory are freed but locked by smaller, more recently allocated blocks lying between   |
 *  |   them and the end of the allocated space. In this case, had the block been allocted with          |
 *  |	'sbrk()', it would have been probably remained unsued by the system for some time (or at least   |
 *  |	most of it).											 |
 *  |													 |
 *  |	Solution: Change your current implementatoin, by looking up how you can use 'mmap()' and munmap()|
 *  |	instead of 'sbrk()' for your memory allocation unit. Use this only for allocation that require   |
 *  |	128kb space or more (128 * 1024 B).								 |
 *  |													 |
 *   ---------------------------------------------------------------------------------------------------- 
 * */

// Cookie
// create a global cookie and add it to the metadata struct.
/*
 *   ---------------------------------------------------------------------------------------------------- 
 *  |	Challnge 5 (Safety & Security):									 |
 *  |	Consider the following case - a buffer overflow happens in the heap memory area (either on 	 |
 *  |	accident or on purpose), and this overflow overwrites the meta data bytes of some allocation with|
 *  |	arbitrary junk (or worse). Think - which problems can happen if we access this overwritten	 |
 *  |	metadata?											 |
 *  |													 |
 *  |	Solution: We can detect (but not only prevent) heap overflows using 'coockies' - 32bit integers  |
 *  |	that are placed in the metadata of each allocation. If an overflow happens, the cookie value will|
 *  |	Change and we can detect this before accessing the allocation's metadata by comparing the cookie |
 *  |	value with the expected cookie value.								 |
 *  |	You are required to add cookies to the allocation's meradata.					 |
 *  |	Note that cookie values should be randomized - otherwise they could be maliciously overwritten   |
 *  |	with that same constant value to overwrite detection. You can choose a global random value for   |
 *  |	all the cookies used in the same process.							 |
 *  |	Change your current implementation, such that before every metadata access, you should check if  |
 *  |	the relevant metadata cookie has changed. In case of overwrite detection, you should immediately |
 *  |	call exit(0xdeadbeef), as the process memory is corrupted and it cannot continue		 |
 *  |	Things to consider - 										 |
 *  |		1. When should you choose the random value?						 |
 *  |		2. Where should the cookie be placed in the metadata? most buffer overflows happen from  |
 *  |		   lower addresses to higher addresses)							 |
 *   ---------------------------------------------------------------------------------------------------- 
 * */





