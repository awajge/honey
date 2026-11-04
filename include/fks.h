#ifndef FKS_H
#define FKS_H

#include <stdint.h>
#include <stdbool.h>

#define FKS_NULL_ENTITY 0ull

typedef uint64_t fks_entity_t;
typedef uint32_t fks_entity_id_t;
typedef struct fks_world_t fks_world_t;

/**
 * returns new fks_world_t world on succesfull calloc(), if it fails, returns NULL.
 */
fks_world_t* fks_world_init(void);

/**
 * frees an fks_world_t world. make sure to run this after you're done.
 */
void fks_world_fini(fks_world_t* wrld);

/**
 * returns a free entity id. if entities have been removed and not reassigned to a 
 * new entity, this function grabs the next free id from an intrusive free list in the
 * entity state list. otherwise, it employs a much faster high watermark system that 
 * simply gets the next numerical id. if entity limit has been reached, this function 
 * will return the null entity.
 */
fks_entity_t fks_entity_create(fks_world_t* wrld);

/**
 * destroys the given entity. this id will be next up for entity creation unless another
 * entity is destroyed. This function returns false if the entity is the null entity, not
 * been allocated yet, or a stale id. Otherwise, it returns true.
 */
bool fks_entity_destroy(fks_world_t* wrld, fks_entity_t ntt);

/**
 * returns entity count of given fks_world_t world. returns UINT32_MAX (+1 more than the
 * entity max) on a NULL wrld.
 */
uint32_t fks_entity_count(const fks_world_t* wrld);

/**
 * returns whether an id is "alive" or not - in other words, if it has been destroyed
 * or not. This function returns true if the entity is alive and false if the entity has
 * been destroyed or is the null entity.
 */
bool fks_entity_vital(const fks_world_t* wrld, fks_entity_t ntt);

#endif