FLAGS="--std=c++17 -Isource -g -Wreorder -Wsign-promo -Wchar-subscripts -Winit-self -Wparentheses -Wreturn-type -Wswitch -Wtrigraphs -Wextra -Wno-sign-compare -Wno-narrowing -Wno-attributes"
echo compiling with CXXFLAGS=$CXXFLAGS
mkdir bin -p
g++ $FLAGS $CXXFLAGS source/yggdrasill.cpp -lboost_filesystem -lboost_program_options -o bin/yggdrasill
g++ $FLAGS $CXXFLAGS source/hliðskjálf.cpp -lboost_filesystem -lboost_program_options -lncurses -pthread -o bin/hliðskjálf

