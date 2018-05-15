project(ltstransform)

add_executable(ltstransform ltstransform.cpp)
target_link_libraries(ltstransform atermpp core data dparser lps process utilities)
install(TARGETS ltstransform DESTINATION bin)
