execute_process(COMMAND bin/hliðskjálf --script test/script/workscript.m --workoutput test/output --computations test/computations/test.comp  --schema test/script/testschema.info --workload 1 --stdio >/dev/null)
execute_process(COMMAND cat test/output/* | sort > test/workscript.test)
