// Comprehensive test suite for Dead Store Elimination (DSE)
// Compile: clang -O0 -Xclang -disable-O0-optnone -S -emit-llvm dse_test_suite.c -o dse_test_suite.ll
// Simplify: opt -passes=mem2reg dse_test_suite.ll -S -o dse_test_suite_simplified.ll
// Run DSE: opt -load-pass-plugin=./libDeadStoreElimination.dylib -passes="dse" dse_test_suite_simplified.ll -S -o dse_test_suite_optimized.ll

// ============================================================
// TEST 1: Simple dead store - should be eliminated
// ============================================================
void test1_simple_dead_store(int *p) {
    *p = 1;    // DEAD - overwritten by next store
    *p = 2;    // LIVE
}

// ============================================================
// TEST 2: Three consecutive stores - first two should be eliminated
// ============================================================
void test2_multiple_dead_stores(int *p) {
    *p = 1;    // DEAD
    *p = 2;    // DEAD
    *p = 3;    // LIVE
}

// ============================================================
// TEST 3: Store followed by load - should NOT be eliminated
// ============================================================
void test3_store_with_use(int *p, int *result) {
    *p = 5;         // LIVE - used by the load
    *result = *p;   // Uses the store
    *p = 10;        // LIVE - final value
}

// ============================================================
// TEST 4: Dead store in same location after computation
// ============================================================
void test4_computed_dead_store(int *p, int x) {
    *p = x * 2;      // DEAD
    *p = x + 5;      // LIVE
}

// ============================================================
// TEST 5: Dead store with different pointer (same location)
// ============================================================
void test5_same_location_different_syntax(int *p) {
    p[0] = 100;      // DEAD
    *p = 200;        // LIVE (same location as p[0])
}

// ============================================================
// TEST 6: NOT dead - conditional paths
// ============================================================
void test6_conditional_not_dead(int *p, int cond) {
    *p = 1;          // NOT DEAD - might be the final value
    if (cond) {
        *p = 2;      // Only overwrites on this path
    }
    // At exit, *p could be 1 or 2
}

// ============================================================
// TEST 7: Dead store before unconditional overwrite
// ============================================================
void test7_unconditional_overwrite(int *p, int cond) {
    *p = 1;          // DEAD
    if (cond) {
        *p = 2;      // Overwrites on this path
    } else {
        *p = 3;      // Overwrites on this path
    }
    // All paths overwrite, so first store is dead
}

// ============================================================
// TEST 8: NOT dead - different locations (aliasing)
// ============================================================
void test8_different_locations(int *p, int *q) {
    *p = 10;         // NOT DEAD - may not alias with q
    *q = 20;         // Different pointer
    // We can't prove p and q are the same location
}

// ============================================================
// TEST 9: Dead store in loop - tricky case
// ============================================================
void test9_loop_dead_store(int *p, int n) {
    for (int i = 0; i < n; i++) {
        *p = i;      // Each iteration overwrites previous
    }
    *p = 999;        // Overwrites final loop value
    // The assignment inside the loop is NOT dead (needed for iterations)
    // But if we had *p = 0 before the loop, that would be dead
}

// ============================================================
// TEST 10: Dead initialization before loop
// ============================================================
void test10_dead_init_before_loop(int *p, int n) {
    *p = 0;          // DEAD if loop always executes at least once
    for (int i = 0; i < n; i++) {
        *p = i;
    }
}

// ============================================================
// TEST 11: Array elements - dead stores
// ============================================================
void test11_array_dead_stores(int *arr) {
    arr[0] = 1;      // DEAD
    arr[0] = 2;      // LIVE
    
    arr[1] = 10;     // LIVE
    arr[2] = 20;     // LIVE
}

// ============================================================
// TEST 12: Post-domination check - return in between
// ============================================================
int test12_early_return(int *p, int cond) {
    *p = 1;          // NOT DEAD - might be final value
    if (cond) {
        return 0;    // Early return - second store doesn't post-dominate
    }
    *p = 2;          // Only executed conditionally
    return 1;
}

// ============================================================
// TEST 13: Multiple dead stores in sequence
// ============================================================
void test13_chain_of_dead_stores(int *p) {
    *p = 1;          // DEAD
    *p = 2;          // DEAD
    *p = 3;          // DEAD
    *p = 4;          // DEAD
    *p = 5;          // LIVE
}

// ============================================================
// TEST 14: Dead store with pointer arithmetic
// ============================================================
void test14_pointer_arithmetic(int *p) {
    *p = 10;         // DEAD
    *(p + 0) = 20;   // LIVE (same location)
}

// ============================================================
// TEST 15: Struct member stores
// ============================================================
struct Point {
    int x;
    int y;
};

void test15_struct_members(struct Point *pt) {
    pt->x = 1;       // DEAD
    pt->x = 2;       // LIVE
    
    pt->y = 10;      // LIVE (different field)
}

// ============================================================
// TEST 16: Dead store followed by function call
// ============================================================
void external_func(int *p);

void test16_call_may_use(int *p) {
    *p = 5;              // NOT DEAD - function might read it
    external_func(p);    // May read *p
    *p = 10;             // LIVE
}

// ============================================================
// TEST 17: Store after volatile load (edge case)
// ============================================================
void test17_after_volatile(int *p, volatile int *v) {
    *p = 1;          // DEAD
    int x = *v;      // Volatile load (side effect)
    *p = 2;          // LIVE
}

// ============================================================
// TEST 18: Complex control flow with post-domination
// ============================================================
void test18_complex_postdom(int *p, int a, int b) {
    *p = 0;          // DEAD - all paths lead to overwrite
    
    if (a > 0) {
        if (b > 0) {
            *p = 1;
        } else {
            *p = 2;
        }
    } else {
        *p = 3;
    }
    // All paths set *p, so initial store is dead
}

// ============================================================
// TEST 19: NOT dead - partial overwrite in loop
// ============================================================
void test19_partial_loop(int *p, int n) {
    *p = 100;        // NOT DEAD - used if n <= 0
    for (int i = 0; i < n; i++) {
        *p = i;
    }
}

// ============================================================
// TEST 20: Dead store with known array indices
// ============================================================
void test20_known_indices(int *arr) {
    arr[5] = 42;     // DEAD
    arr[5] = 99;     // LIVE
    
    arr[3] = 10;     // LIVE (different index)
}

// ============================================================
// Helper main for testing
// ============================================================
int main() {
    int x;
    int arr[10];
    struct Point pt;
    
    test1_simple_dead_store(&x);
    test2_multiple_dead_stores(&x);
    test3_store_with_use(&x, &arr[0]);
    test4_computed_dead_store(&x, 5);
    test5_same_location_different_syntax(&x);
    test6_conditional_not_dead(&x, 1);
    test7_unconditional_overwrite(&x, 1);
    test8_different_locations(&x, &arr[1]);
    test9_loop_dead_store(&x, 10);
    test10_dead_init_before_loop(&x, 10);
    test11_array_dead_stores(arr);
    test12_early_return(&x, 0);
    test13_chain_of_dead_stores(&x);
    test14_pointer_arithmetic(&x);
    test15_struct_members(&pt);
    test18_complex_postdom(&x, 1, 1);
    test19_partial_loop(&x, 5);
    test20_known_indices(arr);
    
    return 0;
}