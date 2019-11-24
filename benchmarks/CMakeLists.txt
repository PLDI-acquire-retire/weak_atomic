
function(add_benchmark TARGET)
  add_executable(${TARGET} ${TARGET}.cpp)
  target_link_libraries(${TARGET} arc)
  target_link_libraries(${TARGET} jss)
  target_compile_options(${TARGET} PRIVATE -Wall -Wextra -Wfatal-errors)
  target_compile_features(${TARGET} PRIVATE cxx_std_20)
endfunction()

add_benchmark(bench_trivial)
add_benchmark(bench_load_only)