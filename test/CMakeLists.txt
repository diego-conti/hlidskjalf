cmake_minimum_required(VERSION 3.10)
project(TestHlidskjalf)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
add_compile_options(-g -O0)

enable_testing()
add_executable(schema source/schema.cpp)
add_test(NAME prepareschema COMMAND ${CMAKE_CURRENT_BINARY_DIR}/schema ${PROJECT_SOURCE_DIR}/script/testschema.info ${PROJECT_BINARY_DIR}/testschema.test)
set_tests_properties(prepareschema PROPERTIES FIXTURES_SETUP runworkscript)
add_executable(csv source/csv.cpp)
add_test(NAME preparecsv COMMAND ${CMAKE_CURRENT_BINARY_DIR}/csv ${PROJECT_SOURCE_DIR}/script/testschema.info ${PROJECT_SOURCE_DIR}/workoutput/test.work ${PROJECT_BINARY_DIR}/testcsv.test)
set_tests_properties(preparecsv PROPERTIES FIXTURES_SETUP runworkscript)
add_executable(computation source/computation.cpp)
add_test(NAME preparecomputation COMMAND ${CMAKE_CURRENT_BINARY_DIR}/computation ${PROJECT_SOURCE_DIR}/script/testschema.info ${PROJECT_SOURCE_DIR}/workoutput/test.work ${PROJECT_BINARY_DIR}/testcomputation.test)
set_tests_properties(preparecomputation PROPERTIES FIXTURES_SETUP runworkscript)
add_test(NAME prepareworkscript COMMAND ${CMAKE_COMMAND} -DWORKSCRIPT=workscript -DHLIDSKJALF_FLAGS="" -DCMAKE_TOP_BINARY_DIR=${CMAKE_BINARY_DIR} -DPROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR} -DPROJECT_BINARY_DIR=${PROJECT_BINARY_DIR} -P ${CMAKE_CURRENT_SOURCE_DIR}/runworkscript.cmake )
set_tests_properties(prepareworkscript PROPERTIES FIXTURES_SETUP runworkscript)
add_test(NAME preparetimeoutworkscript COMMAND ${CMAKE_COMMAND} -DWORKSCRIPT=timeoutworkscript [[-DHLIDSKJALF_FLAGS=--base-timeout 2 --memory 2048 --total-memory 4]]
	-DCMAKE_TOP_BINARY_DIR=${CMAKE_BINARY_DIR} -DPROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR} -DPROJECT_BINARY_DIR=${PROJECT_BINARY_DIR} -P ${CMAKE_CURRENT_SOURCE_DIR}/runworkscript.cmake 
)
set_tests_properties(preparetimeoutworkscript PROPERTIES FIXTURES_SETUP runworkscript)

file(GLOB ok_files LIST_DIRECTORIES false "${PROJECT_SOURCE_DIR}/*.ok")
foreach(ok_file ${ok_files})	
	cmake_path(GET ok_file STEM file_stem)		
	add_test(NAME ${file_stem} COMMAND ${CMAKE_COMMAND} -E compare_files ${PROJECT_BINARY_DIR}/${file_stem}.test ${ok_file})
	set_tests_properties(${file_stem} PROPERTIES FIXTURES_REQUIRED runworkscript)
endforeach()

#override the dependency for test yggdrasillomits, which really depends on the invocation of yggdrasill
set_tests_properties(yggdrasillomits PROPERTIES FIXTURES_REQUIRED runyggdrasill)

add_test(NAME preparedb COMMAND ${CMAKE_COMMAND} -DPROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR} -DPROJECT_BINARY_DIR=${PROJECT_BINARY_DIR} -DCMAKE_TOP_BINARY_DIR=${CMAKE_BINARY_DIR}
 -P ${CMAKE_CURRENT_SOURCE_DIR}/runyggdrasill.cmake
)
set_tests_properties(preparedb PROPERTIES FIXTURES_SETUP runyggdrasill)
file(GLOB db_files "${PROJECT_SOURCE_DIR}/db.ok/*")
foreach(db_file ${db_files})	
	cmake_path(GET db_file FILENAME filename)
	add_test(NAME "db${filename}" COMMAND ${CMAKE_COMMAND} -E compare_files ${PROJECT_BINARY_DIR}/db.test/${filename} ${db_file})
	set_tests_properties(db${filename} PROPERTIES FIXTURES_REQUIRED runyggdrasill)
endforeach()
