// includes stdint and stdbool
#include "hashmap.h" 
// For malloc / calloc / free
#include <stdlib.h>
// For strcmp / strdup
#include <string.h>

// FNV-1a hash function constants
#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL

// Initial load factor and growth settings
#define INITIAL_CAPACITY 16
#define LOAD_FACTOR_THRESHOLD 0.75

typedef struct Entry {
    char* key;
    void* value;
    struct Entry* next;
} Entry;


// typedef in header
struct HashMap {
    // A pointer to a pointer of type Entry,
    // in this case this is an array of pointers of Entry.
	// Each index in the array is a "bucket" and the
    // Entries are a linked list.
    Entry** buckets;
    size_t capacity;
    size_t size;
    // A fn to call when we clean up values.
    ValueCleanupFn value_cleanup;
};

// FNV-1a hash function - proven fast for string keys
static uint64_t hash_key(const char* key) {
    uint64_t hash = FNV_OFFSET;
    for (const char* p = key; *p; p++) {
        hash ^= (uint64_t)(unsigned char)(*p);
        hash *= FNV_PRIME;
    }
    return hash;
}

size_t round_to_power_of_2(size_t initial_capacity) {
    // Ensure power of two capacity to ensure fast indexing
    if (initial_capacity < INITIAL_CAPACITY) {
        return INITIAL_CAPACITY;
    }

    initial_capacity--;

    // Calculate the number of shifts needed based on the size of size_t
    #if SIZE_MAX == 0xffffffffffffffffULL  // 64-bit
        initial_capacity |= initial_capacity >> 32;
    #endif
    initial_capacity |= initial_capacity >> 16;
    initial_capacity |= initial_capacity >> 8;
    initial_capacity |= initial_capacity >> 4;
    initial_capacity |= initial_capacity >> 2;
    initial_capacity |= initial_capacity >> 1;

    // Check for overflow before incrementing
    if (initial_capacity == SIZE_MAX) {
        // We can't round up - we should handle this error
        // Maybe return 0 to indicate error, or
        // use errno, or return the largest power of 2
        return SIZE_MAX >> 1;  // Return largest power of 2 that fits
    }
    
    return initial_capacity + 1;

}

// Create a `HashMap`, returns NULL if a `HashMap` could not be created.
HashMap* hm_create(size_t initial_capacity, ValueCleanupFn value_cleanup) {
    // Round up to the next power of 2
    initial_capacity = round_to_power_of_2(initial_capacity);
    HashMap* map = malloc(sizeof(HashMap));
    if (!map) return NULL;

    map->value_cleanup = value_cleanup;

	// calloc initializes the memory when it creates it, and it does the 
	// multiplication of `sizeof(Entry*) * capapcity` and checks for overflows.
    map->buckets = calloc(initial_capacity, sizeof(Entry*));
    if (!map->buckets) {
        free(map);
        return NULL;
    }

    map->capacity = initial_capacity;
    map->size = 0;
    return map;
}

static void resize_if_needed(HashMap* map) {
    if ((double)map->size / map->capacity > LOAD_FACTOR_THRESHOLD) {
        // No resize needed
        return;
    }

    size_t new_capacity = map->capacity * 2;
    Entry** new_buckets = calloc(new_capacity, sizeof(Entry*));
    if (!new_buckets) return; // Failed alloc

    // Rehash all entries
    for (size_t i = 0; i < map->capacity; i++) {
        // since we calloc'd to zero this out, buckets with no pointer are NULL
        Entry* entry = map->buckets[i];
        while (entry) { // Traverse the linked list
            Entry* next = entry->next; 
            size_t new_index = hash_key(entry->key) & (new_capacity - 1);
            // Set the next for this entry to whatever is currently in the 
            // new_buckets list of pointers, then set the new_buckets pointer
            // to this entry, extending the linked list.
            entry->next = new_buckets[new_index];
            new_buckets[new_index] = entry;
            entry=next;
        }
    }
    free(map->buckets);
    map->buckets = new_buckets;
    map->capacity = new_capacity;
}

// Returns true if the key was set, false if allocations failed and it was not set.
// If the key already exists, this will overwrite its value.
bool hm_put(HashMap* map, const char* key, void* value) {
    resize_if_needed(map);
    size_t index = hash_key(key) & (map->capacity - 1);

    // Check for an existing key
    for (Entry* entry = map->buckets[index]; entry; entry = entry->next) {
        if (strcmp(entry->key, key) == 0) {
            if (map->value_cleanup) map->value_cleanup(entry->value);
            entry->value = value;
            return true;
        }
    }
    
    // Create a new entry
    Entry* new_entry = malloc(sizeof(Entry));
    if (!new_entry) return false;
    
    new_entry->key = strdup(key);
    if (!new_entry->key) {
        free(new_entry);
        return false;
    }

    new_entry->value = value;
    new_entry->next = map->buckets[index];
    map->buckets[index] = new_entry;
    map->size++;
    return true;
}

void* hm_get(HashMap* map, const char* key) {
    size_t index = hash_key(key) & (map->capacity - 1);
    for (Entry* entry = map->buckets[index]; entry; entry = entry->next) {
        if (strcmp(entry->key, key) == 0) {
            return entry->value;
        }
    }
    return NULL;
}


// Returns true if the key was in the map and was deleted.
bool hm_remove(HashMap* map, const char* key) {
    size_t index = hash_key(key) & (map->capacity - 1);
    Entry** pp = &map->buckets[index];

    while (*pp) {
        Entry* entry = *pp;
        if (strcmp(entry->key, key) == 0) {
            // This is tricky.
            // This is setting the pervious entries / buckets pointer
            // to point the the next entry to skip this node in the linked list.
            *pp = entry->next;
            free(entry->key);
            if (map->value_cleanup) map->value_cleanup(entry->value);
            free(entry);
            map->size--;
            return true;

        }
        pp = &entry->next;
    }
    return false;
}

void hm_destroy(HashMap* map) {
    if (!map) return;

    for (size_t i = 0; i < map->capacity; i++) {
        Entry* entry = map->buckets[i];
        while (entry) {
            Entry* next = entry->next;
            free(entry->key);
            if (map->value_cleanup) map->value_cleanup(entry->value);
            free(entry);
            entry=next;
        }

    }
    free(map->buckets);
    free(map);
}

// Get the current number of elements in the map
size_t hm_size(HashMap* map) {
    return map->size;
}


// Check if the map is empty
bool hm_is_empty(HashMap* map) {
    return map->size == 0;
}
