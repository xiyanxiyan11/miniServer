/**
 * @file kmem.c
 * @brief kmem head file 
 * @author mhw
 * @date 072123412013
 * @notic: OBJ_DEBUG AND NO_OUTPUT_OBJ
 */


#include <stdio.h>
#include <string.h>
#include "kmem.h"

#ifdef DEBUG
#include <string.h>
#endif

#define NAME 40
#define OF   ((unsigned char)(0xff))		/*OVERFLOW FLAG*/		
#define OFS  (sizeof(unsigned char))        /* SIZE OF OVERFLOW FLAG*/

/***
 * set size to align
 * @size origin size
 * @align align as 
 */
#define set_align(size, align) (((size) + (align - 1)) & ~(align - 1))


#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:    the pointer to the member.
 * @type:   the type of the container struct this is embedded in.
 * @member: the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({          \
	const typeof(((type *)0)->member)*__mptr = (ptr);    \
		     (type *)((char *)__mptr - offsetof(type, member)); })

/*
 * Simple doubly linked list implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */

struct list_head {
	struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)

#define INIT_LIST_HEAD(ptr) do { \
	(ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

/*
 * Insert a new entry between two known consecutive entries. 
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_add(struct list_head * new,
	struct list_head * prev,
	struct list_head * next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

/**
 * list_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void list_add(struct list_head *new, struct list_head *head)
{
	__list_add(new, head, head->next);
}

/**
 * list_add_tail - add a new entry
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
	__list_add(new, head->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_del(struct list_head * prev,
				  struct list_head * next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty on entry does not return true after this, the entry is in an undefined state.
 */
static inline void list_del(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
}

/**
 * list_del_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
static inline void list_del_init(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
	INIT_LIST_HEAD(entry); 
}

/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */
static inline int list_empty(struct list_head *head)
{
	return head->next == head;
}

/**
 * list_for_each	-	iterate over a list
 * @pos:	the &struct list_head to use as a loop cursor.
 * @head:	the head for your list.
 *
 * This variant differs from list_for_each() in that it's the
 * simplest possible list iteration code, no prefetching is done.
 * Use this for code that knows the list to be very short (empty
 * or 1 entry) most of the time.
 */
#define list_for_each(pos, head) \
	for(pos = (head)->next; pos != (head); pos = pos->next)

struct obj{
	struct list_head link;				/**< link obj as a list*/
	void *buf;					/**< point to the mem*/
};


struct kmem_cache{
	char name[NAME];
	int  mem_size;					/**< obj size information*/
	int tots;					/**< tot nums  of obj in kmem_cache*/
	int used;					/**< used nums of obj in kmem_cache*/
	int free;					/**< free nums of obj in kmem_cache*/
	struct list_head free_objs;			/**< head of the  free objs */ 
};

struct kmem{
	int tots;					/**< total nums of kmem_caches in kmem*/
	int size;					/**< the total size that kmem manage */
	int used_size; 					/**< the used  size that kmem manage */
	struct kmem_cache *free;			/**< point to the free mem ready for kmem_cache*/
	int align;					/**< align as*/
	int lock;					/**< as lock */
};

/**
 * set sem up 
 *
 */
static inline void sem_up(int sem)
{
	++sem;
}

/**
 * set sem down without wait
 *
 */
static inline int sem_down_now(int sem){ 	
	if(0 == sem){
	    return -1;
	}
	--sem;
	return 0;
}

/**
 *set sem down with  wait
 *
 */
static inline void sem_down(int sem){
	while(0 == sem);
		--sem;	
}


static inline void debug_obj(struct obj *ptr, int mem_size) 
{						
		printf("obj(x%x):", ptr); 
		check_overflow(ptr->buf, mem_size) ? printf("overflow") : printf("safe");
		printf("\n");
}


/**
 * @param[in|out] objp pointer to the obj init 
 * @param[in] mem_size size of the buf
 * return ptr to obj inited
 */
struct obj *init_obj(struct obj *objp, int mem_size);


/**
 * @param[in|out] kmem_cachep point to the kmem_cachep init
 * @param[in] name of the kmem_cache
 * @param[in] mem_size the buf size which obj control
 * @param[in] tots total nums of obj manage in this kmem_cache
 * @param[in] align align size as
 * return ptr to kmem_cache inited
 */struct kmem_cache *init_kmem_cache(struct kmem_cache *kmem_cachep, const char *name, int mem_size, int tots, int align);

/**
 * @param[in|out] kmem_cachep pointer to the kmem_cache alloc obj from
 * @return pointer to obj  after successed
 * @return pointer to null after failed
 * @notice basic function for kmem_cache_alloc or kmem_cache_alloc_try
 */
void *_kmem_cache_alloc(struct kmem_cache *kmem_cachep);


/**
 * @param[in|out] kmem_cachep pointer to the kmem_cachep to collect the obj
 * @param[in|out] pointer to the buf to be collected;
 */
void kmem_cache_free(struct kmem_cache *kmem_cachep, void *buf);


/* 
 *@param[in] buf ptr point to buf to be checked
 *@param[in] mem_size size of the mem
 *retun 0 safe, -1 overflow
*/
int check_overflow(void *bufp, int mem_size);


int check_overflow(void *bufp, int mem_size){
	char *index = 0;

	index = (char *)bufp;
#ifdef DEBUG	
	printf("in check_overflow: buf = %x, mem_size = %d\n", bufp, mem_size);
#endif

#ifdef DEBUG	
	printf("index  = %x\n", index);
#endif
	if( (index) && ( (*((unsigned char *)(index - 1))) == OF)&&( *( (unsigned char *)( index + mem_size) ) == OF)){
			return 0;
	}else{
			return -1;
	}

}
int kmem_cache_checkoverflow(struct kmem_cache *kmem_cachep, void *bufp)
{
	return check_overflow(bufp, kmem_cachep->mem_size);
}

void debug_kmem_cache( struct kmem_cache *ptr, int mode)
{
	struct list_head *list_headt = 0; 
	printf("kmem_cache:  %s\n", (ptr)->name);     
	printf("mem_size  =  %d\n", (ptr)->mem_size); 
	printf("tot objs  =  %d\n", (ptr)->tots); 
	printf("used objs =  %d\n", (ptr)->used); 
	printf("free objs =  %d\n", (ptr)->free);
	if(mode){				  		/* debug obj chain'status when mode = 1 || mode == 2*/	
		printf("free obj chain checked:\n");
		list_for_each(list_headt, &(ptr->free_objs)){
			if(2 == mode){				/*only check chain link without output when mode == 1*/
				debug_obj(container_of(list_headt, struct obj, link), ptr->mem_size);
			}
		}
	}
}


void debug_kmem( struct kmem *ptr)
{	
	sem_down(ptr->lock);

	printf("kmem:\n");
	printf("total kmem_caches   =  %d\n", (ptr)->tots);     
	printf("total size of mem   =  %d\n", (ptr)->size); 
	printf("total used_size of mem  =  %d\n", (ptr)->used_size); 

	sem_up(ptr->lock);
}

struct kmem *init_kmem(void *ptr, int size, int align)
{
	struct kmem *kmemt = 0;
	int real_size = set_align( sizeof(struct kmem), align);
	if(size <= real_size)				/* space too small*/	
		return 0;
	kmemt = (struct kmem*)ptr;		
	kmemt->tots        = 0;
	kmemt->align       = align; 
	kmemt->size        = size;
	kmemt->used_size   = real_size;
	kmemt->free        = (struct kmem_cache*)((char *)kmemt + real_size);

	kmemt->lock        = 1;   /*set lock*/
	return kmemt;
}

struct obj *init_obj(struct obj *objp, int mem_size)
{
	char *index = 0;
	INIT_LIST_HEAD(&(objp->link));
	index = ( (char*)(objp + 1));							
	*((unsigned char *)index) = OF;									/*set start eof*/
	index += 1;
	objp->buf = index;								/*set buf ptr*/
	index += mem_size;								
	*((unsigned char *)index) = OF;									/*set end eof*/
#ifdef DEBUG	
	memset(objp->buf, 0, 5);		//test writeable
	printf("return obj from init_obj\n");
	debug_obj(objp,mem_size); 
#endif
	return objp;
}

struct kmem_cache *init_kmem_cache(struct kmem_cache *kmem_cachep, const char *name, int mem_size, int tots, int align)
{
	/*set basic information for kmem_cache*/
	struct obj *index = 0;
	int real_mem_size, real_size;
	real_size = set_align(sizeof(struct kmem_cache), align);
	real_mem_size = set_align(sizeof(struct obj) + mem_size + 2*OFS, align);
	strncpy(kmem_cachep->name, name, NAME - 1);	
	kmem_cachep->mem_size = mem_size;
	kmem_cachep->tots     = tots;
	kmem_cachep->free     = tots;
	kmem_cachep->used     = 0;
	INIT_LIST_HEAD(&(kmem_cachep->free_objs));
	/*create free objs chain*/
	index = (struct obj *)( (char *)kmem_cachep + real_size);	
	while(tots--){
		init_obj(index, mem_size);
		list_add(&(index->link), &(kmem_cachep->free_objs));
		index = (struct obj *)((char *)index + real_mem_size);
	}
#ifdef DEBUG	
	printf("return kmem_cachep from kmem_cachep\n");
	debug_kmem_cache(kmem_cachep); 
#endif
	return kmem_cachep;
}


struct kmem_cache *kmem_cache_create(struct kmem *kmemp, const char *name, int mem_size, int tots)
{
	struct kmem_cache * kmem_cachet = 0;
	int real_size;
	if(sem_down_now(kmemp->lock)){				/*try lock */
		return 0;
	}

	real_size = set_align(sizeof(struct kmem_cache), kmemp->align) + \
		set_align((sizeof(struct obj) + mem_size + 2*OFS) , kmemp->align) * (tots);	/*every mem as obj + mem_size + OFS*/
#ifdef DEBUG
printf("need size = %u , kmem_cache_size = %u\n", real_size, sizeof(struct kmem_cache));
#endif
	if( (kmemp->size - kmemp->used_size) <= real_size){	/*space too small*/
		sem_up(kmemp->lock);					/*unlock */
		return NULL;
	}
	kmem_cachet = kmemp->free;				/*point to the first free kmem_cache*/	
	memset(kmem_cachet, 0, real_size);			/*notice! init mem here*/
	/*set new kmem_cache*/
	init_kmem_cache(kmem_cachet, name, mem_size, tots, kmemp->align);
	/*reset kmem*/
	kmemp->used_size += real_size;
	kmemp->free = (struct kmem_cache *)((char *)(kmemp->free) + real_size);
	++kmemp->tots;

	sem_up(kmemp->lock);					/*unlock */
	return kmem_cachet;
}



void *kmem_cache_alloc(struct kmem_cache *kmem_cachep)
{
	struct list_head *list_headt = 0;
	struct obj *objt = 0;
	if(0 == kmem_cachep->free){
		return 0;
	}
	list_headt = (kmem_cachep->free_objs).next;		/*alloc first obj in the chain*/	
	list_del_init(list_headt);				/*del from free list*/
	--kmem_cachep->free;
	++kmem_cachep->used;
	objt = container_of(list_headt, struct obj, link);
#ifdef DEBUG
fprintf(stderr, "obj(x%x):obj get from kmem_cache_alloc\n", objt);
#endif
	return objt->buf;
}


void kmem_cache_free(struct kmem_cache *kmem_cachep, void *bufp)
{
	
	struct obj *objt = (struct obj *)((char *)bufp - sizeof(struct obj) - OFS);
	list_add(&(objt->link), &(kmem_cachep->free_objs));	/*add into free list*/
	--kmem_cachep->used;
	++kmem_cachep->free;
}

