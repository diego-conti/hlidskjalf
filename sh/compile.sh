FLAGS="--std=c++17 -Isource -g"
echo compiling with CXXFLAGS=$CXXFLAGS
mkdir bin -p
g++ $FLAGS $CXXFLAGS source/yggdrasill.cpp -lboost_filesystem -lboost_program_options -o bin/yggdrasill
g++ $FLAGS $CXXFLAGS source/hliðskjálf.cpp -lboost_filesystem -lboost_program_options -lncurses -pthread -o bin/hliðskjálf

