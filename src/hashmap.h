#ifndef HASHMAP_H
#define HASHMAP_H

// stdint pulls in exact width integers like uint64_t
#include <stdint.h>
// stdbool includes...bool
#include <stdbool.h>
// For size_t
#include <stddef.h>

// Opaque pointer to hide implementation details
typedef struct HashMap HashMap;

typedef void (*ValueCleanupFn)(void*);

// Create/destroy functions
HashMap* hm_create(size_t initial_capacity, ValueCleanupFn value_cleanup);
void hm_destroy(HashMap* map);

// Core operations
bool hm_put(HashMap* map, const char* key, void* value);
void* hm_get(HashMap* map, const char* key);
bool hm_remove(HashMap* map, const char* key);

// Utility functions
size_t hm_size(HashMap* map);
bool hm_is_empty(HashMap* map);

#endif // HASHMAP_H
