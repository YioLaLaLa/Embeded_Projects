/**
 * @file unity_framework.h
 * @brief Unity Test Framework header for embedded C testing
 */

/* Unity Test Framework - Public API */

#ifndef UNITY_FRAMEWORK_H
#define UNITY_FRAMEWORK_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

/* Unity configuration */
#define UNITY_INCLUDE_CONFIG_FILE 0
#define UNITY_SUPPRESS_FRAMEWORK 1

/* Unity return codes */
#define UNITY_SKIP_PRECONDITION (-1)
#define UNITY_EXECUTE_RESET_HOOKS() do {} while(0)

/* Unity internal variables */
extern unsigned int UnityTestMsgAllocFailure;

/* Unity macros */
#define TEST_FAIL_MESSAGE(msg) do { \
    UnityPrint(msg); \
    UnityFail((msg), __LINE__); \
} while(0)

#define TEST_FAIL(msg) TEST_FAIL_MESSAGE(msg)

#define TEST_ASSERT_TRUE(condition) do { \
    if (!(condition)) { \
        UnityFail("Expression was not true", __LINE__); \
    } \
} while(0)

#define TEST_ASSERT_FALSE(condition) TEST_ASSERT_TRUE(!(condition))

#define TEST_ASSERT_EQUAL(expected, actual) do { \
    if ((expected) != (actual)) { \
        UnityFail("Values are not equal", __LINE__); \
    } \
} while(0)

#define TEST_ASSERT_EQUAL_INT(expected, actual) TEST_ASSERT_EQUAL((expected), (actual))

#define TEST_ASSERT_EQUAL_UINT32(expected, actual) do { \
    if ((uint32_t)(expected) != (uint32_t)(actual)) { \
        UnityFail("Unsigned 32-bit values are not equal", __LINE__); \
    } \
} while(0)

#define TEST_ASSERT_EQUAL_HEX32(expected, actual) do { \
    if ((uint32_t)(expected) != (uint32_t)(actual)) { \
        UnityFail("Hex values are not equal", __LINE__); \
    } \
} while(0)

#define TEST_ASSERT_EQUAL_STRING(expected, actual) do { \
    if (strcmp((expected), (actual)) != 0) { \
        UnityFail("Strings are not equal", __LINE__); \
    } \
} while(0)

#define TEST_ASSERT_NOT_EQUAL(expected, actual) do { \
    if ((expected) == (actual)) { \
        UnityFail("Values are equal but should not be", __LINE__); \
    } \
} while(0)

#define TEST_ASSERT_NOT_NULL(ptr) do { \
    if ((ptr) == NULL) { \
        UnityFail("Pointer is NULL", __LINE__); \
    } \
} while(0)

#define TEST_ASSERT_NULL(ptr) TEST_ASSERT_TRUE((ptr) == NULL)

#define TEST_ASSERT_FLOAT_WITHIN(delta, expected, actual) do { \
    float diff = (float)(expected) - (float)(actual); \
    if (diff < 0) diff = -diff; \
    if (diff > (delta)) { \
        UnityFail("Float values not within tolerance", __LINE__); \
    } \
} while(0)

#define TEST_ASSERT_INT_WITHIN(delta, expected, actual) do { \
    int diff = (int)(expected) - (int)(actual); \
    if (diff < 0) diff = -diff; \
    if (diff > (delta)) { \
        UnityFail("Integer values not within tolerance", __LINE__); \
    } \
} while(0)

/* Test case macros */
#define TEST_CASE(x)
#define RUN_TEST(func, line) do { \
    Unity.CurrentTestLineNumber = line; \
    Unity.NumberOfTests++; \
    if (func() == 0) { \
        Unity.TestPasses++; \
    } \
} while(0)

/* Unity function declarations */
void UnityBegin(const char* filename);
int UnityEnd(void);
void UnityPrint(const char* msg);
void UnityFail(const char* msg, unsigned int line);
void UnityAssertFailCallback(const char* msg, unsigned int line, const char* file);
void UnityAssertNumbersWithin(unsigned int delta, unsigned int actual, unsigned int expected, const char* msg, unsigned int line);

/* Unity test context */
typedef struct {
    unsigned int TestPasses;
    unsigned int TestFailures;
    unsigned int TestIgnores;
    unsigned int CurrentTestLineNumber;
    unsigned int NumberOfTests;
    const char* CurrentTest;
    const char* CurrentFile;
} Unity_t;

extern Unity_t Unity;

/* setUp and tearDown */
void setUp(void);
void tearDown(void);

#endif /* UNITY_FRAMEWORK_H */
