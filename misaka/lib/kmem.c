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
#include "list.h"

#ifdef DEBUG
#include <string.h>
#endif

#define NAME 40
#define OF   ((unsigned char)(0xff))		        /*OVERFLOW FLAG*/		
#define OFS  (sizeof(unsigned char))                    /*SIZE OF OVERFLOW FLAG*/

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
		printf("obj(x%p):", ptr); 
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
 */
struct kmem_cache *init_kmem_cache(struct kmem_cache *kmem_cachep, const char *name, int mem_size, int tots, int align);

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
	objp->buf = index;								                /*set buf ptr*/
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
