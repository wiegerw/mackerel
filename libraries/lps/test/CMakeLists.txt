project(LPS_TEST)

file(GLOB SOURCES "*.cpp")
foreach( OBJ ${SOURCES} )
  get_filename_component(result "${OBJ}" NAME_WE)
  add_executable("lps_${result}" "${OBJ}"  )
endforeach( OBJ )

