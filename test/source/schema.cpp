#include "csvschema.h"
#include <iostream>
int main(int argv, char** argc) {
	pt::ptree tree;
	pt::read_info(argc[1],tree);
	try {
		cout<<CSVSchema(tree).to_string();
	}
	catch (const Exception& e) {
		std::cerr<<e.what()<<endl;
		return 1;
	}
	return 0;
}

