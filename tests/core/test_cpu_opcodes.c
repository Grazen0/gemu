#include <cjson/cJSON.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unity.h>
#include "core/cpu.h"

static Cpu cpu;
static uint8_t mem_data[0x10000];
static Memory mock_memory;

typedef struct RamEntry {
    uint16_t address;
    uint8_t value;
} RamEntry;

typedef struct CpuState {
    uint16_t pc;
    uint16_t sp;
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
    uint8_t e;
    uint8_t f;
    uint8_t h;
    uint8_t l;
    uint8_t ime;
    RamEntry* ram;
    int ram_len;
} CpuState;

static uint8_t read_mock_mem(const void* _ctx, const uint16_t addr) {
    (void)_ctx;
    return mem_data[addr];
}

static void write_mock_mem(void* _ctx, const uint16_t addr, const uint8_t value) {
    (void)_ctx;
    mem_data[addr] = value;
}

CpuState CpuState_from_cjson(const cJSON* const src) {
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

    RamEntry* const ram = calloc(ram_len, sizeof(*ram));

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

void CpuState_destroy(CpuState* const restrict state) {
    free(state->ram);
    state->ram = NULL;
    state->ram_len = 0;
}

void setUp(void) {
    mock_memory = (Memory){
        .ctx = NULL,
        .read = read_mock_mem,
        .write = write_mock_mem,
    };
}

void run_cpu_tick_test(
    const CpuState* const restrict initial_state,
    const CpuState* const restrict final_state
) {
    cpu = Cpu_new();
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
    memset(mem_data, 0, sizeof(mem_data));
    for (int j = 0; j < initial_state->ram_len; ++j) {
        const RamEntry* const entry = &initial_state->ram[j];
        mem_data[entry->address] = entry->value;
    }

    Cpu_tick(&cpu, &mock_memory);

    TEST_ASSERT_EQUAL_HEX16(cpu.pc, final_state->pc);
    TEST_ASSERT_EQUAL_HEX16(cpu.sp, final_state->sp);
    TEST_ASSERT_EQUAL_HEX8(cpu.a, final_state->a);
    TEST_ASSERT_EQUAL_HEX8(cpu.b, final_state->b);
    TEST_ASSERT_EQUAL_HEX8(cpu.c, final_state->c);
    TEST_ASSERT_EQUAL_HEX8(cpu.d, final_state->d);
    TEST_ASSERT_EQUAL_HEX8(cpu.e, final_state->e);
    TEST_ASSERT_EQUAL_HEX8(cpu.f, final_state->f);
    TEST_ASSERT_EQUAL_HEX8(cpu.h, final_state->h);
    TEST_ASSERT_EQUAL_HEX8(cpu.l, final_state->l);
    TEST_ASSERT_EQUAL_HEX8(cpu.ime, final_state->ime);

    for (size_t j = 0; j < 0x10000; ++j) {
        uint8_t expected_value = 0x00;

        for (int k = 0; k < final_state->ram_len; ++k) {
            const RamEntry* const entry = &final_state->ram[k];
            if (j == entry->address) {
                expected_value = entry->value;
                break;
            }
        }

        TEST_ASSERT_EQUAL_HEX8(expected_value, mem_data[j]);
    }
}

void run_opcode_tests(const char* const restrict filepath) {
    FILE* const file = fopen(filepath, "r");
    TEST_ASSERT_NOT_NULL_MESSAGE(file, "could not open JSON file");

    fseek(file, 0, SEEK_END);
    const size_t file_len = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* const content = calloc(file_len, sizeof(*content));
    const size_t read_bytes = fread(content, sizeof(*content), file_len, file);
    TEST_ASSERT_EQUAL_MESSAGE(file_len, read_bytes, "could not read JSON file");
    fclose(file);

    cJSON* const cases = cJSON_ParseWithLength(content, file_len);
    TEST_ASSERT_NOT_NULL_MESSAGE(cases, "could not parse JSON");
    free(content);

    const int cases_len = cJSON_GetArraySize(cases);

    for (int i = 0; i < cases_len; ++i) {
        const cJSON* const test_case = cJSON_GetArrayItem(cases, i);

        const cJSON* const name = cJSON_GetObjectItemCaseSensitive(test_case, "name");
        TEST_MESSAGE(name->valuestring);

        const cJSON* const initial = cJSON_GetObjectItemCaseSensitive(test_case, "initial");
        const cJSON* const final = cJSON_GetObjectItemCaseSensitive(test_case, "final");

        CpuState initial_state = CpuState_from_cjson(initial);
        CpuState final_state = CpuState_from_cjson(final);

        run_cpu_tick_test(&initial_state, &final_state);

        CpuState_destroy(&initial_state);
        CpuState_destroy(&final_state);
    }

    cJSON_Delete(cases);
}

void test_cpu_opcode_00(void) {
    run_opcode_tests("core/data/00.json");
    run_opcode_tests("core/data/01.json");
    run_opcode_tests("core/data/02.json");
    run_opcode_tests("core/data/03.json");
    run_opcode_tests("core/data/04.json");
    run_opcode_tests("core/data/05.json");
    run_opcode_tests("core/data/06.json");
    run_opcode_tests("core/data/07.json");
    run_opcode_tests("core/data/08.json");
    run_opcode_tests("core/data/09.json");
}
