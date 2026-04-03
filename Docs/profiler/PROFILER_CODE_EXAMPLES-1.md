# PyTorch-Style Profiler: Code Implementation Examples

## 1. Self CPU Time Calculation

```cpp
// In profiler_report.cpp
double calculate_self_cpu_time(const profiler_scope_data* scope) const {
    if (scope == nullptr) return 0.0;
    
    double total_time = scope->get_duration_ms();
    double children_time = 0.0;
    
    for (const auto& child : scope->children_) {
        children_time += child->get_duration_ms();
    }
    
    return std::max(0.0, total_time - children_time);
}
```

## 2. Percentage Calculation

```cpp
// In profiler_report.cpp
double calculate_cpu_percentage(
    double operation_time_ms, 
    double session_total_ms) const {
    if (session_total_ms <= 0.0) return 0.0;
    return (operation_time_ms / session_total_ms) * 100.0;
}
```

## 3. Aggregation Structure

```cpp
// In profiler_report.h
struct aggregated_operation_stats {
    std::string name;
    double total_cpu_time_ms = 0.0;
    double self_cpu_time_ms = 0.0;
    size_t call_count = 0;
    
    double get_cpu_percentage(double session_total) const {
        return (total_cpu_time_ms / session_total) * 100.0;
    }
    
    double get_self_cpu_percentage(double session_total) const {
        return (self_cpu_time_ms / session_total) * 100.0;
    }
    
    double get_avg_time_ms() const {
        return call_count > 0 ? total_cpu_time_ms / call_count : 0.0;
    }
};
```

## 4. Aggregation Logic

```cpp
// In profiler_report.cpp
std::vector<aggregated_operation_stats> aggregate_by_operation(
    const profiler_scope_data* root) const {
    
    std::map<std::string, aggregated_operation_stats> aggregated;
    
    std::function<void(const profiler_scope_data*)> traverse = 
        [&](const profiler_scope_data* scope) {
            if (scope == nullptr) return;
            
            auto& stats = aggregated[scope->name_];
            stats.name = scope->name_;
            stats.total_cpu_time_ms += scope->get_duration_ms();
            stats.self_cpu_time_ms += calculate_self_cpu_time(scope);
            stats.call_count++;
            
            for (const auto& child : scope->children_) {
                traverse(child.get());
            }
        };
    
    traverse(root);
    
    std::vector<aggregated_operation_stats> result;
    for (auto& [name, stats] : aggregated) {
        result.push_back(std::move(stats));
    }
    
    // Sort by total time descending
    std::sort(result.begin(), result.end(),
        [](const auto& a, const auto& b) {
            return a.total_cpu_time_ms > b.total_cpu_time_ms;
        });
    
    return result;
}
```

## 5. Table Formatting

```cpp
// In profiler_report.cpp
std::string format_pytorch_table(
    const std::vector<aggregated_operation_stats>& stats,
    double session_total_ms) const {
    
    std::stringstream ss;
    
    // Header
    ss << std::setw(20) << std::left << "Name"
       << std::setw(12) << std::right << "Self CPU %"
       << std::setw(12) << std::right << "Self CPU"
       << std::setw(12) << std::right << "CPU total %"
       << std::setw(12) << std::right << "CPU total"
       << std::setw(12) << std::right << "CPU time avg"
       << std::setw(12) << std::right << "# of Calls"
       << "\n";
    
    // Separator
    ss << std::string(92, '-') << "\n";
    
    // Data rows
    for (const auto& stat : stats) {
        ss << std::setw(20) << std::left << stat.name
           << std::setw(12) << std::right 
               << std::fixed << std::setprecision(2) 
               << stat.get_self_cpu_percentage(session_total_ms) << "%"
           << std::setw(12) << std::right 
               << std::fixed << std::setprecision(3) 
               << stat.self_cpu_time_ms << "ms"
           << std::setw(12) << std::right 
               << std::fixed << std::setprecision(2) 
               << stat.get_cpu_percentage(session_total_ms) << "%"
           << std::setw(12) << std::right 
               << std::fixed << std::setprecision(3) 
               << stat.total_cpu_time_ms << "ms"
           << std::setw(12) << std::right 
               << std::fixed << std::setprecision(3) 
               << stat.get_avg_time_ms() << "ms"
           << std::setw(12) << std::right 
               << stat.call_count
           << "\n";
    }
    
    return ss.str();
}
```

## 6. Integration into Report Generation

```cpp
// In profiler_report.cpp
std::string profiler_report::generate_pytorch_table() const {
    auto const* root = session_.get_root_scope();
    if (root == nullptr) {
        return "No profiling data available.\n";
    }
    
    double session_total_ms = 
        std::chrono::duration_cast<std::chrono::milliseconds>(
            session_.session_end_time() - session_.session_start_time()
        ).count();
    
    auto aggregated = aggregate_by_operation(root);
    return format_pytorch_table(aggregated, session_total_ms);
}
```

## 7. Usage Example

```cpp
// User code
auto session = profiler_session_builder()
    .with_timing(true)
    .with_hierarchical_profiling(true)
    .build();

session->start();

{
    PROFILER_PROFILE_SCOPE("model_inference");
    // ... model code ...
}

session->stop();

auto report = session->generate_report();
std::cout << report->generate_pytorch_table();
```

## 8. Unit Test Example

```cpp
QUARISMATEST(profiler_report_test, pytorch_table_format) {
    // Create test data
    auto session = profiler_session_builder()
        .with_timing(true)
        .with_hierarchical_profiling(true)
        .build();
    
    session->start();
    
    {
        PROFILER_PROFILE_SCOPE("operation_a");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    {
        PROFILER_PROFILE_SCOPE("operation_b");
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    
    session->stop();
    
    auto report = session->generate_report();
    auto table = report->generate_pytorch_table();
    
    // Verify table contains expected columns
    EXPECT_TRUE(table.find("Self CPU %") != std::string::npos);
    EXPECT_TRUE(table.find("CPU total %") != std::string::npos);
    EXPECT_TRUE(table.find("# of Calls") != std::string::npos);
    
    // Verify operations appear in table
    EXPECT_TRUE(table.find("operation_a") != std::string::npos);
    EXPECT_TRUE(table.find("operation_b") != std::string::npos);
}
```

