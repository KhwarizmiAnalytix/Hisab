---
name: new-test
description: Scaffold a new XSigma unit test file with the correct Google Test macro (TEST/TEST_F, or Core's own legacy test macro only inside Library/Core) and correctly placed under a library's Testing/Cxx directory. Use when the user asks to add/write a test for a class or function in Library/*.
---

# new-test

Scaffolds a test file matching XSigma's existing test conventions (see root
`/CLAUDE.md` "Testing" section). Test files are discovered automatically by
CMake glob (`Test*.cpp` under `Testing/Cxx/`) — **no CMakeLists.txt edit is
needed** for a new file, only for a new subdirectory.

## Steps

1. **Find the right location.** For a source file at
   `Library/<Lib>/<path>/<name>.h`, the test goes in
   `Library/<Lib>/Testing/Cxx/Test<ClassName>.cpp` (CamelCase class name,
   even though the rest of the codebase is `snake_case` — this is
   test-file-only). Check for an existing test file for the same class
   first (`grep -rl` the class name under `Testing/Cxx/`) and add cases
   there instead of creating a duplicate file.

2. **Pick the test macro based on which library this is** — this is not
   uniform project-wide, verified by actual usage counts (`TEST`/`TEST_F`
   outnumber Core's legacy test macro ~14:1 overall):
   - `Library/Core`: either plain `TEST`/`TEST_F` or its own legacy
     `<macro>(module, name)`-style test macro are in use (check
     `Library/Core/Testing/baseTest.h` for its exact name/signature) — grep
     the target directory for the file you're adding alongside and match
     whichever style it already uses.
   - Every other library (`Logging`, `Memory`, `Parallel`, `Profiler`,
     `Vectorization`): use plain Google Test **`TEST(Suite, case)`** or
     **`TEST_F(FixtureClass, case)`**. These libraries don't include the
     header Core's legacy macro is defined in — don't add it there.
   - When in doubt, open one existing `Test*.cpp` in the same
     `Testing/Cxx/` directory and copy its macro choice, namespace usage
     (`using namespace <lib>;`, e.g. `memory`, `profiler::profiler_impl`,
     `vectorization`), and license header verbatim rather than composing
     from memory.

3. **Write cases.** One behavior per case. At minimum include:
   - happy path
   - a boundary/edge case
   - an error/failure path (invalid input, null, empty collection) that
     checks the return value — this codebase does not throw for expected
     failures (see root `/CLAUDE.md` error-handling policy), so assert on
     the returned `bool`/`optional`/result, not on an exception.

   ```cpp
   // Most libraries (Memory/Vectorization/Profiler/Logging/Parallel):
   using namespace memory;

   TEST(MyClass, handles_valid_input)
   {
       my_class obj;
       EXPECT_TRUE(obj.do_something());
   }

   TEST(MyClass, rejects_invalid_input)
   {
       my_class obj;
       EXPECT_FALSE(obj.do_something_with(-1));
   }

   // Fixture-based, when shared setup is needed:
   class MyClassTest : public ::testing::Test
   {
   protected:
       void SetUp() override { /* ... */ }
   };

   TEST_F(MyClassTest, handles_valid_input)
   {
       EXPECT_TRUE(obj_.do_something());
   }
   ```

   ```cpp
   // Library/Core only, if the neighboring file already uses its legacy
   // test macro (name/signature: see Library/Core/Testing/baseTest.h):
   <CORE_TEST_MACRO>(my_class, handles_valid_input)
   {
       my_class obj;
       EXPECT_TRUE(obj.do_something());
   }
   ```

4. **GPU tests**: name the file/cases so they match the exclusion patterns
   already in the library's `Testing/Cxx/CMakeLists.txt` (typically
   `TestGpu*`/`TestCuda*`) so they're automatically skipped on
   backends that don't support them — check that CMakeLists.txt in the
   target library before assuming a new naming pattern will be excluded
   correctly.

5. **Build and run just this test's library** rather than the whole tree:
   ```
   cd Scripts
   python3 setup.py config.build.test.native --project.<library-lowercase>
   ```
   (see the `xsigma-build` skill for more invocation patterns).

Don't introduce a third test-macro style, and don't import Core's legacy
test macro into a library outside `Core` — match what's already there.
