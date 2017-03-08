#include "dr_api.h"
#include "hashtable.h"

#define SIZE    (32)

static hashtable_t g_ht_data;

static void free_mem_SIZE(void *ptr) {
    dr_global_free(ptr, SIZE);
}

static void event_exit(void) {
    hashtable_delete(&g_ht_data);
}

DR_EXPORT void dr_init(client_id_t id) {
    hashtable_init_ex(&g_ht_data, 13, HASH_INTPTR, false, false,
            free_mem_SIZE, NULL, NULL);
    dr_register_exit_event(event_exit);
}

