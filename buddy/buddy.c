/**
 * Buddy Allocator
 *
 * For the list library usage, see http://www.mcs.anl.gov/~kazutomo/list/
 */

/**************************************************************************
 * Conditional Compilation Options
 **************************************************************************/
#define USE_DEBUG 0

/**************************************************************************
 * Included Files
 **************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#include "buddy.h"
#include "list.h"

/**************************************************************************
 * Public Definitions
 **************************************************************************/

#define ORDER_SIZE(order) (1 << (order))

#define MIN_ORDER 12
#define MAX_ORDER 20

#define PAGE_SIZE (1 << MIN_ORDER)
/* page index to address */
#define PAGE_TO_ADDR(page_idx) (void *)((page_idx*PAGE_SIZE) + g_memory)

/* address to page index */
#define ADDR_TO_PAGE(addr) ((unsigned long)((void *)addr - (void *)g_memory) / PAGE_SIZE)

/* find buddy address */
#define BUDDY_ADDR(addr, o) (void *)((((unsigned long)addr - (unsigned long)g_memory) ^ (1<<o)) \
									 + (unsigned long)g_memory)

#if USE_DEBUG == 1
#  define PDEBUG(fmt, ...) \
	fprintf(stderr, "%s(), %s:%d: " fmt,			\
		__func__, __FILE__, __LINE__, ##__VA_ARGS__)
#  define IFDEBUG(x) x
#else
#  define PDEBUG(fmt, ...)
#  define IFDEBUG(x)
#endif

/**************************************************************************
 * Public Types
 **************************************************************************/
typedef struct {
	struct list_head list;
	/* TODO: DECLARE NECESSARY MEMBER VARIABLES */
	char *address;
	int order;
	int index;
} page_t;

/**************************************************************************
 * Global Variables
 **************************************************************************/
/* free lists*/
struct list_head free_area[MAX_ORDER+1];

/* memory area */
char g_memory[1<<MAX_ORDER];

/* page structures */
page_t g_pages[(1<<MAX_ORDER)/PAGE_SIZE];

/**************************************************************************
 * Public Function Prototypes
 **************************************************************************/

/**************************************************************************
 * Local Functions
 **************************************************************************/

/**
 * Initialize the buddy system
 */

int find_order(int size){
	int order = MIN_ORDER;

	while(ORDER_SIZE(order) < size && order <= MAX_ORDER){
		order = order + 1;
	}
	return order;
}

void buddy_init()
{
	int i;
	int n_pages = (ORDER_SIZE(MAX_ORDER)) / PAGE_SIZE;
	for (i = 0; i < n_pages; i++) {
		/* TODO: INITIALIZE PAGE STRUCTURES */
		INIT_LIST_HEAD(&g_pages[i].list);
		g_pages[i].address = PAGE_TO_ADDR(i);
		g_pages[i].order = -1;
		g_pages[i].index = i;
	}


	/* initialize freelist */
	for (i = MIN_ORDER; i <= MAX_ORDER; i++) {
		INIT_LIST_HEAD(&free_area[i]);
	}
	g_pages[0].order = MAX_ORDER;
	/* add the entire memory as a freeblock */
	list_add(&g_pages[0].list, &free_area[MAX_ORDER]);
}

/**
 * Allocate a memory block.
 *
 * On a memory request, the allocator returns the head of a free-list of the
 * matching size (i.e., smallest block that satisfies the request). If the
 * free-list of the matching block size is empty, then a larger block size will
 * be selected. The selected (large) block is then splitted into two smaller
 * blocks. Among the two blocks, left block will be used for allocation or be
 * further splitted while the right block will be added to the appropriate
 * free-list.
 *
 * @param size size in bytes
 * @return memory block address
 */
void *buddy_alloc(int size)
{
	/* TODO: IMPLEMENT THIS FUNCTION */
	if(size < 1){
		return NULL;
	}

	int alloc_order = find_order(size);

	for(int i = alloc_order; i <= MAX_ORDER; i++){
		if(!list_empty(&free_area[i])){
			page_t* temp_left;
			if(i == alloc_order){
				temp_left = list_entry(free_area[i].next, page_t, list);
				list_del(&(temp_left->list));
			}

			else{
				temp_left = &( g_pages[ADDR_TO_PAGE(buddy_alloc((ORDER_SIZE((1 + alloc_order)))))] );
				int right_ix = temp_left->index + ((ORDER_SIZE(alloc_order))/PAGE_SIZE);
				list_add(&(g_pages[right_ix].list), &free_area[alloc_order]);
			}
			temp_left->order = alloc_order;
			return PAGE_TO_ADDR(temp_left->index);
		}
	}

	return NULL;
}

/**
 * Free an allocated memory block.
 *
 * Whenever a block is freed, the allocator checks its buddy. If the buddy is
 * free as well, then the two buddies are combined to form a bigger block. This
 * process continues until one of the buddies is not free.
 *
 * @param addr memory block address to be freed
 */
void buddy_free(void *addr)
{
	/* TODO: IMPLEMENT THIS FUNCTION */
	int index = ADDR_TO_PAGE(addr);
	int order = g_pages[index].order;
	struct list_head *temp_list;
	page_t* temp_page = NULL;


	do {
		temp_page = NULL;
		list_for_each(temp_list, &free_area[order]){
			temp_page = list_entry(temp_list, page_t, list);
			if(temp_page == NULL || temp_page->address == BUDDY_ADDR(addr, order)){
				break;
			}
		}

		if(temp_page == NULL || temp_page->address != BUDDY_ADDR(addr, order)){
			g_pages[order].order = -1;
			list_add(&g_pages[index].list, &free_area[order]);
			return;
		}

		if((char*)addr > temp_page->address){
			addr = temp_page->address;
			index = ADDR_TO_PAGE(addr);
		}

		list_del(&temp_page->list);



		order++;
	} while(order <= MAX_ORDER);


}

/**
 * Print the buddy system status---order oriented
 *
 * print free pages in each order.
 */
void buddy_dump()
{
	int o;
	for (o = MIN_ORDER; o <= MAX_ORDER; o++) {
		struct list_head *pos;
		int cnt = 0;
		list_for_each(pos, &free_area[o]) {
			cnt++;
		}
		printf("%d:%dK ", cnt, (1<<o)/1024);
	}
	printf("\n");
}
