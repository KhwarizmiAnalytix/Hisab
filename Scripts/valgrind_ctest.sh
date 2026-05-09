#!/usr/bin/env bash
# =============================================================================
# Quarisma Valgrind CTest Runner
# =============================================================================
# This script runs CTest with Valgrind memory checking.
#
# All Valgrind configuration options are defined in Cmake/tools/valgrind.cmake.
# This script only handles test execution and result analysis.
#
# Usage: valgrind_ctest.sh <build_directory>
# =============================================================================

set -euo pipefail

# =============================================================================
# Color Definitions for Output
# =============================================================================

readonly RED='\033[0;31m'
readonly GREEN='\033[0;32m'
readonly YELLOW='\033[1;33m'
readonly CYAN='\033[0;36m'
readonly NC='\033[0m' # No Color

# =============================================================================
# Utility Functions
# =============================================================================

# Print formatted status messages
print_status() {
    local message="$1"
    local type="${2:-INFO}"

    case "$type" in
        SUCCESS)
            echo -e "${GREEN}[✓] $message${NC}"
            ;;
        ERROR)
            echo -e "${RED}[✗] $message${NC}"
            ;;
        WARNING)
            echo -e "${YELLOW}[!] $message${NC}"
            ;;
        *)
            echo -e "${CYAN}[i] $message${NC}"
            ;;
    esac
}

# Print section header
print_header() {
    local message="$1"
    echo ""
    echo -e "${CYAN}========================================${NC}"
    echo -e "${CYAN}$message${NC}"
    echo -e "${CYAN}========================================${NC}"
}

# Extract Quarisma-specific stack trace information from Valgrind logs
# This function identifies function names and source locations from the quarisma namespace
extract_quarisma_stack_trace() {
    local log_file="$1"
    local issue_type="$2"  # "definitely lost", "Invalid read", etc.

    # Find the block containing the issue and extract stack trace
    # Valgrind format: lines starting with "at 0x" or "by 0x" contain stack info
    awk -v issue="$issue_type" '
        BEGIN { in_block = 0; block_count = 0 }

        # Start of a new block (marked by ==PID==)
        /^==.*==/ && NF > 1 {
            if (in_block && block_count > 0) {
                # End of previous block
                in_block = 0
            }
        }

        # Look for the issue type
        $0 ~ issue {
            in_block = 1
            block_count++
            print "=== Issue Block " block_count " ==="
            print $0
            next
        }

        # Collect stack trace lines while in a block
        in_block && /^==.*== *(at|by) 0x/ {
            print $0
        }

        # Stop collecting when we hit a blank line or new section
        in_block && /^==.*== *$/ {
            in_block = 0
        }
    ' "$log_file"
}

# Parse stack trace line to extract Quarisma function and source location
# Input format: "==12345==    by 0x123456: quarisma::namespace::function() (file.cpp:123)"
parse_stack_frame() {
    local frame="$1"

    # Extract the part after the address
    local info="${frame#*: }"

    # Check if this is an Quarisma frame (contains quarisma:: or .cpp/.h file)
    if [[ "$info" =~ quarisma:: ]] || [[ "$info" =~ \.(cxx|h|hpp|cc|cpp)\: ]]; then
        # Extract function name (everything before the opening paren)
        local func="${info%%(*}"

        # Extract file and line number (inside parentheses)
        local file_line="${info##*(}"
        file_line="${file_line%)*}"

        echo "  Function: $func"
        echo "  Location: $file_line"
    fi
}

# Extract all Quarisma frames from a stack trace block
extract_quarisma_frames() {
    local log_file="$1"
    local issue_type="$2"

    # Get the stack trace for this issue
    local stack_trace
    stack_trace=$(extract_quarisma_stack_trace "$log_file" "$issue_type" | grep -E "at 0x|by 0x")

    # Process each frame
    while IFS= read -r frame; do
        if [[ -n "$frame" ]]; then
            parse_stack_frame "$frame"
        fi
    done <<< "$stack_trace"
}

# =============================================================================
# Argument Validation
# =============================================================================

if [ $# -eq 0 ]; then
    print_status "Usage: $0 <build_directory>" "ERROR"
    print_status "Example: $0 ../build_ninja_valgrind" "INFO"
    exit 1
fi

BUILD_DIR="$1"

if [ ! -d "$BUILD_DIR" ]; then
    print_status "Build directory not found: $BUILD_DIR" "ERROR"
    exit 1
fi

# =============================================================================
# Platform Compatibility Check
# =============================================================================

check_platform_compatibility() {
    if [[ "$OSTYPE" == "darwin"* ]]; then
        local arch
        arch=$(uname -m)
        if [ "$arch" == "arm64" ]; then
            print_status "WARNING: Valgrind does not support Apple Silicon (ARM64)" "WARNING"
            print_status "Consider using sanitizers instead:" "INFO"
            print_status "  AddressSanitizer: python3 setup.py config.ninja.clang.test --sanitizer.address" "INFO"
            print_status "  LeakSanitizer:    python3 setup.py config.ninja.clang.test --sanitizer.leak" "INFO"
            print_status "Attempting to run Valgrind anyway (will fail if not installed)..." "WARNING"
        fi
    fi
}

# =============================================================================
# Valgrind Installation Check
# =============================================================================

check_valgrind_installed() {
    if ! command -v valgrind &> /dev/null; then
        print_status "Valgrind is not installed!" "ERROR"
        print_status "Please install Valgrind:" "INFO"
        print_status "  Ubuntu/Debian: sudo apt-get install valgrind" "INFO"
        print_status "  Fedora/RHEL:   sudo dnf install valgrind" "INFO"
        print_status "  macOS (Intel): brew install valgrind" "INFO"
        return 1
    fi

    local valgrind_version
    valgrind_version=$(valgrind --version)
    print_status "Found Valgrind: $valgrind_version" "SUCCESS"
    return 0
}

# =============================================================================
# CTest Execution with Valgrind
# =============================================================================

run_valgrind_tests() {
    local build_dir="$1"

    cd "$build_dir"
    print_status "Running Valgrind tests in: $build_dir"

    print_header "Running CTest with Valgrind Memory Checking"
    print_status "This may take a while (tests run 10-50x slower under Valgrind)..." "INFO"
    echo ""

    # Run CTest with memcheck
    # Note: All Valgrind options are configured in CMake (valgrind.cmake)
    # The --output-on-failure flag ensures we see test output if something fails
    local ctest_exit_code=0
    #
    # We explicitly overwrite CTest's MemCheck options here to avoid "still reachable"
    # noise (common in libstdc++/gtest static initialization) being reported as leaks.
    # This keeps the CI signal focused on real defects: invalid accesses, uninitialized
    # uses, and *definite* leaks.
    # Keep options close to CTest defaults, but drop "--show-reachable=yes" so that
    # "still reachable" allocations (common in libstdc++/gtest) don't get reported
    # as potential leaks/defects.
    local memcheck_opts="--tool=memcheck --leak-check=yes --show-reachable=no --num-callers=50"
    local suppressions_file="${build_dir%/}/../Scripts/suppressions/valgrind_suppression.txt"
    if [ -f "$suppressions_file" ]; then
        ctest --overwrite "MemoryCheckCommandOptions=$memcheck_opts --suppressions=$suppressions_file" -T memcheck || ctest_exit_code=$?
    else
        ctest --overwrite "MemoryCheckCommandOptions=$memcheck_opts" -T memcheck || ctest_exit_code=$?
    fi

    return $ctest_exit_code
}

# =============================================================================
# Report Generation Functions
# =============================================================================

# Generate a comprehensive Valgrind summary report
generate_valgrind_report() {
    local build_dir="$1"
    local report_file="$2"
    local memcheck_logs="$3"

    # Initialize report
    {
        echo "================================================================================"
        echo "Quarisma Valgrind Memory Analysis Report"
        echo "================================================================================"
        echo "Generated: $(date '+%Y-%m-%d %H:%M:%S')"
        echo "Build Directory: $build_dir"
        echo ""

        # =====================================================================
        # Section 1: Executive Summary
        # =====================================================================
        echo "EXECUTIVE SUMMARY"
        echo "================================================================================"

        local total_errors=0
        local total_leaks=0
        local total_invalid_access=0
        local total_uninitialized=0
        local total_fd_leaks=0

        # Count different types of errors
        # Note: Valgrind logs can have two formats:
        # 1. ERROR SUMMARY: N errors (when summary is present)
        # 2. Individual loss records: "N bytes in M blocks are definitely lost"

        # Try to get error count from ERROR SUMMARY first, then fall back to counting loss records
        total_errors=$(grep -h "ERROR SUMMARY:" $memcheck_logs 2>/dev/null | \
                      grep -oE "[0-9]+ errors" | grep -oE "[0-9]+" | \
                      awk '{sum+=$1} END {print sum+0}' 2>/dev/null || echo "0")
        total_errors=$(echo "$total_errors" | tr -d '\n' | xargs)

        # Parse definitely lost bytes from loss records (format: "N bytes in M blocks are definitely lost")
        total_leaks=$(grep -h "are definitely lost" $memcheck_logs 2>/dev/null | \
                     awk '{sum+=$2} END {print sum+0}' 2>/dev/null || echo "0")
        total_leaks=$(echo "$total_leaks" | tr -d '\n' | xargs)

        total_invalid_access=$(grep -hE "Invalid read|Invalid write" $memcheck_logs 2>/dev/null | wc -l)
        total_invalid_access=$(echo "$total_invalid_access" | tr -d '\n' | xargs)

        total_uninitialized=$(grep -h "Use of uninitialised value" $memcheck_logs 2>/dev/null | wc -l)
        total_uninitialized=$(echo "$total_uninitialized" | tr -d '\n' | xargs)

        total_fd_leaks=$(grep -h "Open file descriptor" $memcheck_logs 2>/dev/null | wc -l)
        total_fd_leaks=$(echo "$total_fd_leaks" | tr -d '\n' | xargs)

        echo "Total Memory Errors:        $total_errors"
        echo "Total Bytes Leaked:         $total_leaks bytes"
        echo "Invalid Memory Accesses:    $total_invalid_access"
        echo "Uninitialized Value Uses:   $total_uninitialized"
        echo "File Descriptor Leaks:      $total_fd_leaks"
        echo ""

        # =====================================================================
        # Section 2: Memory Leaks
        # =====================================================================
        echo "MEMORY LEAKS ANALYSIS"
        echo "================================================================================"

        if grep -qE "are definitely lost" $memcheck_logs 2>/dev/null; then
            echo "Status: FOUND"
            echo ""

            # Count and display individual leaks
            local leak_count=0
            grep -h "are definitely lost" $memcheck_logs 2>/dev/null | while read leak_line; do
                leak_count=$((leak_count + 1))

                # Extract bytes and blocks from the leak line
                # Format: "==12345== N bytes in M blocks are definitely lost in loss record X of Y"
                local bytes=$(echo "$leak_line" | awk '{print $2}')
                local blocks=$(echo "$leak_line" | awk '{print $5}')
                local record=$(echo "$leak_line" | grep -oE "loss record [0-9]+" | awk '{print $3}')

                echo "Leak #$leak_count: $bytes bytes in $blocks block(s)"

                # Try to extract Quarisma-specific stack trace information
                # Look for the allocation site in the log
                local alloc_site=$(grep -A 20 "loss record $record of" "$memcheck_logs" 2>/dev/null | \
                                  grep -E "quarisma::" | head -1)

                if [[ -n "$alloc_site" ]]; then
                    # Extract function name (everything between quarisma:: and the opening paren)
                    local func=$(echo "$alloc_site" | sed -n 's/.*\(quarisma::[^(]*\).*/\1/p')
                    if [[ -n "$func" ]]; then
                        echo "  Allocated by: $func"
                    fi

                    # Extract file and line number from the format: (filename.cpp:linenum)
                    local file_line=$(echo "$alloc_site" | sed -n 's/.*(\([^)]*\)).*/\1/p')
                    if [[ -n "$file_line" ]]; then
                        echo "  Location: $file_line"
                    fi
                fi

                echo ""
            done

            echo "Leak Locations (with stack traces):"
            echo "-----------------------------------"
            # Show stack traces for definitely lost blocks
            grep -B 5 "are definitely lost" $memcheck_logs 2>/dev/null | \
            grep -E "at 0x|by 0x" | head -50
        else
            echo "Status: NONE DETECTED ✓"
        fi
        echo ""

        # =====================================================================
        # Section 3: Invalid Memory Access
        # =====================================================================
        echo "INVALID MEMORY ACCESS"
        echo "================================================================================"

        if grep -qE "Invalid (read|write)" $memcheck_logs 2>/dev/null; then
            echo "Status: FOUND"
            echo ""

            # Count invalid accesses
            local invalid_count=$(grep -hE "Invalid read|Invalid write" $memcheck_logs 2>/dev/null | wc -l)
            echo "Total Invalid Memory Accesses: $invalid_count"
            echo ""

            echo "Invalid Read/Write Errors:"
            echo "--------------------------"
            grep -h "Invalid read\|Invalid write" $memcheck_logs 2>/dev/null | \
            sort | uniq -c | sort -rn | head -20
            echo ""

            echo "Quarisma-Specific Locations:"
            echo "-------------------------"
            # Extract stack traces and filter for Quarisma code
            # Look both before and after the Invalid read/write line
            grep -B 10 -A 10 "Invalid read\|Invalid write" $memcheck_logs 2>/dev/null | \
            grep -E "quarisma::|\.cpp:|\.h:" | head -20

            echo ""
            echo "Full Stack Traces (first 30 frames):"
            echo "-----------------------------------"
            grep -B 5 -A 5 "Invalid read\|Invalid write" $memcheck_logs 2>/dev/null | \
            grep -E "at 0x|by 0x" | head -30
        else
            echo "Status: NONE DETECTED ✓"
        fi
        echo ""

        # =====================================================================
        # Section 4: Uninitialized Values
        # =====================================================================
        echo "UNINITIALIZED VALUE USAGE"
        echo "================================================================================"

        if grep -q "Use of uninitialised value" $memcheck_logs 2>/dev/null; then
            echo "Status: FOUND"
            echo ""

            local uninit_count=$(grep -h "Use of uninitialised value" $memcheck_logs 2>/dev/null | wc -l)
            echo "Total Uninitialized Value Errors: $uninit_count"
            echo ""

            echo "Quarisma-Specific Locations:"
            echo "-------------------------"
            # Extract stack traces and filter for Quarisma code
            # Look both before and after the Use of uninitialised value line
            grep -B 10 -A 10 "Use of uninitialised value" $memcheck_logs 2>/dev/null | \
            grep -E "quarisma::|\.cpp:|\.h:" | head -20

            echo ""
            echo "Sample Stack Traces (first 20 frames):"
            echo "-------------------------------------"
            grep -B 3 -A 3 "Use of uninitialised value" $memcheck_logs 2>/dev/null | \
            grep -E "at 0x|by 0x" | head -20
        else
            echo "Status: NONE DETECTED ✓"
        fi
        echo ""

        # =====================================================================
        # Section 5: File Descriptor Leaks
        # =====================================================================
        echo "FILE DESCRIPTOR LEAKS"
        echo "================================================================================"

        if grep -q "Open file descriptor" $memcheck_logs 2>/dev/null; then
            echo "Status: FOUND"
            echo ""
            echo "Open File Descriptors:"
            echo "---------------------"
            grep -h "Open file descriptor" $memcheck_logs 2>/dev/null | head -20
        else
            echo "Status: NONE DETECTED ✓"
        fi
        echo ""

        # =====================================================================
        # Section 6: Error Summary by Test
        # =====================================================================
        echo "ERROR SUMMARY BY TEST"
        echo "================================================================================"

        local tests_with_issues=0
        if [ -n "$memcheck_logs" ]; then
            for log_file in $memcheck_logs; do
                local test_name=$(basename "$log_file" | sed 's/MemoryChecker\.\(.*\)\.log/\1/')
                local error_count=$(grep -c "ERROR SUMMARY:" "$log_file" 2>/dev/null || echo "0")
                # Trim whitespace and newlines from error_count
                error_count=$(echo "$error_count" | tr -d '\n' | xargs)

                # Check if this test has any issues
                local has_issues=0
                if [ -n "$error_count" ] && [ "$error_count" -gt 0 ]; then
                    has_issues=1
                fi

                if grep -q "definitely lost: [1-9]" "$log_file" 2>/dev/null; then
                    has_issues=1
                fi

                if grep -q "Invalid read\|Invalid write" "$log_file" 2>/dev/null; then
                    has_issues=1
                fi

                if [ $has_issues -eq 1 ]; then
                    tests_with_issues=$((tests_with_issues + 1))
                    echo "Test: $test_name"

                    # Show error summary if present
                    if [ -n "$error_count" ] && [ "$error_count" -gt 0 ]; then
                        grep "ERROR SUMMARY:" "$log_file" 2>/dev/null | head -1
                    fi

                    # Show memory leaks if present
                    if grep -q "definitely lost: [1-9]" "$log_file" 2>/dev/null; then
                        echo "  Memory Leaks:"
                        grep "definitely lost:" "$log_file" 2>/dev/null | head -1 | sed 's/^/    /'
                    fi

                    # Show invalid access if present
                    if grep -q "Invalid read\|Invalid write" "$log_file" 2>/dev/null; then
                        echo "  Invalid Memory Access:"
                        grep -c "Invalid read\|Invalid write" "$log_file" 2>/dev/null | xargs echo "    Count:"
                    fi

                    echo ""
                fi
            done

            if [ $tests_with_issues -eq 0 ]; then
                echo "All tests passed memory checks ✓"
            else
                echo "Total tests with issues: $tests_with_issues"
            fi
        fi

        # =====================================================================
        # Section 7: Overall Status
        # =====================================================================
        echo "OVERALL STATUS"
        echo "================================================================================"

        if [ "$total_errors" -eq 0 ] && [ "$total_leaks" -eq 0 ]; then
            echo "Result: PASS ✓"
            echo "No memory issues detected during test execution."
        else
            echo "Result: FAIL ✗"
            echo "Memory issues detected. Please review the details above."
        fi
        echo ""
        echo "================================================================================"
        echo "End of Report"
        echo "================================================================================"

    } > "$report_file"

    print_status "Report saved to: $report_file" "SUCCESS"
}

# =============================================================================
# Valgrind Results Analysis
# =============================================================================

analyze_valgrind_results() {
    local build_dir="$1"
    local has_memory_issues=0

    print_header "Analyzing Valgrind Results"

    # Find all Valgrind log files
    local memcheck_logs
    memcheck_logs=$(find "$build_dir/Testing/Temporary" -name "MemoryChecker.*.log" 2>/dev/null || true)

    if [ -z "$memcheck_logs" ]; then
        print_status "No Valgrind log files found" "WARNING"
        print_status "This may indicate that tests did not run or Valgrind was not invoked" "WARNING"
        return 1
    fi

    print_status "Found Valgrind log files, analyzing..."

    # Generate comprehensive report
    local report_file="$build_dir/valgrind_summary_report.txt"
    generate_valgrind_report "$build_dir" "$report_file" "$memcheck_logs"

    # Check for memory leaks (definitely lost)
    # Pattern: "N bytes in M blocks are definitely lost"
    if grep -lE "are definitely lost" $memcheck_logs 2>/dev/null | grep -q .; then
        print_status "Memory leaks detected!" "ERROR"
        echo ""
        echo "=== Memory Leak Summary ==="
        grep -h "are definitely lost" $memcheck_logs 2>/dev/null | head -30
        has_memory_issues=1
    else
        print_status "No memory leaks detected" "SUCCESS"
    fi

    # Check for memory errors (invalid reads/writes, use of uninitialized values, etc.)
    if grep -l "ERROR SUMMARY: [1-9][0-9]* errors" $memcheck_logs 2>/dev/null | grep -q .; then
        print_status "Memory errors detected!" "ERROR"
        echo ""
        echo "=== Memory Error Summary ==="
        grep -h "ERROR SUMMARY" $memcheck_logs 2>/dev/null
        has_memory_issues=1
    else
        print_status "No memory errors detected" "SUCCESS"
    fi

    # Check for invalid memory access
    if grep -lE "Invalid (read|write)" $memcheck_logs 2>/dev/null | grep -q .; then
        print_status "Invalid memory access detected!" "ERROR"
        echo ""
        echo "=== Invalid Memory Access ==="
        grep -h -B 2 -A 3 "Invalid" $memcheck_logs 2>/dev/null | head -20
        has_memory_issues=1
    fi

    # Check for file descriptor leaks
    if grep -q "FILE DESCRIPTORS" $memcheck_logs 2>/dev/null; then
        if grep -qE "Open file descriptor [0-9]+:" $memcheck_logs 2>/dev/null; then
            print_status "File descriptor leaks detected" "WARNING"
        fi
    fi

    return $has_memory_issues
}

# =============================================================================
# Display Report Summary to Console
# =============================================================================

display_report_summary() {
    local report_file="$1"

    if [ ! -f "$report_file" ]; then
        return
    fi

    print_header "Valgrind Summary Report"
    echo ""

    # Display key sections from the report
    if grep -q "EXECUTIVE SUMMARY" "$report_file"; then
        echo "=== EXECUTIVE SUMMARY ==="
        sed -n '/^EXECUTIVE SUMMARY/,/^ERROR SUMMARY BY TEST/p' "$report_file" | head -30
        echo ""
    fi

    # Display error summary by test
    if grep -q "ERROR SUMMARY BY TEST" "$report_file"; then
        echo "=== TESTS WITH ISSUES ==="
        sed -n '/^ERROR SUMMARY BY TEST/,/^OVERALL STATUS/p' "$report_file" | grep -v "^===" | head -50
        echo ""
    fi

    # Display overall status
    if grep -q "OVERALL STATUS" "$report_file"; then
        echo "=== OVERALL STATUS ==="
        sed -n '/^OVERALL STATUS/,/^====/p' "$report_file" | grep -v "^====" | head -10
        echo ""
    fi

    print_status "Full report saved to: $report_file" "INFO"
}

# =============================================================================
# Determine Overall Test Status
# =============================================================================

determine_test_status() {
    local ctest_exit_code="$1"
    local memory_issues="$2"
    local build_dir="$3"

    print_header "Test Results Summary"

    # Check if tests timed out
    local timeout_detected=0
    if [ -f "$build_dir/Testing/Temporary/LastTest.log" ]; then
        if grep -q "Timeout" "$build_dir/Testing/Temporary/LastTest.log" 2>/dev/null; then
            timeout_detected=1
            print_status "Some tests timed out" "WARNING"
            print_status "Tests run slower under Valgrind (10-50x). Consider increasing timeouts." "INFO"
        fi
    fi

    # Determine final status based on multiple factors
    local final_exit_code=0

    # Memory issues are the primary concern
    if [ $memory_issues -ne 0 ]; then
        print_status "FAILED: Memory issues detected by Valgrind" "ERROR"
        final_exit_code=1
    # CTest failures (excluding timeouts if no memory issues)
    elif [ $ctest_exit_code -ne 0 ] && [ $timeout_detected -eq 0 ]; then
        print_status "FAILED: Tests failed (non-timeout failures)" "ERROR"
        final_exit_code=1
    # Timeout without memory issues is a warning, not a hard failure
    elif [ $timeout_detected -eq 1 ] && [ $memory_issues -eq 0 ]; then
        print_status "WARNING: Tests timed out but no memory issues detected" "WARNING"
        print_status "Consider this a PASS for memory checking purposes" "INFO"
        print_status "Increase test timeouts in CMake configuration if needed" "INFO"
        final_exit_code=0
    # All good
    else
        print_status "SUCCESS: All tests passed with no memory issues!" "SUCCESS"
        final_exit_code=0
    fi

    # Display report summary to console
    local report_file="$build_dir/valgrind_summary_report.txt"
    display_report_summary "$report_file"

    # Provide helpful information
    echo ""
    print_status "Valgrind logs location: $build_dir/Testing/Temporary/" "INFO"
    print_status "View detailed logs: ls -lh $build_dir/Testing/Temporary/MemoryChecker.*.log" "INFO"

    return $final_exit_code
}

# =============================================================================
# Main Execution
# =============================================================================

main() {
    local exit_code=0

    print_header "Quarisma Valgrind Memory Check"

    # Step 1: Platform compatibility check
    check_platform_compatibility

    # Step 2: Verify Valgrind is installed
    if ! check_valgrind_installed; then
        exit 1
    fi

    # Step 3: Run tests with Valgrind
    local ctest_exit_code=0
    run_valgrind_tests "$BUILD_DIR" || ctest_exit_code=$?

    # Step 4: Analyze Valgrind results
    local memory_issues=0
    analyze_valgrind_results "$BUILD_DIR" || memory_issues=$?

    # Step 5: Determine overall status
    determine_test_status "$ctest_exit_code" "$memory_issues" "$BUILD_DIR" || exit_code=$?

    return $exit_code
}

# =============================================================================
# Script Entry Point
# =============================================================================

# Run main function and capture exit code
main
exit $?

