#include "csvschema.h"
#include "db.h"
#include "csvreader.h"
#include "output.h"
#include <iostream>

using namespace std;

OutputStream os;

class MockDatabase : public Database {
public:
	void insert (int primary_id, const vector<string>& secondary_id, const vector<string>& data) override {
		os<<primary_id;
		for (auto& x: secondary_id) os<<";"<<x;
		for (auto& x: data) os<<";"<<x;
		os<<endl;
	}
	void close() override {};
};


int main(int argv, char** argc) {
	if (argv<2) {
		cerr<<"usage: "<<argc[0]<<" schemafile workfile [outfile]"<<endl;		
		return 1;
	}	
	pt::ptree tree;
	pt::read_info(argc[1],tree);
	try {
		auto schema=CSVSchema(tree);
		auto csv=CSV::from_file(argc[2]);
		MockDatabase mock_db;
		for (auto& line: csv) {
			auto omit=CSVReader::omit_reason_or_null(line,schema);
			if (omit) os<<"omitted by rule: "<<omit->to_string()<<endl;
			else CSVReader::insert_entry(line,schema,mock_db);
		}
	}
	catch (const Exception& e) {
		cerr<<e.what()<<endl;
		return 1;
	}
	if (argv==4) 
		os.flush_to_file(argc[3]);
	else
		os.flush_to_cout();
	return 0;
}

