FLAGS="--std=c++17 -Isource -g"

g++ $FLAGS $CXXFLAGS test/source/schema.cpp -lboost_filesystem -o test/bin/schema
test/bin/schema test/script/testschema.info >test/testschema.test

g++ $FLAGS $CXXFLAGS test/source/csv.cpp -lboost_filesystem -o test/bin/csv
test/bin/csv test/script/testschema.info test/workoutput/test.work >test/testcsv.test

g++ $FLAGS $CXXFLAGS test/source/computation.cpp -lboost_filesystem -o test/bin/computation
test/bin/computation test/script/testschema.info test/workoutput/test.work >test/testcomputation.test

mkdir -p test/db
rm -f test/db/*

bin/yggdrasill --db test/db --schema test/script/testschema.info --workoutput test/workoutput >test/yggdrasillomits.test
for i in $(ls test/db/*); do
	mv $i test/db`basename $i`.test
done

mkdir -p test/output
rm -f test/output/*

bin/hliðskjálf --script test/script/workscript.m --workoutput test/output --computations test/computations/test.comp  --schema test/script/testschema.info --workload 1 >/dev/null
cat test/output/* | sort > test/workscript.test

for out in $(ls test/*.ok); do
	file_to_check=`basename $out .ok`
	diff test/$file_to_check.test test/$file_to_check.ok
	if [ $? -eq 0 ]; then echo $file_to_check OK;
          else echo $file_to_check ERROR; fi
done

