project(CORE_TEST)

file(GLOB SOURCES "*.cpp")
foreach( OBJ ${SOURCES} )
  get_filename_component(result "${OBJ}" NAME_WE)
  add_executable("core_${result}" "${OBJ}"  )
endforeach( OBJ )

