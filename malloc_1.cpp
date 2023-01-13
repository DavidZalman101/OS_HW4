/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%OS_HW_WET_4%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%%~~PART_I~~%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*
 *   ---------------------------------------------------------------------------------------------------- 
 *  |													 |
 *  |	Tries to allocate 'size' bytes									 |
 *  |	Return value:											 |
 *  |			(i).  Success - a pointer to the first allocated byte within the allocated block.|
 *  |			(ii). Failure - 								 |
 *  |					a.	If 'size' is 0 return NULL.				 |
 *  |					b.	If 'size' is more than 10^8, return NULL.		 |
 *  |					c.	If brsk fails, return NULL.				 |
 *  |													 |
 *   ---------------------------------------------------------------------------------------------------- 
 *	Notes:
 *		>	size_t is a typedef to unsigned int in 32-bit architectures, and to unsigned long long
 *			in 64-bit architectures. This means that trying to insert a negative value will result
 *			in compiler warning.
 *
 *		> You do not need to implement free(), calloc() or realloc() for this section.
 *
 *	Discussion:
 *			Before proceedingm try discussing the current implementation with you partner.
 *			What's wrong with it? What's missing? Are we handling fragmentation? What would you
 *			do differently.
 * */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%%~~DEFINES~~%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef _BIG_NUMBER_
#define _BIG_NUMBER_ 100000000
#endif

#ifndef _ERROR_SBRK_
#define _ERROR_SBRK_ -1
#endif

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%~~IMPLEMENTAION~~%%%%%%%%%%%%%%%%%%%%%*/
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <unistd.h>
void* smalloc(size_t size)
{
	/* Check arguments */
	//TODO: It was wise to check <= 0 right?
	if( size <= 0 || size > _BIG_NUMBER_ )
		return NULL;

	/* Increment the program break by 'size' */
	void* program_break = sbrk(size);

	if( program_break == (void*) _ERROR_SBRK_ )
		return NULL;

	/* Now that we allocated memory we will return the previous 'current_program_break' (dont forget to ++) */
	return program_break;
}

/*
 *	What wrong with my implementation?
 *	I'm just moving the program break.
 *	I'm not saving\remembering anywhere what I've allocated and where and how much.
 *
 *	Are we handling fragmentation?
 *	Hell no... All I'm doing is pushing the program break...
 *	
 *	What would I do differently?
 *	Create a Data structure which follows the allocated and free blocks of memory (addresses + sizes)
 *	and maintain it.
 * */
