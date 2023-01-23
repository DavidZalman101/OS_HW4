/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%~~OS_HW_WET_4~~%%%%%%%%%%%%%%%%%%%%%%%%*/
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
#define _BIG_NUMBER_ (100000000)
#endif

#ifndef _ERROR_SBRK_
#define _ERROR_SBRK_ (-1)
#endif

#ifndef _COOKIE_BOUND_
#define _COOKIE_BOUND_ (10000)
#endif

#ifndef _MMAP_MIN_SIZE_
#define _MMAP_MIN_SIZE_ (131072)
#endif

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%~~EXCEPTIONS~~%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class Exception 			: public std::exception {};

class ErrorSBRK 			: public Exception 		{};

class CookieError			: public Exception 		{};

class MmapError				: public Exception 		{};

class ArgumentError			: public Exception		{};

class ErrorBlockNotInList	: public Exception		{};

class ErrorBlockAlreadyInList	: public Exception		{};


/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%~~GLOBAL_COOKIE~~%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

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

	/* INSERT \ REMOVE METHODS */
	void _insert_free_list_	(MMD*  block);

	void _insert_used_list_	(MMD*  block);

	void _insert_mmap_list_	(MMD*  block);

	void _remove_free_list_	(MMD*  block);

	void _remove_used_list_	(MMD*  block);
	
	void _remove_mmap_list_ (MMD* node);

	/* UPDATE \ GET WIDLERNES */
	void _update_wilderness_();

	MMD* get_wilderness_node();

	/* NODE IS IN LIST */
	bool _node_is_in_list_free_ (MMD* node);

	bool _node_is_in_list_used_ (MMD* node);

	bool _node_is_in_list_mmap_ (MMD* node);

	/* GET RIGHT\ LEFT NEIGHBOR */
	MMD* _get_left_neighbor_(MMD* node);

	MMD* _get_right_neighbor_(MMD* node);


	/* MERGE NODES */
	MMD* _merge_nodes_(MMD* node_left, MMD* node_right);


	void _enlarge_wilderness(size_t size);

	/* CONSTRUCTOR */
	LIST_MMD() :  number_of_nodes_free(0), number_of_nodes_used(0), number_of_nodes_mmap(0),
				  list_free_head(NULL),    list_used_head(NULL),    list_mmap_head(NULL),   wilderness_node(NULL) {}

	/* FREE */
	void _free_		(void* ptr); 

	/* ALLOCATE BLOCK */
	void* _allocate_block_ 	(size_t size);

	/* GET MMD */
	MMD* _get_mmd_		(void* ptr);

	/* GET GENERAL DATA */
	int _get_num_blocks_free_();

	int _get_num_bytes_free_ ();

	int _get_num_blocks_used_();

	int _get_num_bytes_used_ ();

};

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%~~LIST METHODS IMPLEMENTATIONS~~%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*_____________________________________________________________________________________________________________*/

/* Insert the given block into the list of free memory nodes and update the number of nodes int he free list */

// NOTE: this is a sorted list: by size, nodes equal by size are sorted by address
void LIST_MMD::_insert_free_list_(MMD* block)
{
	/* Check arguments  */
	if( block == NULL )
		throw ArgumentError();
	
	if( _node_is_in_list_free_( block ) == true )
		throw ErrorBlockAlreadyInList();

	block->next = NULL;
	block->prev = NULL;

	size_t block_size = block->size;

	/* Check if this is the first node in the list */
	if( number_of_nodes_free == 0 )
	{
		
		list_free_head = block;
		block->_cookie_ = global_cookie;
		block->is_free  = true;
		number_of_nodes_free++;
		return;
	}
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
			else
				break;
		}

		/* Check if he needs to be inserted to the end of the list */
		if( following == NULL )
		{
			prev->next  = block;
			block->prev = prev;
			block->next = NULL;
		}
		/* Check if he needs to be inserted to the begining of the list */
		else if( prev == NULL )
		{
			following->prev = block;
			list_free_head  = block;
			block->prev = NULL;
			block->next = following;
		}
		else
		{
			/* Insert the node */
			prev->next 		= block;
			following->prev = block;

			block->prev 	= prev;
			block->next 	= following;
		}
	}

	block->_cookie_ = global_cookie;
	block->is_free  = true;
	number_of_nodes_free++;
}

/*_____________________________________________________________________________________________________________*/

/* Insert the node intio the used list and update the number of nodes in the list */
// NOTE: list is not ordered
void LIST_MMD::_insert_used_list_(MMD* block_to_insert)
{
	/* Check arguments */
	if( block_to_insert == NULL )
		throw ArgumentError();

	/* Insert the block to the head of the list */

	block_to_insert->next 	= NULL;
	block_to_insert->prev 	= NULL;
	block_to_insert->_cookie_ = global_cookie;

	MMD* ptr = list_used_head;

	list_used_head  		  = block_to_insert;
	block_to_insert->next 	  = ptr;
	block_to_insert->is_free  = false;

	if( ptr != NULL )
		ptr->prev = block_to_insert;
	
	number_of_nodes_used++;
}

/*_____________________________________________________________________________________________________________*/

/*Inser to the mmap list and update the number of nodes in the list */
// NOTE: list is not ordered
void LIST_MMD::_insert_mmap_list_(MMD* block_to_insert)
{
	if( block_to_insert == NULL )
		throw ArgumentError();

	block_to_insert->next 	= NULL;
	block_to_insert->prev 	= NULL;
	block_to_insert->_cookie_ = global_cookie;

	MMD* ptr 		= list_mmap_head;

	list_mmap_head 	= block_to_insert;
	block_to_insert->next 	= ptr;
	block_to_insert->is_free  = false;

	if( ptr != NULL )
		ptr->prev = block_to_insert;
	
	number_of_nodes_mmap++;
}

/*_____________________________________________________________________________________________________________*/

/* Check if the given node is in the free list */
bool LIST_MMD::_node_is_in_list_free_(MMD* node)
{
	/* Check arguments */
	if( node == NULL )
		throw ArgumentError();

	MMD* ptr = list_free_head;
	while( ptr != NULL )
	{
		if( ptr == node )
			return true;	
		ptr = ptr->next;
	}
	return false;
}

/*_____________________________________________________________________________________________________________*/

/* Check if the given node is in the used list */
bool LIST_MMD::_node_is_in_list_used_(MMD* node)
{
	if( node == NULL )
		throw ArgumentError();

	MMD* ptr = list_used_head;
	while( ptr != NULL )
	{
		if( ptr == node )
			return true;	
		ptr = ptr->next;
	}
	return false;
}

/*_____________________________________________________________________________________________________________*/

/* check if the given node is in the mmap list */
bool LIST_MMD::_node_is_in_list_mmap_(MMD* node)
{
	if( node == NULL )
		throw ArgumentError();

	MMD* ptr = list_mmap_head;
	while( ptr != NULL )
	{
		if( ptr == node )
			return true;	
		ptr = ptr->next;
	}
	return false;
}

/*_____________________________________________________________________________________________________________*/

/*  Remove the given node from the free list */
void LIST_MMD::_remove_free_list_(MMD* node)
{
	if( node == NULL || number_of_nodes_free == 0 )
		throw ArgumentError();

	/* node is the head of the list */
	if( node == list_free_head )
	{
		list_free_head = node->next;
		if( node->next != NULL)
			node->next->prev = NULL;
	}
	/* node is the end of the list */
	else if( node->next == NULL )
	{
		// since he isn't the head of the list, he has a left neighbor
		node->prev->next = NULL;
	}
	/* node is in the middle of the list (has real neighbors) */
	else
	{
		node->prev->next = node->next;
		node->next->prev = node->prev;
	}
	node->next = NULL;
	node->prev = NULL;
	number_of_nodes_free--;
}

/*_____________________________________________________________________________________________________________*/

/* Remove the given node from the used list */
void LIST_MMD::_remove_used_list_(MMD* node)
{
	if( node == NULL || number_of_nodes_used == 0 )
		throw ArgumentError();

	/* node is the head of the list */
	if( node == list_used_head )
	{
		list_used_head = node->next;
		if( node->next != NULL)
			node->next->prev = NULL;
	}
	/* node is the end of the list */
	else if( node->next == NULL )
	{
		// since he isn't the head of the list, he has a left neighbor
		node->prev->next = NULL;
	}
	/* node is in the middle of the list (has real neighbors) */
	else
	{
		node->prev->next = node->next;
		node->next->prev = node->prev;
	}
	node->next = NULL;
	node->prev = NULL;
	number_of_nodes_used--;
}

/*_____________________________________________________________________________________________________________*/

/* Remove the given node from the mmap list */
void LIST_MMD::_remove_mmap_list_(MMD* node)
{
	if( node == NULL || number_of_nodes_mmap == 0 )
		throw ArgumentError();

	/* node is the head of the list */
	if( node == list_mmap_head )
	{
		list_mmap_head = node->next;
		if( node->next != NULL)
			node->next->prev = NULL;
	}
	/* node is the end of the list */
	else if( node->next == NULL )
	{
		// since he isn't the head of the list, he has a left neighbor
		node->prev->next = NULL;
	}
	/* node is in the middle of the list (has real neighbors) */
	else
	{
		node->prev->next = node->next;
		node->next->prev = node->prev;
	}
	node->next = NULL;
	node->prev = NULL;
	number_of_nodes_mmap--;
}

/*_____________________________________________________________________________________________________________*/

/* Find the right neighbor of the given node only if he is free */
MMD* LIST_MMD::_get_left_neighbor_(MMD* node)
{
	if( node == NULL )
		throw ArgumentError();

	MMD* ptr = list_free_head;

	while( ptr != NULL )
	{
		void* res = (char*) ptr + sizeof(MMD) + ptr->size;
		if( res == node )
			return ptr;
		ptr = ptr->next;
	}
	return NULL;
}

/*_____________________________________________________________________________________________________________*/

/* Find the right neighbor of the given node only if he is free */
MMD* LIST_MMD::_get_right_neighbor_(MMD* node)
{
	if( node == NULL )
		throw ArgumentError();


	/* check if node is the wilderness */
	_update_wilderness_();
	if( node == wilderness_node )
		return NULL;
	
	/* He has a right neighbor */
	MMD* right_neighbor = (MMD*)( (char*)node + sizeof(MMD) + node->size );

	if( right_neighbor->is_free == true )
		return right_neighbor;

	return NULL;
}

/*_____________________________________________________________________________________________________________*/

/* Get the wilderness node */
MMD* LIST_MMD::get_wilderness_node()
{
	_update_wilderness_();
	return wilderness_node;
}

/*_____________________________________________________________________________________________________________*/

/* Update who is the wilderness node */
void LIST_MMD::_update_wilderness_()
{
	wilderness_node = NULL;
	MMD* ptr_free = list_free_head;
	while( ptr_free != NULL )
	{
		if( ptr_free > wilderness_node )
			wilderness_node = ptr_free;
		ptr_free = ptr_free->next;
	}

	MMD* ptr_used = list_used_head;
	while( ptr_used != NULL )
	{
		if( ptr_used > wilderness_node )
			wilderness_node = ptr_used;
		ptr_used = ptr_used->next;
	}
}

/*_____________________________________________________________________________________________________________*/

/* Merged 2 nodes into 1 node */
//NOTE: The method takes into account that both nodes are not in the list
MMD* LIST_MMD::_merge_nodes_(MMD* node_left, MMD* node_right)
{
	if( node_left == NULL || node_right == NULL )
		throw ArgumentError();

	MMD* merged 		= node_left;
	merged->size 	   += (sizeof(MMD) + node_right->size);
	merged->prev 		= NULL;
	merged->next 		= NULL;
	merged->_cookie_ 	= global_cookie;
	merged->is_free 	= true;

	return merged;	
}

/*_____________________________________________________________________________________________________________*/

/* Releases a node from the used node and inserts him to the free list */
void LIST_MMD:: _free_(void* ptr)
{
	/* Check arguments  */
	if( ptr == NULL )
		throw ArgumentError();
		
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

/*_____________________________________________________________________________________________________________*/

/* Allocates a new block of memory (mmp, sbrk, old free blocks) and inserts it to the list (used, mmap) */
void* LIST_MMD::_allocate_block_(size_t size)
{
	if( size <= 0 || size > _BIG_NUMBER_ )
		throw ArgumentError();

	/* Check if size is too big for heap */
	if(  size >= _MMAP_MIN_SIZE_ )
	{
		MMD* new_block = (MMD*)mmap(NULL, size + sizeof(MMD), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if( new_block == MAP_FAILED )
			throw MmapError();
		
		new_block->is_free = false;
		new_block->size = size;
		new_block->next = NULL;
		new_block->prev = NULL;
		new_block->_cookie_ = global_cookie;
		_insert_mmap_list_(new_block);

		return (char*)new_block + sizeof(MMD);
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
			return (char*)ptr + sizeof(MMD);
		}
		ptr = ptr->next;
	}

	/* We didn't find a free block which is big enough  */

	/* Check if 'wilderlness exists and could be enlarged enough  */
	_update_wilderness_();

	size_t allocation_size = 0;
	bool flag_expanding_wilderness = false;

	if( wilderness_node != NULL )
	{
		if( wilderness_node->is_free == true )
		{
			flag_expanding_wilderness = true;
			/* wildernes is free, lets enlarge him just enough */
			allocation_size = size - wilderness_node->size;
		}
		else
			allocation_size = size + sizeof(MMD);
	}

	else
		/* wilderness doesn't exist, lets enlarge the heap by size */
		allocation_size = size + sizeof(MMD);

	/* push the heap */
	void* program_break    = sbrk(allocation_size);

	/* Check if we succesfully allocated more memory (moved the program break) */
	if( program_break == (void*)_ERROR_SBRK_ )
		throw ErrorSBRK();

	MMD* new_block_to_add = NULL;

	if (flag_expanding_wilderness == true) {
		new_block_to_add = wilderness_node;
		//assert (wilderness_node == program_break - wilderness_node->size - sizeof(MMD));
		_remove_free_list_(wilderness_node);
	} else
		new_block_to_add = (MMD*)program_break;

	new_block_to_add->size     = size;
	new_block_to_add->_cookie_ = global_cookie;
	new_block_to_add->is_free  = false;
	new_block_to_add->next     = NULL;
	new_block_to_add->prev     = NULL;

	try
	{
		_insert_used_list_(new_block_to_add);
		return (char*)new_block_to_add + sizeof(MMD);
	}
	catch(Exception& e)
	{
		throw(e);
	}
	return NULL;
}

/*_____________________________________________________________________________________________________________*/

/* Gets the mmd of a pointer */
MMD* LIST_MMD::_get_mmd_(void* ptr)
{
	/* Check argument  */
	if( ptr == NULL )
		throw ArgumentError();

	return (MMD*)( (char*)ptr - sizeof(MMD) );
}

/*_____________________________________________________________________________________________________________*/

// EASY TO UNDERSTAND ...

int LIST_MMD::_get_num_blocks_free_()
{
	return number_of_nodes_free;
}

/*_____________________________________________________________________________________________________________*/

int LIST_MMD::_get_num_bytes_free_ ()
{
	MMD*  ptr    = list_free_head;
	int  counter = 0;

	while( ptr != NULL )
	{
		counter += (int)ptr->size;
		ptr = ptr->next;
	}	
	return counter;
}

/*_____________________________________________________________________________________________________________*/

int LIST_MMD::_get_num_blocks_used_()
{
	return number_of_nodes_used + number_of_nodes_mmap;
}

/*_____________________________________________________________________________________________________________*/

int LIST_MMD::_get_num_bytes_used_ ()
{
	MMD*  ptr    = list_used_head;
	int  counter = 0;

	while( ptr != NULL )
	{
		counter += (int)ptr->size;
		ptr = ptr->next;
	}	

	ptr = list_mmap_head;
	while( ptr != NULL )
	{
		counter += (int)ptr->size;
		ptr = ptr->next;
	}	

	return counter;
}

/*_____________________________________________________________________________________________________________*/


void LIST_MMD::_enlarge_wilderness(size_t size)
{
	if(size <=0 || size > _BIG_NUMBER_ )
		throw ArgumentError();

	void* program_break = sbrk(size);

	if( program_break == (void*)_ERROR_SBRK_ )
		throw ArgumentError();
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

/*_____________________________________________________________________________________________________________*/

void smalloc_helper(MMD* node, size_t size)
{
	/* Check arguments */
	if( size <= 0 || size > _BIG_NUMBER_ || node == NULL )
		throw ArgumentError();
	
	/* Check if block was allocated by mmap */
	if( size >= _MMAP_MIN_SIZE_ ) 
	{
		return;
	}

	MMD* block_ = node;

	/* Check if the block chosen is 'too big' and should be split */
	size_t size_rem = block_->size - size;
	if (size_rem < 128 + sizeof(MMD))
		return;

	/* Is 'large enough' -> lets split */

	/* update the size of the 'current allocation'  */
	//NOTE: we don't have to do anything else with the allocated block, he will stay in the used list
	block_->size = size;

	/* Create the remained block */
	MMD* block_rem 	   	= (MMD*)( (char*)block_ + sizeof(MMD) + size );

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

/*_____________________________________________________________________________________________________________*/

void* smalloc(size_t size)
{
	/* Check arguments */
	if( size <= 0 || size > _BIG_NUMBER_ )
		return NULL;

	try
	{
		/* Allocate */
		void* ptr = memory_list_._allocate_block_(size);

		if( ptr == NULL )
			return NULL;

		MMD* ptr_mmd = memory_list_._get_mmd_(ptr);

		/* Check if needs 'spliting' */
		smalloc_helper(ptr_mmd,size);

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

/*_____________________________________________________________________________________________________________*/

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

/*_____________________________________________________________________________________________________________*/

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

/*_____________________________________________________________________________________________________________*/

void sfree_helper(MMD* block)
{
	/* Check arguments */
	if( block == NULL )
		throw ArgumentError();

	memory_list_._remove_free_list_(block);

	/* Get the neighbors of 'block' if they exist */
	MMD* left_neighbor  = memory_list_._get_left_neighbor_(block);
	MMD* right_neighbor = memory_list_._get_right_neighbor_(block);

	MMD* merged = block;

	/* Check if left neighbor should be merged */
	if( left_neighbor != NULL )
	{
		/* remove left_neighbor from the list */
		memory_list_._remove_free_list_(left_neighbor);

		/* merge the left_neighbor with current */
		merged = memory_list_._merge_nodes_(left_neighbor,block);
	}
	
	/* Check if right neighbor should be merged */
	if( right_neighbor != NULL )
	{
		/* remove right_neighbor from the list */
		memory_list_._remove_free_list_(right_neighbor);

		/* merge the right with current/merged */
		merged = memory_list_._merge_nodes_(merged, right_neighbor);
	}

	/* Insert the merged block */
	memory_list_._insert_free_list_(merged);	
}

/*_____________________________________________________________________________________________________________*/

void sfree(void* p)
{
	/* Check arguments */
	if( p == NULL )
		return;

	if( memory_list_._get_mmd_(p)->_cookie_ != global_cookie )
		exit(0xdeadbeef);

	try
	{
		bool is_allocated_by_mmap = false;
		if( memory_list_._get_mmd_(p)->size >= _MMAP_MIN_SIZE_ )
			is_allocated_by_mmap = true;

		/* Free the block */
		memory_list_._free_(p);

		/* Try and merge him */
		if( is_allocated_by_mmap == false )
			sfree_helper(memory_list_._get_mmd_(p));
	}
	catch(Exception& e)
	{
		return;
	}
}

/*_____________________________________________________________________________________________________________*/

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

/*_____________________________________________________________________________________________________________*/

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
	if( memmove(new_block + sizeof(MMD), oldp, size_of_oldp_block ) != new_block + sizeof(MMD) )
		return NULL;	

	/* Remove the old block from the mmap list  */
	memory_list_._remove_mmap_list_(original_block);

	/* Delete\Clear the old block */
	if( munmap(original_block, size_of_oldp_block + sizeof(MMD)) != 0 )
		return NULL;

	/* Insert the new block to the mmap list */
	memory_list_._insert_mmap_list_(new_block);

	new_block->_cookie_ = global_cookie;
	return (char *)new_block + sizeof(MMD);
}

/*_____________________________________________________________________________________________________________*/

void* srealloc_helper_wilderness(void* oldp, size_t size)
{
	/* Check arguments */
	if( oldp == NULL || size <= 0 || size > _BIG_NUMBER_ )
		throw ArgumentError();

	/* Get the block for oldp */
	MMD* original_block = memory_list_._get_mmd_(oldp);

	/* remove him from the used list*/
	memory_list_._remove_used_list_(original_block);

	/* Check if the left neighbor is free */
	MMD* left_neighbor = memory_list_._get_left_neighbor_( original_block );
	
	MMD* merged = NULL;

	if( left_neighbor != NULL )
	{
		/* He exists! */
		/* remove the left neighbor from the list */
		memory_list_._remove_free_list_(left_neighbor);

		/* merge the nodes */
		merged = memory_list_._merge_nodes_(left_neighbor,original_block);
	}
	if(merged == NULL)
		merged = original_block;
	/* Check if we have enough space */
	if( merged->size < size )
	{
		/* We don't have enogh space */
		/* Enlarge the wilderness */
		size_t rem = size - merged->size;
		memory_list_._enlarge_wilderness(rem);
		merged->size = size;
	}	
	/* We have enought space */
	/* Copy the data to merge */
	if( memmove((char*)merged + sizeof(MMD), oldp, original_block->size) != (char*)merged + sizeof(MMD) )
		return NULL;
	/* Insert to the used list */
	merged->_cookie_ = global_cookie;
	memory_list_._insert_used_list_(merged);


	return (char*)merged + sizeof(MMD);
}

/*_____________________________________________________________________________________________________________*/

void* srealloc_helper_normal_node(void* oldp, size_t size)
{
	/* Check arguments */
	if( oldp == NULL || size <= 0 || size > _BIG_NUMBER_ )
		throw ArgumentError();
	
	MMD* original_block = memory_list_._get_mmd_(oldp);
	
	/* Remove the old block from the used list */
	memory_list_._remove_used_list_(original_block);

	/* Try and find the left neighbor */
	MMD* left_neighbor  = memory_list_._get_left_neighbor_(original_block);
	MMD* right_neighbor = memory_list_._get_right_neighbor_(original_block); 
	MMD* merged = NULL;

	if( left_neighbor != NULL )
	{
		/* Left neighbor exists! */
		/* remove the left neighbor from the free list */
		memory_list_._remove_free_list_(left_neighbor);

		/* merge the nodes */
		merged = memory_list_._merge_nodes_(left_neighbor, original_block);
	}	
	else
		/* Left neighbot doesn't exist */
		merged = original_block;

	/* Check if we don't have enoguth space */
    if( merged->size < size )
	{
		/* We don't have enough space */
		/* Check if right neighbor exists*/
		if( right_neighbor != NULL )
		{
			/* He eixsts! */
			/*  remove the right neighbor from the free list */
			memory_list_._remove_free_list_(right_neighbor);

			/* merge him */
			memory_list_._merge_nodes_(merged, right_neighbor);
		}
	}

	/* Check if we don't have enough space */
	if( merged->size < size )
	{
		/* We don't */
		/* CASE: right neighbor is the wilderness and free */
		if( right_neighbor != NULL )
		{
			memory_list_._insert_free_list_(merged);
			MMD* wilderness_ = memory_list_.get_wilderness_node();
			memory_list_._remove_free_list_(merged);
			if( merged == wilderness_ )
			{
				//NOTE: at this point, we already merged with the right neighbor.
				memory_list_._enlarge_wilderness(size - merged->size);
				merged->size = size;	
			}
		}
		else
		{
		/* CASE: our right neighbor is not the wilderness or he is but not free */
		/* We need to allocated a new block */
		void* new_pointer = smalloc(size);
		memory_list_._remove_used_list_(memory_list_._get_mmd_(new_pointer));
		memory_list_._insert_free_list_(merged);
		merged = memory_list_._get_mmd_(new_pointer);
		}
	}
	//NOTE: at this point, we have enough or more space.

	/* Check if we need splitting */
	int _rem_ = merged->size - size - sizeof(MMD);
	if(  _rem_ >= 128 )
	{
		/* Split */
		MMD* rem = (MMD*)((char*)merged + sizeof(MMD) + size);
		rem->is_free = true;
		rem->next = NULL;
		rem->prev = NULL;
		rem->size = _rem_; 
		rem->_cookie_ = global_cookie;
		
		merged->size = size;
		//Try and merged rem with his neighbors

		MMD* rem_merged = rem;
		MMD* rem_right_neighbor = memory_list_._get_right_neighbor_(rem);
		/*MMD* rem_left_neighbor = memory_list_._get_left_neighbor_(rem);
		if( rem_left_neighbor != NULL )
		{
			memory_list_._remove_free_list_(rem_left_neighbor);
			rem_merged = memory_list_._merge_nodes_(rem_left_neighbor, rem);
		}
		*/
		if( rem_right_neighbor != NULL )
		{
			memory_list_._remove_free_list_(rem_right_neighbor);
			rem_merged = memory_list_._merge_nodes_(rem_merged, rem_right_neighbor);
		}
		memory_list_._insert_free_list_(rem_merged);
	}
	/* Copy the data */
	if( memmove ( (char*)merged + sizeof(MMD), oldp, original_block->size) != (char*)merged + sizeof(MMD) )
		return NULL;
					
	/* insert eh new memory block to the used list */
	merged->_cookie_=global_cookie;
	memory_list_._insert_used_list_(merged);
	return (char*)merged + sizeof(MMD);
}

/*_____________________________________________________________________________________________________________*/

void* srealloc(void* oldp, size_t size)
{
	/* Check arguments */
	if( size <= 0 || size > _BIG_NUMBER_ )
		return NULL;

	try
	{
		if( oldp == NULL )
		{
			return smalloc(size);
		}

		if( memory_list_._get_mmd_(oldp)->_cookie_ != global_cookie )
			exit(0xdeadbeef);


		MMD* original_block = memory_list_._get_mmd_(oldp);
		if( original_block == NULL )
			return NULL;

		size_t size_of_oldp_block = original_block->size;
		if(size == size_of_oldp_block)
			return oldp;

		/* Check if the current size is enough */
		if( size_of_oldp_block >= size && size_of_oldp_block < _MMAP_MIN_SIZE_ ) {
			if((size_of_oldp_block - size) >= 128 + sizeof(MMD) ) {
				original_block->size = size;
				MMD* new_block = (MMD*)((char *)original_block + sizeof(MMD) + size);
				new_block->is_free = true;
				new_block->next = NULL;
				new_block->prev = NULL;
				new_block->size = size_of_oldp_block - size - sizeof(MMD);
				new_block->_cookie_ = global_cookie;
				memory_list_._insert_free_list_(new_block);
			}
			return oldp;	
		} else if (size_of_oldp_block >= size) {
			void* new_ptr = memory_list_._allocate_block_(size);
			memmove(new_ptr,oldp,size);
			memory_list_._remove_mmap_list_(memory_list_._get_mmd_(oldp));
			munmap(original_block, size_of_oldp_block + sizeof(MMD));
			return new_ptr;
		}
		
		/*  Check if we are dealing with a mmap block */
		if( size_of_oldp_block >= _MMAP_MIN_SIZE_ )
			return srealloc_helper_mmap_block(oldp, size);

		/* Check if we are the wilderness node */
		else if( memory_list_.get_wilderness_node() == original_block ) 
			return srealloc_helper_wilderness(oldp, size);

		/* We are just a node in the heap (not the wilderness node) */
		else
			return srealloc_helper_normal_node(oldp, size);
	}
	catch(Exception& e)
	{
		return NULL;
	}
}

/*_____________________________________________________________________________________________________________*/

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

size_t _num_allocated_blocks()
{
	return memory_list_._get_num_blocks_used_() + memory_list_._get_num_blocks_free_();
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
	return memory_list_._get_num_bytes_used_() + memory_list_._get_num_bytes_free_();
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
	return (memory_list_._get_num_blocks_free_() + memory_list_._get_num_blocks_used_()) * sizeof(MMD);
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
