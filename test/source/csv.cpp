#include "csvschema.h"
#include "db.h"
#include "csvreader.h"

class MockDatabase : public Database {
public:
	void insert (int primary_id, const vector<string>& secondary_id, const vector<string>& data) override {
		cout<<primary_id;
		for (auto& x: secondary_id) cout<<";"<<x;
		for (auto& x: data) cout<<";"<<x;
		cout<<endl;
	}
	void close() override {};
};


int main(int argv, char** argc) {
	pt::ptree tree;
	pt::read_info(argc[1],tree);
	try {
		auto schema=CSVSchema(tree);
		auto csv=CSV::from_file(argc[2]);
		MockDatabase mock_db;
		for (auto& line: csv) {
			auto omit=CSVReader::omit_reason_or_null(line,schema);
			if (omit) cout<<"omitted by rule: "<<omit->to_string()<<endl;
			else CSVReader::insert_entry(line,schema,mock_db);
		}
	}
	catch (const Exception& e) {
		cout<<e.what()<<endl;
		return 1;
	}
	return 0;
}

