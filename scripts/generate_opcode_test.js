#!/usr/bin/env node

const fs = require("fs");

if (process.argv.length < 4) {
  console.error("No input or output filenames provided.");
  process.exit();
}

const jsonFile = process.argv[2];
const outputFile = process.argv[3];

const jsonContents = fs.readFileSync(jsonFile);
const json = JSON.parse(jsonContents);

const testMethodName = (testName) =>
  `test_cpu_opcode_${testName.replace(/\s+/g, "_")}`;

const tests = json.map(
  ({ name, initial, final }) =>
    `
void ${testMethodName(name)}(void) {
    cpu.pc = ${initial.pc};
    cpu.sp = ${initial.sp};
    cpu.a = ${initial.a};
    cpu.b = ${initial.b};
    cpu.c = ${initial.c};
    cpu.d = ${initial.d};
    cpu.e = ${initial.e};
    cpu.f = ${initial.f};
    cpu.h = ${initial.h};
    cpu.l = ${initial.l};
    cpu.ime = ${initial.ime};
${initial.ram.map(([addr, value]) => `    mem_data[${addr}] = ${value};`).join("\n")}

    Cpu_tick(&cpu, &mock_memory);

    TEST_ASSERT_EQUAL_HEX16(${final.pc}, cpu.pc);
    TEST_ASSERT_EQUAL_HEX16(${final.sp}, cpu.sp);
    TEST_ASSERT_EQUAL_HEX8(${final.a}, cpu.a);
    TEST_ASSERT_EQUAL_HEX8(${final.b}, cpu.b);
    TEST_ASSERT_EQUAL_HEX8(${final.c}, cpu.c);
    TEST_ASSERT_EQUAL_HEX8(${final.d}, cpu.d);
    TEST_ASSERT_EQUAL_HEX8(${final.e}, cpu.e);
    TEST_ASSERT_EQUAL_HEX8(${final.f}, cpu.f);
    TEST_ASSERT_EQUAL_HEX8(${final.h}, cpu.h);
    TEST_ASSERT_EQUAL_HEX8(${final.l}, cpu.l);
    TEST_ASSERT_EQUAL_HEX8(${final.ime}, cpu.ime);
}
`,
);

const testRunners = json.map(
  ({ name }) => `    RUN_TEST(${testMethodName(name)});`,
);

const result = `
#include <stdint.h>
#include <string.h>
#include <unity.h>
#include <unity_internals.h>
#include "core/cpu.h"

static Cpu cpu;
static uint8_t mem_data[0x10000];
static Memory mock_memory;

static uint8_t read_mock_mem(const void* ctx, const uint16_t addr) {
    const uint8_t* const restrict memory = ctx;
    return memory[addr];
}

static void write_mock_mem(void* ctx, const uint16_t addr, const uint8_t value) {
    uint8_t* const restrict memory = ctx;
    memory[addr] = value;
}

void setUp(void) {
    cpu = Cpu_new();
    mock_memory = (Memory){
        .ctx = mem_data,
        .read = read_mock_mem,
        .write = write_mock_mem,
    };
    memset(mem_data, 0, sizeof(mem_data));
}

void tearDown(void) {}

${tests.join("")}

int main() {
    UNITY_BEGIN();
${testRunners.join("\n")}
    return UNITY_END();
}
`;

fs.writeFileSync(outputFile, result);
