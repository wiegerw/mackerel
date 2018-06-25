project(lps32lts)

add_executable(lps32lts lps32lts.cpp)
target_link_libraries(lps32lts atermpp core data dparser lps process utilities)
install(TARGETS lps32lts DESTINATION bin)
