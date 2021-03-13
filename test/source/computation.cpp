#include "computation.h"
#include "csvreader.h"

int main(int argv, char** argc) {
	pt::ptree tree;
	pt::read_info(argc[1],tree);
	auto schema=CSVSchema(tree);
	ifstream f(argc[2]);
	while (has_data_after_skipping_empty_lines(f)) {
		auto line = get_line_with_balanced_curly_braces(f);
		Computation c=CSVReader::extract_computation(line,schema);
		cout<<c.to_string()<<endl;
	}
	return 0;
}

