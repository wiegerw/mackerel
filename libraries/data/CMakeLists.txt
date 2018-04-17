project(data)

file(GLOB SOURCES "source/*.cpp" "source/detail/prover/*.cpp" "source/detail/rewrite/*.cpp")
add_library(data ${SOURCES})
target_link_libraries(core)
