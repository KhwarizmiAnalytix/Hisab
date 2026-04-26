include_guard()

if(UNIX)
  include(CheckFunctionExists)
endif()

include(CheckIncludeFile)
include(CheckCSourceCompiles)
include(CheckCSourceRuns)
include(CheckCCompilerFlag)
include(CheckCXXSourceCompiles)
include(CheckCXXCompilerFlag)
include(CMakePushCheckState)

# ---[ If running on Ubuntu, check system version and compiler version.
if(EXISTS "/etc/os-release")
  execute_process(
    COMMAND "sed" "-ne" "s/^ID=\\([a-z]\\+\\)$/\\1/p" "/etc/os-release"
    OUTPUT_VARIABLE OS_RELEASE_ID OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  execute_process(
    COMMAND "sed" "-ne" "s/^VERSION_ID=\"\\([0-9\\.]\\+\\)\"$/\\1/p" "/etc/os-release"
    OUTPUT_VARIABLE OS_RELEASE_VERSION_ID OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if(OS_RELEASE_ID STREQUAL "ubuntu")
    if(OS_RELEASE_VERSION_ID VERSION_GREATER "17.04")
      if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "6.0.0")
          message(FATAL_ERROR "Please use GCC 6 or higher on Ubuntu 17.04 and higher.")
        endif()
      endif()
    endif()
  endif()
endif()

# ---[ Apply platform-specific optimization flags after compiler validation
if(COMMAND quarisma_apply_platform_flags)
  quarisma_apply_platform_flags()
endif()

# ---[ Check if std::exception_ptr is supported.
cmake_push_check_state(RESET)
set(CMAKE_REQUIRED_FLAGS "-std=c++20")
check_cxx_source_compiles(
  "#include <string>
    #include <exception>
    int main(int argc, char** argv) {
      std::exception_ptr eptr;
      try {
          std::string().at(1);
      } catch(...) {
          eptr = std::current_exception();
      }
    }"
  TMP_EXCEPTION_PTR_SUPPORTED
)

if(TMP_EXCEPTION_PTR_SUPPORTED)
  message("--std::exception_ptr is supported.")
  set(HAS_EXCEPTION_PTR 1)
else()
  message("--std::exception_ptr is NOT supported.")
endif()
cmake_pop_check_state()

if(NOT TMP_NEED_TO_TURN_OFF_DEPRECATION_WARNING AND NOT MSVC)
  message("--Turning off deprecation warning.")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")
endif()

if(NOT INTERN_BUILD_MOBILE)
  # ---[ Check if the compiler has SSE support.
  cmake_push_check_state(RESET)
  set(VECTORIZATION OFF)
  if(NOT MSVC)
    set(CMAKE_REQUIRED_FLAGS "-msse4.2 -msse4.1 -msse2 -msse")
  endif()
  check_cxx_source_compiles(
    "#include <immintrin.h>
      int main() {
        __m128 a, b;
        a = _mm_set1_ps (1);
        _mm_add_ps(a, a);
        return 0;
      }"
    TMP_COMPILER_SUPPORTS_SSE_EXTENSIONS
  )
  if(TMP_COMPILER_SUPPORTS_SSE_EXTENSIONS)
    message("--Current compiler supports sse extension.")
    if(VECTORIZATION_TYPE STREQUAL "sse")
      set(HAS_SSE 1)
      set(VECTORIZATION ON)
      set(VECTORIZATION_COMPILER_FLAGS "${CMAKE_REQUIRED_FLAGS}")
    endif()
  endif()
  cmake_pop_check_state()

  # ---[ Check if the compiler has AVX support.
  cmake_push_check_state(RESET)
  if(MSVC)
    set(CMAKE_REQUIRED_FLAGS "/arch:AVX /D__F16C__")
  else()
    set(CMAKE_REQUIRED_FLAGS "-mavx -mf16c")
  endif()
  check_cxx_source_compiles(
    "#include <immintrin.h>
      int main() {
        __m256 a, b;
        a = _mm256_set1_ps (1);
        b = a;
        _mm256_add_ps (a,a);
        return 0;
      }"
    TMP_COMPILER_SUPPORTS_AVX_EXTENSIONS
  )
  if(TMP_COMPILER_SUPPORTS_AVX_EXTENSIONS)
    message("--Current compiler supports avx extension.")
    if(VECTORIZATION_TYPE STREQUAL "avx")
      set(HAS_AVX 1)
      set(VECTORIZATION ON)
      set(VECTORIZATION_COMPILER_FLAGS "${CMAKE_REQUIRED_FLAGS}")
    endif()
  endif()
  cmake_pop_check_state()

  # ---[ Check if the compiler has AVX2 support.
  cmake_push_check_state(RESET)
  if(MSVC)
    set(CMAKE_REQUIRED_FLAGS "/arch:AVX2 /D__F16C__")
  else()
    set(CMAKE_REQUIRED_FLAGS "-mavx2 -mf16c")
  endif()
  check_cxx_source_compiles(
    "#include <immintrin.h>
      int main() {
        __m256i a, b;
        a = _mm256_set1_epi8 (1);
        b = a;
        _mm256_add_epi8 (a,a);
        __m256i x;
        _mm256_extract_epi64(x, 0); // we rely on this in our AVX2 code
        return 0;
      }"
    TMP_COMPILER_SUPPORTS_AVX2_EXTENSIONS
  )
  if(TMP_COMPILER_SUPPORTS_AVX2_EXTENSIONS)
    message("--Current compiler supports avx2 extension.")
    if(VECTORIZATION_TYPE STREQUAL "avx2")
      set(HAS_AVX2 1)
      set(VECTORIZATION ON)
      set(VECTORIZATION_COMPILER_FLAGS "${CMAKE_REQUIRED_FLAGS}")
    endif()
  endif()
  cmake_pop_check_state()

  # ---[ Check if the compiler has AVX512 support.
  cmake_push_check_state(RESET)
  if(MSVC)
    set(CMAKE_REQUIRED_FLAGS "/D__AVX512F__ /D__AVX512DQ__ /D__AVX512VL__ /D__F16C__")
  else()
    set(CMAKE_REQUIRED_FLAGS "-mavx512f -mavx512dq -mavx512vl -mf16c")
  endif()
  check_cxx_source_compiles(
    "#if defined(_MSC_VER)
     #include <intrin.h>
     #else
     #include <immintrin.h>
     #endif
     // check avx512f
     __m512 addConstant(__m512 arg) {
       return _mm512_add_ps(arg, _mm512_set1_ps(1.f));
     }
     // check avx512dq
     __m512 andConstant(__m512 arg) {
       return _mm512_and_ps(arg, _mm512_set1_ps(1.f));
     }
     int main() {
       __m512i a = _mm512_set1_epi32(1);
       __m256i ymm = _mm512_extracti64x4_epi64(a, 0);
       ymm = _mm256_abs_epi64(ymm); // check avx512vl
       __mmask16 m = _mm512_cmp_epi32_mask(a, a, _MM_CMPINT_EQ);
       __m512i r = _mm512_andnot_si512(a, a);
     }"
    TMP_COMPILER_SUPPORTS_AVX512_EXTENSIONS
  )
  if(TMP_COMPILER_SUPPORTS_AVX512_EXTENSIONS)
    message("--Current compiler supports avx512f extension.")
    if(VECTORIZATION_TYPE STREQUAL "avx512")
      set(HAS_AVX512 1)
      set(VECTORIZATION ON)
      set(VECTORIZATION_COMPILER_FLAGS "${CMAKE_REQUIRED_FLAGS}")
    endif()
  endif()
  cmake_pop_check_state()

  # ---[ Check if the compiler has FMA support.
  cmake_push_check_state(RESET)
  if(MSVC)
    set(CMAKE_REQUIRED_FLAGS "${VECTORIZATION_COMPILER_FLAGS} /D__FMA__")
  else()
    set(CMAKE_REQUIRED_FLAGS "${VECTORIZATION_COMPILER_FLAGS} -mfma")
  endif()
  check_cxx_source_compiles(
    "#if defined(_MSC_VER)
     #include <intrin.h>
     #else
     #include <immintrin.h>
     #endif

      int main() {
        __m128 a, b;
        a = _mm_set1_ps (1);
        b = _mm_set1_ps (1);
        a = _mm_fmadd_ps(a,b,b);
        return 0;
      }"
    TMP_COMPILER_SUPPORTS_FMA_EXTENSIONS
  )
  if(TMP_COMPILER_SUPPORTS_FMA_EXTENSIONS)
    message("--Current compiler supports fma extension.")
    set(VECTORIZATION_COMPILER_FLAGS "${CMAKE_REQUIRED_FLAGS}")
  endif()
  cmake_pop_check_state()

endif()

if(USE_NATIVE_ARCH)
  check_cxx_compiler_flag("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
  if(COMPILER_SUPPORTS_MARCH_NATIVE)
    string(APPEND CMAKE_C_FLAGS " -march=native")
    string(APPEND CMAKE_CXX_FLAGS " -march=native")
  else()
    message(WARNING "Your compiler does not support -march=native. Turn off this warning "
                    "by setting -DUSE_NATIVE_ARCH=OFF."
    )
  endif()
endif()
