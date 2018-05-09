project(DATA_TEST)

file(GLOB SOURCES "*.cpp")
foreach( OBJ ${SOURCES} )
  get_filename_component(result "${OBJ}" NAME_WE)
  add_executable("data_${result}" "${OBJ}"  )
endforeach( OBJ )

