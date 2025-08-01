#include "cpu.h"
#include "stdinc.h"
#include <cjson/cJSON.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unity.h>

typedef struct DumbRam {
    bool active[0x10000];
    u8 data[0x10000];
} DumbRam;

typedef struct RamEntry {
    u16 address;
    u8 value;
} RamEntry;

typedef struct CpuState {
    u16 pc;
    u16 sp;
    u8 a;
    u8 b;
    u8 c;
    u8 d;
    u8 e;
    u8 f;
    u8 h;
    u8 l;
    u8 ime;
    RamEntry* ram;
    int ram_len;
} CpuState;

static u8 read_mock_ram(const void* ctx, const u16 addr) {
    const DumbRam* const ram = ctx;
    TEST_ASSERT_TRUE_MESSAGE(
        ram->active[addr], "tried to read from inactive memory"
    );
    return ram->data[addr];
}

static void write_mock_ram(void* ctx, const u16 addr, const u8 value) {
    DumbRam* const ram = ctx;
    TEST_ASSERT_TRUE_MESSAGE(
        ram->active[addr], "tried to write into inactive memory"
    );
    ram->data[addr] = value;
}

static CpuState CpuState_from_cjson(const cJSON* const src) {
    const cJSON* const pc = cJSON_GetObjectItemCaseSensitive(src, "pc");
    const cJSON* const sp = cJSON_GetObjectItemCaseSensitive(src, "sp");
    const cJSON* const a = cJSON_GetObjectItemCaseSensitive(src, "a");
    const cJSON* const b = cJSON_GetObjectItemCaseSensitive(src, "b");
    const cJSON* const c = cJSON_GetObjectItemCaseSensitive(src, "c");
    const cJSON* const d = cJSON_GetObjectItemCaseSensitive(src, "d");
    const cJSON* const e = cJSON_GetObjectItemCaseSensitive(src, "e");
    const cJSON* const f = cJSON_GetObjectItemCaseSensitive(src, "f");
    const cJSON* const h = cJSON_GetObjectItemCaseSensitive(src, "h");
    const cJSON* const l = cJSON_GetObjectItemCaseSensitive(src, "l");
    const cJSON* const ime = cJSON_GetObjectItemCaseSensitive(src, "ime");
    const cJSON* const ram_src = cJSON_GetObjectItemCaseSensitive(src, "ram");

    const int ram_len = cJSON_GetArraySize(ram_src);

    RamEntry* const ram = malloc(ram_len * sizeof(*ram));

    for (int i = 0; i < ram_len; ++i) {
        const cJSON* const entry_src = cJSON_GetArrayItem(ram_src, i);

        const cJSON* const address = cJSON_GetArrayItem(entry_src, 0);
        const cJSON* const value = cJSON_GetArrayItem(entry_src, 1);

        ram[i] = (RamEntry){
            .address = address->valueint,
            .value = value->valueint,
        };
    }

    return (CpuState){
        .pc = pc->valueint,
        .sp = sp->valueint,
        .a = a->valueint,
        .b = b->valueint,
        .c = c->valueint,
        .d = d->valueint,
        .e = e->valueint,
        .f = f->valueint,
        .h = h->valueint,
        .l = l->valueint,
        .ime = ime->valueint,
        .ram = ram,
        .ram_len = ram_len,
    };
}

static void CpuState_destroy(CpuState* const restrict state) {
    free(state->ram);
    state->ram = nullptr;
    state->ram_len = 0;
}

static void run_cpu_tick_test(
    const CpuState* const restrict initial_state,
    const CpuState* const restrict final_state,
    const char* const restrict test_name
) {
    Cpu cpu = Cpu_new();

    DumbRam dumb_ram = {};

    Memory mock_memory = (Memory){
        .ctx = &dumb_ram,
        .read = read_mock_ram,
        .write = write_mock_ram,
    };

    cpu.pc = initial_state->pc;
    cpu.sp = initial_state->sp;
    cpu.a = initial_state->a;
    cpu.b = initial_state->b;
    cpu.c = initial_state->c;
    cpu.d = initial_state->d;
    cpu.e = initial_state->e;
    cpu.f = initial_state->f;
    cpu.h = initial_state->h;
    cpu.l = initial_state->l;
    cpu.ime = initial_state->ime;

    // Set up memory
    for (int j = 0; j < initial_state->ram_len; ++j) {
        const RamEntry* const entry = &initial_state->ram[j];
        dumb_ram.data[entry->address] = entry->value;
        dumb_ram.active[entry->address] = true;
    }

    Cpu_tick(&cpu, &mock_memory);

    char msg_buffer[32];

    snprintf(msg_buffer, sizeof(msg_buffer), "(%s, pc)", test_name);
    TEST_ASSERT_EQUAL_HEX16_MESSAGE(final_state->pc, cpu.pc, msg_buffer);
    snprintf(msg_buffer, sizeof(msg_buffer), "(%s, sp)", test_name);
    TEST_ASSERT_EQUAL_HEX16_MESSAGE(final_state->sp, cpu.sp, msg_buffer);
    snprintf(msg_buffer, sizeof(msg_buffer), "(%s, a)", test_name);
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(final_state->a, cpu.a, msg_buffer);
    snprintf(msg_buffer, sizeof(msg_buffer), "(%s, b)", test_name);
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(final_state->b, cpu.b, msg_buffer);
    snprintf(msg_buffer, sizeof(msg_buffer), "(%s, c)", test_name);
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(final_state->c, cpu.c, msg_buffer);
    snprintf(msg_buffer, sizeof(msg_buffer), "(%s, d)", test_name);
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(final_state->d, cpu.d, msg_buffer);
    snprintf(msg_buffer, sizeof(msg_buffer), "(%s, e)", test_name);
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(final_state->e, cpu.e, msg_buffer);
    snprintf(msg_buffer, sizeof(msg_buffer), "(%s, f)", test_name);
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(final_state->f, cpu.f, msg_buffer);
    snprintf(msg_buffer, sizeof(msg_buffer), "(%s, h)", test_name);
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(final_state->h, cpu.h, msg_buffer);
    snprintf(msg_buffer, sizeof(msg_buffer), "(%s, l)", test_name);
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(final_state->l, cpu.l, msg_buffer);
    snprintf(msg_buffer, sizeof(msg_buffer), "(%s, ime)", test_name);
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(final_state->ime, cpu.ime, msg_buffer);

    for (int i = 0; i < final_state->ram_len; ++i) {
        const RamEntry* const entry = &final_state->ram[i];

        snprintf(
            msg_buffer,
            sizeof(msg_buffer),
            "(%s, ram @ $%04X)",
            test_name,
            entry->address
        );
        TEST_ASSERT_EQUAL_HEX8_MESSAGE(
            entry->value, dumb_ram.data[entry->address], msg_buffer
        );
    }
}

static void run_opcode_test_file(const char* const restrict filepath) {
    FILE* const file = fopen(filepath, "r");
    TEST_ASSERT_NOT_NULL_MESSAGE(file, "could not open JSON file");

    fseek(file, 0, SEEK_END);
    const size_t file_len = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* const file_content = malloc(file_len * sizeof(*file_content));
    const size_t read_bytes =
        fread(file_content, sizeof(*file_content), file_len, file);
    TEST_ASSERT_EQUAL_MESSAGE(file_len, read_bytes, "could not read JSON file");
    fclose(file);

    cJSON* const test_cases = cJSON_ParseWithLength(file_content, file_len);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_cases, "could not parse JSON");
    free(file_content);

    for (cJSON* test_case = test_cases->child; test_case != nullptr;
         test_case = test_case->next) {
        const cJSON* const name =
            cJSON_GetObjectItemCaseSensitive(test_case, "name");

        const cJSON* const initial =
            cJSON_GetObjectItemCaseSensitive(test_case, "initial");
        const cJSON* const final =
            cJSON_GetObjectItemCaseSensitive(test_case, "final");

        CpuState initial_state = CpuState_from_cjson(initial);
        CpuState final_state = CpuState_from_cjson(final);
        TEST_ASSERT_EQUAL(initial_state.ram_len, final_state.ram_len);

        run_cpu_tick_test(&initial_state, &final_state, name->valuestring);

        CpuState_destroy(&initial_state);
        CpuState_destroy(&final_state);

        test_case = test_case->next;
    }

    cJSON_Delete(test_cases);
}

static int dirent_filter(const struct dirent* const restrict entry) {
    return entry->d_type == DT_REG && entry->d_name[0] != '.';
}

void test_cpu_opcodes(void) {
    struct dirent** entries = nullptr;
    const int entries_len =
        scandir("data/core/cpu_opcodes", &entries, dirent_filter, alphasort);
    TEST_ASSERT_NOT_EQUAL_MESSAGE(
        -1, entries_len, "could not read data directory"
    );

    for (int i = 0; i < entries_len; ++i) {
        struct dirent* const entry = entries[i];

        char full_path[512];
        snprintf(
            full_path,
            sizeof(full_path),
            "data/core/cpu_opcodes/%s",
            entry->d_name
        );
        free(entry);

        run_opcode_test_file(full_path);
    }

    free((void*)entries);
}
