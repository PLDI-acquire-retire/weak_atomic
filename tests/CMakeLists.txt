
function(add_my_test TARGET)
  add_executable(${TARGET} ${TARGET}.cpp)
  target_link_libraries(${TARGET} arc)
  target_compile_options(${TARGET} PRIVATE -Wall -Wextra -Wfatal-errors -fsanitize=address,undefined)
  target_link_options(${TARGET} PRIVATE -fsanitize=address,undefined)
  add_test(${TARGET} ${TARGET})   
endfunction()

# List all test cases
add_my_test(test_weak_atomic)
add_my_test(test_weak_atomic_16byte)
add_my_test(test_weak_atomic_large)
add_my_test(test_weak_llsc)
add_my_test(test_sw_copy)

# testing pseudo-code from paper
add_my_test(test_resource)
add_my_test(test_stack)
add_my_test(test_ref_counting)

# Test just::threads if it is available
if(JUST_THREADS_AVAILABLE)
  add_my_test(test_jss_arc)
endif()

