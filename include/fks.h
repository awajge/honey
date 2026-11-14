#ifndef FKS_H
#define FKS_H

/**
 * @file fks.h
 * @author Akshat Wajge
 * @brief A simplistic approach to an ECS
 * @version 1.0
 * @date 2026-11-09 
 */

#include <stdint.h>
#include <stdbool.h>

/** Null entity represents the lack of an entity */
#define FKS_NULL_ENTITY ((fks_entity_t) 0u)

/** Null component represents the lack of a component */
#define FKS_NULL_COMPONENT ((fks_component_t) 0u)

/** 64 bit entity identifier (32 generation bits + 32 id bits) */
typedef uint64_t fks_entity_t;

/** 16 bit component id */
typedef uint16_t fks_component_t;

/** ECS world instance */
typedef struct fks_world_t fks_world_t;

/**
 * @brief Initializes an ECS world instance
 * 
 * @return fks_world_t* Pointer to newly allocated world
 * @retval NULL Allocation failure
 */
fks_world_t* fks_world_init(void);

/**
 * @brief Frees an ECS world instance
 * 
 * @param wrld World to free
 */
void fks_world_fini(fks_world_t* wrld);

/**
 * @brief Returns world's entity count
 * 
 * @param wrld ECS world
 *
 * @return uint32_t [0, UINT32_MAX-1] entity count
 * @retval UINT32_MAX wrld is NULL
 */
uint32_t fks_world_entity_count(const fks_world_t* wrld);

/**
 * @brief Creates a new entity
 * @details This function recycles the id of the most recently destroyed entity from
 * while incrementing its generation bits by 1. If there are no ids left to recycle,
 * this function returns a new id with a generation id of 0. When generation bits reach
 * UINT32_MAX+1, it wraps around back to 0.  
 * 
 * @param wrld ECS world
 * @return fks_entity_t Created entity
 * @retval FKS_NULL_ENTITY wrld is NULL or entity limit has been reached
 */
fks_entity_t fks_entity_create(fks_world_t* wrld);

/**
 * @brief Destroys an entity
 * @details This function destroys all data stored in a world pertaining to a given entity,
 * namely all component data. After destroying an entity, it will not be alive and creating
 * an entity directly after will reuse its id, with different generation bits. Should the
 * given entity/world be invalid, or the entity is not alive, this function does nothing.
 * 
 * @param wrld ECS world
 * @param ntt Entity to destroy
 * @return bool True if world and entity are valid, and entity is alive in the world - The
 * return value is functionally equivalent to @ref fks_entity_alive 
 */
bool fks_entity_destroy(fks_world_t* wrld, fks_entity_t ntt);

/**
 * @brief Checks if an entity is "alive"
 * @details An entity is alive if it has been returned by @ref fks_entity_create and not
 * destroyed by @ref fks_entity_destroy since. Entitys with the same id have differing
 * generation bits which makes id recycling effective, with only an extraordinarily rare
 * chance of a false positive from this function.
 * 
 * @param wrld ECS world 
 * @param ntt Entity to check
 * @return bool True if world and entity are valid, and entity is alive in the world
 */
bool fks_entity_alive(const fks_world_t* wrld, fks_entity_t ntt);

/**
 * @brief Registers a new component 
 * @details This function simply increments a counter to return component ids.
 * 
 * @param wrld ECS world
 * @param size Size of component data payload
 * @return fks_component_t Registered component
 * @retval NULL_COMPONENT wrld is NULL or component limit has been reached
 */
fks_component_t fks_component_register(fks_world_t* wrld, uint32_t size);

/**
 * @brief Enables component creation
 * @details This function enables entities to have this component. This functionality
 * is on by default.
 * 
 * @param wrld ECS world
 * @param cmp Component to enable
 */
void fks_component_enable(fks_world_t* wrld, fks_component_t cmp);

/**
 * @brief Disables component creation
 * @details This function disables entities from have this component.
 * 
 * @param wrld ECS world 
 * @param cmp Component to disable
 */
void fks_component_disable(fks_world_t* wrld, fks_component_t cmp);

/**
 * @brief Get a list of entities that have a component
 * @details Gives a fks_entity_t* list of entities that have a component. The size of the list
 * is returned in the function parameter rsize. Should rsize be NULL this function will effectively
 * do nothing.
 * 
 * @param wrld ECS world
 * @param cmp Given component
 * @param rsize Entity list size
 * @return const fks_entity_t* List of entities
 * @retval NULL if wrld, cmp, or rsize are invalid/NULL
 */
const fks_entity_t* fks_component_get_entities(const fks_world_t* wrld, fks_component_t cmp, uint32_t* rsize);

/**
 * @brief Adds a component to an entity
 * @details Will do nothing if the component is disabled, or the entity already contains
 * the component. Data is automatically truncated to the component size. Responsibility
 * of making sure data is proper size is given to the caller.
 * 
 * @param wrld ECS world 
 * @param ntt Entity to update
 * @param cmp Component to add
 * @param data Component data
 * @return bool true if data is added and entity is updated, false if not
 */
bool fks_entity_add_component(fks_world_t* wrld, fks_entity_t ntt, fks_component_t cmp, const void* data);

/**
 * @brief Removes a component from an entity
 * @details Will remove data whether or not the component is enabled/disabled. Also will not log
 * whether or not the entity is alive or not.
 * 
 * @param wrld ECS world 
 * @param ntt Entity to update
 * @param cmp Component to remove
 * @return bool true if component was removed, false if not
 */
bool fks_entity_remove_component(fks_world_t* wrld, fks_entity_t ntt, fks_component_t cmp);

/**
 * @brief Checks if an entity has a component
 * 
 * @param wrld ECS world
 * @param ntt Entity to check
 * @param cmp Component to check
 * @return bool Whether the entity has the component or not, or if inputs aren't valid
 */
bool fks_entity_has_component(const fks_world_t* wrld, fks_entity_t ntt, fks_component_t cmp);

/**
 * @brief Gets component data from an entity
 * @details Does not return size, as it should be known by the caller. Calling memory past the size
 * of a single component is undefined.
 * 
 * @param wrld ECS world
 * @param ntt Entity to retrieve data from
 * @param cmp Component data type to retrieve
 * @return void* Data if present, NULL if invalid
 */
void* fks_entity_get_component(const fks_world_t* wrld, fks_entity_t ntt, fks_component_t cmp);

#endif