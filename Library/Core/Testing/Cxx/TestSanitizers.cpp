/**
 * @file TestSanitizers.cpp
 * @brief Test cases to verify sanitizer functionality
 *
 * This file contains test cases that intentionally trigger sanitizer
 * detections to verify that the sanitizer configuration is working correctly.
 * These tests should only be run when sanitizers are enabled.
 */
#if 0
#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

// Test AddressSanitizer - Buffer overflow detection
void test_address_sanitizer_buffer_overflow()
{
    std::cout << "Testing AddressSanitizer - Buffer overflow detection\n";

    // This should trigger AddressSanitizer
    int* arr = new int[10];

    // Intentional buffer overflow - should be detected by ASan
    for (int i = 0; i < 10; ++i)
    {  // Note: <= instead of <
        arr[i] = i;
    }

    delete[] arr;
}

// Test AddressSanitizer - Use after free detection
void test_address_sanitizer_use_after_free()
{
    try
    {
        std::cout << "Testing AddressSanitizer - Use after free detection\n";

        int* ptr = new int(42);
        delete ptr;

        // Intentional use after free - should be detected by ASan
        std::cout << "Value: " << *ptr << std::endl;
    }
    catch (...)
    {
    }
}

// Test AddressSanitizer - Double free detection
void test_address_sanitizer_double_free()
{
    std::cout << "Testing AddressSanitizer - Double free detection\n";

    int* ptr = new int(42);
    delete ptr;

    // Intentional double free - should be detected by ASan
    delete ptr;
}

// Test UndefinedBehaviorSanitizer - Integer overflow
void test_undefined_behavior_sanitizer_overflow()
{
    std::cout << "Testing UndefinedBehaviorSanitizer - Integer overflow\n";

    int max_int = 2147483647;  // INT_MAX

    // Intentional integer overflow - should be detected by UBSan
    int result = max_int + 1;
    std::cout << "Result: " << result << std::endl;
}

// Test UndefinedBehaviorSanitizer - Null pointer dereference
void test_undefined_behavior_sanitizer_null_deref()
{
    std::cout << "Testing UndefinedBehaviorSanitizer - Null pointer dereference\n";

    int* ptr = nullptr;

    // Intentional null pointer dereference - should be detected by UBSan
    int value = *ptr;
    std::cout << "Value: " << value << std::endl;
}

// Global variable for thread sanitizer test
std::atomic<int> shared_counter{0};
int              non_atomic_shared = 0;

// Test ThreadSanitizer - Data race detection
void test_thread_sanitizer_data_race()
{
    std::cout << "Testing ThreadSanitizer - Data race detection\n";

    auto worker = []()
    {
        for (int i = 0; i < 1000; ++i)
        {
            // This should NOT trigger TSan (atomic)
            shared_counter.fetch_add(1);

            // This SHOULD trigger TSan (data race)
            non_atomic_shared++;
        }
    };

    std::thread t1(worker);
    std::thread t2(worker);

    t1.join();
    t2.join();

    std::cout << "Atomic counter: " << shared_counter.load() << std::endl;
    std::cout << "Non-atomic counter: " << non_atomic_shared << std::endl;
}

// Test MemorySanitizer - Uninitialized memory read
void test_memory_sanitizer_uninit_read()
{
    std::cout << "Testing MemorySanitizer - Uninitialized memory read\n";

    int uninitialized_var;

    // Intentional use of uninitialized variable - should be detected by MSan
    if (uninitialized_var > 0)
    {
        std::cout << "Uninitialized variable is positive\n";
    }
    else
    {
        std::cout << "Uninitialized variable is not positive\n";
    }
}

// Test LeakSanitizer - Memory leak detection
void test_leak_sanitizer_memory_leak()
{
    std::cout << "Testing LeakSanitizer - Memory leak detection\n";

    // Intentional memory leak - should be detected by LSan
    int* leaked_memory = new int[100];

    // Fill the memory to make sure it's actually allocated
    for (int i = 0; i < 100; ++i)
    {
        leaked_memory[i] = i;
    }

    // Don't delete - this creates a memory leak
    std::cout << "Created memory leak of 400 bytes\n";
}

// Safe test function that doesn't trigger sanitizers
void test_sanitizer_safe_operations()
{
    std::cout << "Testing safe operations (should not trigger sanitizers)\n";

    // Safe memory operations
    std::vector<int> safe_vector(10);
    for (size_t i = 0; i < safe_vector.size(); ++i)
    {
        safe_vector[i] = static_cast<int>(i);
    }

    // Safe smart pointer usage
    auto smart_ptr = std::make_unique<int>(42);
    std::cout << "Smart pointer value: " << *smart_ptr << std::endl;

    // Safe atomic operations
    std::atomic<int> safe_atomic{0};
    safe_atomic.store(100);
    std::cout << "Atomic value: " << safe_atomic.load() << std::endl;

    std::cout << "All safe operations completed successfully\n";
}

int main(int argc, char* argv[])
{
    std::cout << "=== XSigma Sanitizer Test Suite ===\n";
    std::cout << "This program tests sanitizer functionality by intentionally\n";
    std::cout << "triggering various types of errors that sanitizers should detect.\n\n";

    test_sanitizer_safe_operations();
    test_address_sanitizer_buffer_overflow();
    test_address_sanitizer_use_after_free();
    test_address_sanitizer_double_free();
    test_undefined_behavior_sanitizer_overflow();
    test_undefined_behavior_sanitizer_null_deref();
    test_thread_sanitizer_data_race();
    test_memory_sanitizer_uninit_read();
    test_leak_sanitizer_memory_leak();

    std::cout << "\nTest completed. If sanitizers are enabled, they should have\n";
    std::cout << "detected and reported the intentional errors above.\n";

    return 0;
}
#endif