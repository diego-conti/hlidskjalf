file(REMOVE_RECURSE db.test)
file(MAKE_DIRECTORY db.test)
execute_process(COMMAND ${CMAKE_BINARY_DIR}/yggdrasill --db db.test --schema ${PROJECT_SOURCE_DIR}/script/testschema.info --workoutput ${PROJECT_SOURCE_DIR}/workoutput --output ${CMAKE_BINARY_DIR}/yggdrasillomits.test)
