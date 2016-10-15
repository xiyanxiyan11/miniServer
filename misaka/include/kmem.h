#ifndef KMEM_H_
#define KMEM_H_

/**
 * @file kmem.h
 * @brief kmem head file 
 * @author mhw
 * @date 072123412013
 * @notic: OBJ_DEBUG AND NO_OUTPUT_OBJ
 */
struct kmem_cache;
struct kmem;

/**
 * Used to debug kmem_cache 	
 * @param[in] ptr pointer to the kmem_cache
 * @param[in] mode mode for check 0(no check for obj) 1(check obj chain link) 2(check all for obj)
 *                                      
 */
void debug_kmem_cache( struct kmem_cache *ptr, int mode);

/**
 * Used to debug kmem
 * @param[in] ptr pointer to the kmem
 * 
 */
void debug_kmem( struct kmem *ptr);

/**
 * @param[in|out] ptr pointer to the mem want to be managed by kmem
 * @param[in] size size of the mem 
 * @param[in] align align block as
 * @return kmem init kmem success
 * @return NULL init kmem fail
 */
struct kmem *init_kmem(void *ptr, int size, int align);

/**
 * @param[in|out] kmemp kmem manage the mem  
 * @param[in] name the name of the kmem_cache want to get	 	
 * @param[in] mem_size the size of the obj in kmem_cache
 * @param[in] total nums of  objs store in kmem_cache
 * return kmem_cache point to the kmem_cache create 
 * return NULL create kmem_cache fail
 */
struct kmem_cache *kmem_cache_create(struct kmem *kmemp, const char *name, int mem_size, int tots);

/**
 * @param[in|out] kmem_cachep pointer to the kmem_cache alloc obj from
 * @return pointer to obj  after successed
 * @return pointer to null after failed
 */
void *kmem_cache_alloc(struct kmem_cache *kmem_cachep);

/**
 * @param[in|out] kmem_cachep pointer to the kmem_cachep to collect the obj
 * @param[in|out] pointer to the buf to be collected;
 */
void kmem_cache_free(struct kmem_cache *kmem_cachep, void *bufp);

/**
 * @param[in] kmem_cachep kmem_cache the buf from 
 * @param[in] buf from user
 *
 */
int kmem_cache_checkoverflow(struct kmem_cache *kmem_cachep, void *bufp);

#endif
