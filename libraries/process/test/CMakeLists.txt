project(PROCESS_TEST)

file(GLOB SOURCES "*.cpp")
foreach( OBJ ${SOURCES} )
  get_filename_component(result "${OBJ}" NAME_WE)
  add_executable("process_${result}" "${OBJ}"  )
endforeach( OBJ )

