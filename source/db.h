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

#ifndef DB_H
#define DB_H

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include "stdincludes.h"
#include "exception.h"
#include "csv.h"

namespace po = boost::program_options;
namespace fs = boost::filesystem;


class DbException : public Exception {
	string error;
public:
	DbException(string error) : error{error} {}
	string what() const noexcept override {
		return "DB error: "+error;
	}
};

class Database {
public:
	Database()=default;
	virtual void insert (int primary_id, const vector<string>& secondary_id, const vector<string>& data)=0;
	virtual void close()=0;
	virtual ~Database()=default;
};

struct FieldsInDB {
	vector<string> fields;
	FieldsInDB()=default;
	template<typename V> FieldsInDB(V&& v) : fields{std::forward<V>(v)} {}
	void to_stream(ostream& os, char sep=';') const {
		if (fields.empty()) return;
		auto i=fields.begin();
		os<<*i;
		while (++i!=fields.end()) os<<sep<<*i;
	}
	explicit operator CSVLine() const &  {return CSVLine::from_vector(fields);}
	explicit operator CSVLine() && {return CSVLine::from_vector(move(fields));}
};

struct DBKey : FieldsInDB {
	using FieldsInDB::FieldsInDB;
	bool operator<(const DBKey& other) const {
		assert(fields.size()==other.fields.size());
		for (int i=0;i<fields.size();++i) {
			auto cmp=fields[i].compare(other.fields[i]);
			if (cmp<0) return true;
			else if (cmp>0) return false;
		}
		return false;
	}
};

pair<DBKey,FieldsInDB> db_line_to_fields(const CSVLine& csv_line, int no_of_secondary_inputs) {
	pair<DBKey,FieldsInDB> result;
	auto& secondary_inputs=result.first.fields;
	auto& data=result.second.fields;
	for (int i=0;i<no_of_secondary_inputs;++i)
		secondary_inputs.push_back(csv_line[i]);
	for (int i=no_of_secondary_inputs;i<csv_line.size();++i)
		 data.push_back(csv_line[i]);
	return result;
}

class EntriesWithFixedPrimaryId {
//TODO use 	unordered_map
	map<DBKey,FieldsInDB> entries;
	bool modified=false;
public:
	EntriesWithFixedPrimaryId()=default;
	EntriesWithFixedPrimaryId(const fs::path& path, int no_of_secondary_inputs) {
		if (!exists(path)) return;
		for (auto& csv_line : CSV::from_file(path.native())) {
			entries.insert(db_line_to_fields(csv_line,no_of_secondary_inputs));
		}
	}
	void insert(const DBKey&  secondary_id, const FieldsInDB& data) {
		auto result_of_insertion=entries.emplace(secondary_id,data);
		if (result_of_insertion.second) modified=true;
	}
	void print_to_file(const fs::path& path) const {
		if (!modified) return;
		ofstream f{path.native()};
		for (auto& entry : entries)  {
			entry.first.to_stream(f);		
			f<<";";
			entry.second.to_stream(f);
			f<<"\n";
		}
	}
};


//current implementation assumes the whole Database fits in memory
class SimpleDatabase : public Database {
	fs::path directory;
	int no_of_secondary_inputs;
	map<int,EntriesWithFixedPrimaryId> entries_by_primary_id;
		
	fs::path filepath_from_primary_id(int d) const {
		string s=d>0? std::to_string(d) : ("m"s+std::to_string(-d));
		return directory/s;
	}

	void create_db_if_nonexisting() {
		if (fs::exists(directory)) {
			if (!fs::is_directory(directory))
				throw DbException(directory.native() + " is not a directory");
		}
		else
			fs::create_directory(directory);
	}		
	
	EntriesWithFixedPrimaryId& get_entries_by_primary_id(int d) {	
		auto it=entries_by_primary_id.find(d);
		if (it==entries_by_primary_id.end())
			it=entries_by_primary_id.emplace(d, EntriesWithFixedPrimaryId(filepath_from_primary_id(d),no_of_secondary_inputs)).first;
		return it->second;
	}
	
public:
	SimpleDatabase(const fs::path& directory, int no_of_secondary_inputs) : directory{directory},no_of_secondary_inputs{no_of_secondary_inputs} {
		create_db_if_nonexisting();
	}
	void insert(int primary_id, const vector<string>& secondary_id, const vector<string>& data) override {
		get_entries_by_primary_id(primary_id).insert(secondary_id,data);
	}		
	void close() override {
			for (auto& pair: entries_by_primary_id) 
				pair.second.print_to_file(filepath_from_primary_id(pair.first));
	}
};

//a class to iterate through elements of the database
class SimpleDatabaseView {
	fs::path directory;	
	int no_of_secondary_inputs;
	fs::path filepath_from_primary_input(int d) const {
		return directory/std::to_string(d);
	}
	
	template<typename F> void iterate_through_entries(F&& f, int d, const fs::path& file) const {	
		ifstream s{file.native()};
		string line;
		while (has_data_after_skipping_empty_lines(s)) {
			getline(s,line);
			CSVLine csv_line{line};
			auto fields=db_line_to_fields (csv_line, no_of_secondary_inputs);
			f(d,fields.first, fields.second);
		}
	}
public:
	SimpleDatabaseView(const fs::path& directory, int no_of_secondary_inputs) : directory{directory},no_of_secondary_inputs{no_of_secondary_inputs} {}
	
	template<typename F> void iterate_through_entries(F&& f) const {
		if (!fs::exists(directory)) throw DbException("Database directory not found");
		if (!fs::is_directory(directory)) throw DbException("Database path is not a directory");
		for (auto& file : fs::directory_iterator(directory))  {
			auto path=file.path();
			int d=std::stoi(path.filename().native());
			iterate_through_entries(f,d,path);
		}
	}	

	template<typename F> void iterate_through_entries(F&& f, const set<int>& primary_inputs) const {
		if (!fs::exists(directory)) throw DbException("Database directory not found");
		if (!fs::is_directory(directory)) throw DbException("Database path is not a directory");
		for (auto d: primary_inputs) {
			auto path=filepath_from_primary_input(d);
			if (fs::exists(path))
				iterate_through_entries(f,d,path);
		}
	}	
};

#endif
