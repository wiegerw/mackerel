project(lts32lps)

add_executable(lts32lps lts32lps.cpp)
target_link_libraries(lts32lps atermpp core data dparser lps process utilities)
install(TARGETS lts32lps DESTINATION bin)
