#include "computation.h"
#include "csvreader.h"
#include "output.h"

using namespace std;

int main(int argv, char** argc) {
	OutputStream os;
	pt::ptree tree;
	pt::read_info(argc[1],tree);
	auto schema=CSVSchema(tree);
	if (argv<3) {
		cerr<<"usage: "<<argc[0]<<" schemafile workfile [outfile]"<<endl;		
		return 1;
	}
	ifstream f(argc[2]);
	while (has_data_after_skipping_empty_lines(f)) {
		auto line = get_line_with_balanced_curly_braces(f);
		Computation c=CSVReader::extract_computation(line,schema);
		os<<c.to_string()<<endl;
	}
	if (argv==4) 
		os.flush_to_file(argc[3]);
	else
		os.flush_to_cout();
	return 0;
}

