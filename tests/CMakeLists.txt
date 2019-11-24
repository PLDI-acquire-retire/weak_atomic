
function(add_my_test TARGET)
  add_executable(${TARGET} ${TARGET}.cpp)
  target_link_libraries(${TARGET} arc)
  target_compile_options(${TARGET} PRIVATE -Wall -Wextra -Wfatal-errors -fsanitize=address,undefined)
  target_link_options(${TARGET} PRIVATE -fsanitize=address,undefined)
  add_test(${TARGET} ${TARGET})   
endfunction()

add_my_test(test_weak_atomic)
add_my_test(test_weak_atomic_16byte)
add_my_test(test_weak_atomic_large)
add_my_test(test_weak_llsc)
add_my_test(test_sw_copy)
