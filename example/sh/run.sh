build/hliðskjálf --computations example/computations.comp --schema example/schema.info --script example/magma/workscript.m --workoutput example/output
build/yggdrasill --db example/db --schema example/schema.info --workoutput example/output
magma -b PATH_TO_DB:=example/db example/magma/printentries.m
