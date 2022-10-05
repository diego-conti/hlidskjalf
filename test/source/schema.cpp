#include "csvschema.h"
#include <iostream>
#include "output.h"
using namespace std;

int main(int argv, char** argc) {
	pt::ptree tree;
	OutputStream os;
	if (argv<2) {
		cerr<<"usage: "<<argc[0]<<" schemafile [outfile]"<<endl;		
		return 1;
	}	
	pt::read_info(argc[1],tree);
	try {
		os<<CSVSchema(tree).to_string();
	}
	catch (const Exception& e) {
		std::cerr<<e.what()<<endl;
		return 1;
	}
	if (argv==3) 
		os.flush_to_file(argc[2]);
	else
		os.flush_to_cout();
	return 0;
}

