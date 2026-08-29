---
type: "always_apply"
---

### Test Scope
- Test happy paths and success cases
- Explicitly test boundary conditions and edge cases
- Test error handling and failure scenarios (return value validation)
- Test null pointers, empty collections, and invalid inputs
- Verify state changes and side effects
- use exclusively tests macros XSIGMATEST

✅ **Correct Example**
```cpp
XSIGMATEST(my_class_test, handles_valid_input) {
  my_class obj;
  EXPECT_TRUE(obj.do_something());
}

XSIGMATEST(my_class_test, handles_invalid_input) {
  my_class obj;
  EXPECT_FALSE(obj.do_something_with(-1));
}

XSIGMATEST(my_class_test, handles_null_pointer) {
  my_class obj;
  EXPECT_FALSE(obj.process(nullptr));
}
```

---