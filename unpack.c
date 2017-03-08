#include "dr_api.h"
#include "hashtable.h"
#include "drmgr.h"
#include <string.h>

#define LOG(level, fmt, ...) \
    do {\
        dr_log(NULL, LOG_ALL, (level), "%s:%d:unpack:" fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
    } while (0)

#define DEBUG(fmt, ...) LOG(3, "D: " fmt, ##__VA_ARGS__)
#define INFO(fmt, ...) LOG(2, "I: " fmt, ##__VA_ARGS__)
#define WARN(fmt, ...) LOG(1, "W: " fmt, ##__VA_ARGS__)
#define ERROR(fmt, ...) LOG(0, "E: " fmt, ##__VA_ARGS__)

#define SIZE    (32)

static hashtable_t g_ht_data;

static void dump_memory(const void *p_data, size_t size, const app_pc target) {
    file_t dump = INVALID_FILE;
    int ret;
    ssize_t written = -1;
    char dumpfname[256];
    
    ret = dr_snprintf(dumpfname, sizeof(dumpfname), "dump.%p.%p", target, p_data);
    if (0 > ret || sizeof(dumpfname) <= ret) {
        ERROR("failed to generate dump name (%d)", ret);
        return;
    }

    dump = dr_open_file(dumpfname, DR_FILE_WRITE_OVERWRITE);
    if (INVALID_FILE == dump) {
        ERROR("opening file for dump failed");
        return;
    }

    while (0 < size) {
        written = dr_write_file(dump, p_data, size);
        if (ret <= 0) {
            ERROR("writing dump file failed");
            break;
        }
        p_data += written;
        size -= written;
    }

    dr_close_file(dump);
    return;
}

static void free_mem_SIZE(void *ptr) {
    dr_global_free(ptr, SIZE);
}

static bool should_dump(const dr_mem_info_t *info) {
    bool ret = true;
    void *prev = NULL;
    void *data = NULL;

    data = dr_global_alloc(SIZE);
    DR_ASSERT(data);
    
    if (false == dr_safe_read(info->base_pc, SIZE, data, NULL)) {
        ret = false;
        free_mem_SIZE(data);
        goto exit;
    }

    prev = hashtable_add_replace(&g_ht_data, info->base_pc, data);
    if (NULL == prev) {
        ret = true;
        goto exit;
    }

    if (0 == memcmp(prev, data, SIZE)) {
        ret = false;
    }

exit:
    if (prev) { free_mem_SIZE(prev); }

    return ret;
}

static void check_target(app_pc pc, app_pc target) {
    //TODO: locking?
    dr_mem_info_t info = {0};

    if (false == dr_query_memory_ex(target, &info)) {
        WARN("cant query memory %p, going to call it????", target);
        return;
    }

    if (!(info.prot & DR_MEMPROT_WRITE) && (info.type & DR_MEMTYPE_IMAGE)) {
        //seems legit, no write permissions and mapped binary
        return;
    }

    if (!should_dump(&info)) {
        DEBUG("fishy but already dumped...");
        return;
    }

    INFO("found fishy behavior in %p, dumping", target);
    dump_memory(info.base_pc, info.size, target);
}

static dr_emit_flags_t
event_bb_insert(void *drcontext, void *tag, instrlist_t *bb,
                instr_t *instr, bool for_trace, bool translating,
                void *user_data) {
    if (NULL == instr_get_app_pc(instr)) {
        return DR_EMIT_DEFAULT;
    }

    if (!instr_is_call_indirect(instr) && !instr_is_mbr(instr)) {
        return DR_EMIT_DEFAULT;
    }

    DEBUG("instrumeting %p", instr_get_app_pc(instr));
    dr_insert_mbr_instrumentation(drcontext, bb, instr, check_target,
                                  SPILL_SLOT_1);
    return DR_EMIT_DEFAULT;
}

static void event_exit(void) {
    INFO("finised");
    hashtable_delete(&g_ht_data);
    drmgr_exit();
}

DR_EXPORT void dr_init(client_id_t id) {
    /* Specify priority relative to other instrumentation operations: */
    drmgr_priority_t priority = {sizeof(priority), "unpack", NULL, NULL, 0};
    INFO("starting");

    drmgr_init();
    hashtable_init_ex(&g_ht_data, 4, HASH_INTPTR, false, false,
            free_mem_SIZE, NULL, NULL);
    dr_register_exit_event(event_exit);

    if (!drmgr_register_bb_instrumentation_event(NULL,
                event_bb_insert, &priority)) {
        DR_ASSERT(false);
        return;
    }
}

