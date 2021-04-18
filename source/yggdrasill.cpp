/***************************************************************************
	Copyright (C) 2021 by Diego Conti, diego.conti@unimib.it

	This file is part of hliðskjálf.
	Hliðskjálf is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*****************************************************************************/

#include <boost/filesystem.hpp>
#include <iostream>
#include <fstream>
#include "db.h"
#include "csvschema.h"
#include "csvreader.h"

using namespace std;

class OmittedEntries {
		map<const OmitRule*,int> omitted_by;
public:
	void increment(const OmitRule* rule) {
		++omitted_by[rule];
	}
	string to_string() const {
		string s="Omitted:\n";
		for (auto p : omitted_by) 
			s+=std::to_string(p.second) + ": "+p.first->to_string()+"\n";
		return s;
	}
	OmittedEntries& operator+=(const OmittedEntries& other) {
		for (auto p:other.omitted_by)
			omitted_by[p.first]+=p.second;
		return *this;
	}
};


class DBUpdater {
	Database& db;
public:
	DBUpdater(Database& db) : db{db} {}	
	OmittedEntries update_db(const CSV& csv, const CSVSchema& schema) {
		OmittedEntries omitted;
		for (auto& line : csv) {
			auto omit=CSVReader::omit_reason_or_null(line,schema);
			if (omit) omitted.increment(omit);
			else CSVReader::insert_entry(line,schema,db);
		}
		return omitted;
	}
};


OmittedEntries update_db_from_file(const fs::path& file, Database& db,const CSVSchema& schema) {
	auto csv=CSV::from_file(file.native(),CSV::balance_braces);
	auto updater=DBUpdater{db};	
	return updater.update_db(csv,schema);
}

void update_db(const fs::path& work_output_path, Database& db,const CSVSchema& schema) {
	set<string> ignored_algorithms;
	OmittedEntries omitted;
	for (auto& file : fs::directory_iterator(work_output_path)) 
		omitted+=update_db_from_file(file,db,schema);	
	cout<<omitted.to_string();
}


int main(int argv, char** argc) {
	po::options_description desc("Allowed options");
	desc.add_options()
    ("help", "produce help message")
    ("workoutput", po::value<string>(), "directory containing the output of the work script")
    ("db", po::value<string>()->default_value("db"), "directory containing the database")
    ("schema", po::value<string>(), "file containing the description of the CSV schema");
	;
	po::variables_map vm;
	po::store(po::parse_command_line(argv, argc, desc), vm);
	po::notify(vm);    	

	if (vm.count("help") || !vm.count("workoutput") || !vm.count("schema")) {
		cout<<desc<<endl;
		return 1;
	}

	string work_output_path=vm["workoutput"].as<string>();
	string db_path=vm["db"].as<string>();
	string schema_path=vm["schema"].as<string>();

	try {	
    pt::ptree tree;
    pt::read_info(schema_path,tree);
    auto schema=CSVSchema{tree};
		auto db=SimpleDatabase{fs::path{db_path},schema.no_secondary_input_columns()};
		update_db(work_output_path,db,schema);
		db.close();
	}
	catch (Exception& e) {
		cout<<e.what()<<endl;
		return 1;
	}
}
