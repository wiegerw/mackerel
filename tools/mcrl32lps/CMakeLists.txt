project(mcrl32lps)

add_executable(mcrl32lps mcrl32lps.cpp)
target_link_libraries(mcrl32lps atermpp core data dparser lps process utilities)
install(TARGETS mcrl32lps DESTINATION bin)
