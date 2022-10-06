set (DB_DIR ${PROJECT_BINARY_DIR}/db.test)
file(REMOVE_RECURSE ${DB_DIR})
file(MAKE_DIRECTORY ${DB_DIR})
execute_process(COMMAND ${CMAKE_BINARY_DIR}/yggdrasill --db ${DB_DIR} --schema ${PROJECT_SOURCE_DIR}/script/testschema.info --workoutput ${PROJECT_SOURCE_DIR}/workoutput --output ${PROJECT_BINARY_DIR}/yggdrasillomits.test)
