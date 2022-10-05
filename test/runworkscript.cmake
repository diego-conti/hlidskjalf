function(cat IN_FILE OUT_FILE)
  file(READ ${IN_FILE} CONTENTS)
  file(APPEND ${OUT_FILE} "${CONTENTS}")
endfunction()

string (JOIN " " LOG COMMAND  "${CMAKE_TOP_BINARY_DIR}/hlidskjalf --script ${PROJECT_SOURCE_DIR}/script/workscript.m --workoutput ${PROJECT_BINARY_DIR}/output --computations ${PROJECT_SOURCE_DIR}/computations/test.comp  --schema ${PROJECT_SOURCE_DIR}/script/testschema.info --workload 1 --stdio")
message(${LOG})
execute_process(COMMAND "${CMAKE_TOP_BINARY_DIR}/hlidskjalf --script ${PROJECT_SOURCE_DIR}/script/workscript.m --workoutput ${PROJECT_BINARY_DIR}/output --computations ${PROJECT_SOURCE_DIR}/computations/test.comp  --schema ${PROJECT_SOURCE_DIR}/script/testschema.info --workload 1 --stdio")

set (UNSORTED_OUTPUT ${PROJECT_BINARY_DIR}/workscript.unsorted)
file(WRITE ${UNSORTED_OUTPUT} "")
file(GLOB output_files LIST_DIRECTORIES false "${PROJECT_BINARY_DIR}/output/*")
foreach(out_file ${output_files})	
    cat(${out_file} ${UNSORTED_OUTPUT})
endforeach()

execute_process(COMMAND sort ${UNSORTED_OUTPUT} -o ${PROJECT_BINARY_DIR}/workscript.test)
file(REMOVE ${UNSORTED_OUTPUT})