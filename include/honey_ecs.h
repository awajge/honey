#ifndef HONEY_ECS_H
#define HONEY_ECS_H

/**
 * @file honey.h
 * @author Akshat Wajge
 * @brief A simplistic approach to an ECS
 * @version 1.0
 * @date 2026-11-09 
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Null entity represents the lack of an entity */
#define HNY_NULL_ENTITY ((hny_entity_t) 0u)

/** Null component represents the lack of a component */
#define HNY_NULL_COMPONENT ((hny_component_t) 0u)

/** 64 bit entity identifier (32 generation bits + 32 id bits) */
typedef uint64_t hny_entity_t;

/** 16 bit component id */
typedef uint16_t hny_component_t;

/** ECS world instance */
typedef struct hny_world_t hny_world_t;

/**
 * @brief Initializes an ECS world instance
 * 
 * @return hny_world_t* Pointer to newly allocated world
 * @retval NULL Allocation failure
 */
hny_world_t* hny_world_init(void);

/**
 * @brief Frees an ECS world instance
 * 
 * @param wrld World to free
 */
void hny_world_fini(hny_world_t* wrld);

/**
 * @brief Returns world's entity count
 * 
 * @param wrld ECS world
 *
 * @return size_t [0, UINT32_MAX] entity count
 * @retval SIZE_MAX if wrld is NULL
 */
size_t hny_world_entity_count(const hny_world_t* wrld);

/**
 * @brief Creates a new entity
 * @details This function recycles the id of the most recently destroyed entity
 * while incrementing its generation bits by 1. If there are no ids left to recycle,
 * this function returns a new id with a generation id of 0. When generation bits reach
 * UINT32_MAX+1, it wraps around back to 0.  
 * 
 * @param wrld ECS world
 * @return hny_entity_t Created entity
 * @retval HNY_NULL_ENTITY wrld is NULL or entity limit has been reached
 */
hny_entity_t hny_entity_create(hny_world_t* wrld);

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
 * return value is functionally equivalent to @ref hny_entity_alive 
 */
bool hny_entity_destroy(hny_world_t* wrld, hny_entity_t ntt);

/**
 * @brief Checks if an entity is "alive"
 * @details An entity is alive if it has been returned by @ref hny_entity_create and not
 * destroyed by @ref hny_entity_destroy since. Entities with the same id have differing
 * generation bits which makes id recycling effective, with only an extraordinarily rare
 * chance of a false positive from this function.
 * 
 * @param wrld ECS world 
 * @param ntt Entity to check
 * @return bool True if world and entity are valid, and entity is alive in the world
 */
bool hny_entity_alive(const hny_world_t* wrld, hny_entity_t ntt);

/**
 * @brief Registers a new component 
 * @details This function simply increments a counter to return component ids. size must
 * not be 0, this function can fail on a couple different allocations.
 * 
 * @param wrld ECS world
 * @param size Size of component data payload
 * @return hny_component_t Registered component
 * @retval HNY_NULL_COMPONENT wrld is NULL or component limit has been reached
 */
hny_component_t hny_component_register(hny_world_t* wrld, uint32_t size);

/**
 * @brief Enables component creation
 * @details This function enables entities to have this component. This functionality
 * is on by default.
 * 
 * @param wrld ECS world
 * @param cmp Component to enable
 */
void hny_component_enable(hny_world_t* wrld, hny_component_t cmp);

/**
 * @brief Disables component creation
 * @details This function disables entities from having this component.
 * 
 * @param wrld ECS world 
 * @param cmp Component to disable
 */
void hny_component_disable(hny_world_t* wrld, hny_component_t cmp);

/**
 * @brief Get a list of entities that have a component
 * @details Gives a hny_entity_t* list of entities that have a component. The size of the list
 * is returned in the function parameter rsize. Should rsize be NULL this function will effectively
 * do nothing. The return pointer can be invalidated after calling @ref hny_entity_add_component,
 * @ref hny_entity_remove_component, and @ref hny_entity_destroy.
 * 
 * @param wrld ECS world
 * @param cmp Given component
 * @param rsize Entity list size
 * @return const hny_entity_t* List of entities
 * @retval NULL if wrld, cmp, or rsize are invalid/NULL
 */
const hny_entity_t* hny_component_get_entities(const hny_world_t* wrld, hny_component_t cmp, size_t* rsize);

/**
 * @brief Gets a list of entities that have a specific set of components
 * @details If the component set or return size ptr is NULL, number of components is below 1, or any
 * of the components is invalid in the given world, this function will return NULL, and rsize is not
 * guaranteed to have any value, and will often be unchanged from when the function was called. If this
 * is not the case then the function runs as intended and the returned pointer is guaranteed to have
 * rsize number of entities that match the given component set. Should there be no matching entities
 * rsize will be 0 and there will be a non-NULL return pointer, but with undefined data should you
 * dereference it. Unlike @ref hny_component_get_entities this function currently leaves the onus of
 * freeing the returned buffer on the caller.
 * 
 * @param wrld ECS world
 * @param cmps Given component set
 * @param ncmps Number of components in set
 * @param rsize Entity list size
 * @return hny_entity_t* List of entities
 * @retval NULL if error in any of params 
 */
hny_entity_t* hny_components_get_entities(const hny_world_t* wrld, const hny_component_t* cmps, size_t ncmps, size_t* rsize);

/**
 * @brief Adds a component to an entity
 * @details Will do nothing if the component is disabled, or the entity already contains
 * the component. Responsibility of possible over-reading and making sure data is proper
 * size is given to the caller.
 * 
 * @param wrld ECS world 
 * @param ntt Entity to update
 * @param cmp Component to add
 * @param data Component data
 * @return bool true if data is added and entity is updated, false if not
 */
bool hny_entity_add_component(hny_world_t* wrld, hny_entity_t ntt, hny_component_t cmp, const void* data);

/**
 * @brief Removes a component from an entity
 * @details Will remove data whether or not the component is enabled/disabled. Does not log even
 * if the given entity does not contain the given component.
 * 
 * @param wrld ECS world 
 * @param ntt Entity to update
 * @param cmp Component to remove
 * @return bool true if component was removed, false if not
 */
bool hny_entity_remove_component(hny_world_t* wrld, hny_entity_t ntt, hny_component_t cmp);

/**
 * @brief Checks if an entity has a component
 * 
 * @param wrld ECS world
 * @param ntt Entity to check
 * @param cmp Component to check
 * @return bool Whether the entity has the component or not, or if inputs aren't valid
 */
bool hny_entity_has_component(const hny_world_t* wrld, hny_entity_t ntt, hny_component_t cmp);

/**
 * @brief Gets component data from an entity
 * @details Does not return size, as it should be known by the caller. Calling memory past the size
 * of a single component is undefined. Pointers from this function can be invalidated after calling
 * @ref hny_entity_add_component, @ref hny_entity_remove_component, and @ref hny_entity_destroy.
 * 
 * @param wrld ECS world
 * @param ntt Entity to retrieve data from
 * @param cmp Component data type to retrieve
 * @return void* Data if present, NULL if invalid
 */
void* hny_entity_get_component(const hny_world_t* wrld, hny_entity_t ntt, hny_component_t cmp);

#ifdef __cplusplus
}
#endif

#endif