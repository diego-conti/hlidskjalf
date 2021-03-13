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

#ifndef CSV_SCHEMA_H
#define CSV_SCHEMA_H

#include "exception.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/foreach.hpp>
#include "csv.h"

namespace pt = boost::property_tree;

class PropertyTreeException : public Exception {
	pt::ptree tree;
	string error;
public:
	PropertyTreeException(const pt::ptree& tree, const string& error) : tree{tree}, error{error} {}
	string what() const noexcept override {
		return "Error parsing property tree "s+error;
	}

};

struct ReplacementRule {
	string match;
	string replace_with;

	ReplacementRule(const pt::ptree& tree) {
		match=tree.get<string>("match");
		replace_with=tree.get<string>("with");
	}
	ReplacementRule()=default;
	string to_string() const {
		return "Replacement rule "+match+" -> "+replace_with;
	}
};

int column_number_from_info(const pt::ptree& tree) {
//convert to zero-based input
	return stoi(tree.data())-1;
}

int column_number_from_info(const pt::ptree& tree, const string& field) {
//convert to zero-based input
	try {
		return tree.get<int>(field)-1;
	}
	catch (const pt::ptree_error& e) {
		throw PropertyTreeException(tree,"Expected numeric field "s+field);
	}
}


struct ColumnForOutput {
	int input;
	vector<ReplacementRule> rules;
public:
	ColumnForOutput()=default;
	ColumnForOutput(const pt::ptree& tree) {
		input=column_number_from_info(tree);
		for (auto& v: tree) {
			if (v.first!="replacement") throw PropertyTreeException(tree,"'replacement' expected, found "s+v.first);
			rules.emplace_back(v.second);
		}
	}
	string from_csv_line(const CSVLine& csvline) const {
		auto entry=csvline[input];
		for (auto& rule : rules)
			if (rule.match==entry) return rule.replace_with;
		return entry;
	}
	
	string to_string() const {
		string s="column "+std::to_string(input+1)+" {";
		auto i=rules.begin();
		if (i!=rules.end()) {
			s+=i->to_string();		
			while (++i!=rules.end())
				s+=","+i->to_string();
		}
		s+="}";
		return s;
	}
};

class OmitRule_impl {
public:
	virtual bool omit(const string& s) const =0;
	virtual	string to_string() const=0;
};

class MatchOmitRule : public OmitRule_impl {
	string s;
public:
	MatchOmitRule(const string& s) : s{s} {}
 	bool omit(const string& column) const override {
 		return column==s;
 	}
 	string to_string() const override {
 		return "matches "+s;
 	}
};

class ContainsOmitRule : public OmitRule_impl {
	int input;
	string s;
public:
	ContainsOmitRule(const string& s) : s{s} {}
 	bool omit(const string& column) const override {
 		return column.find(s)!=string::npos;
 	}
 	string to_string() const override {
 		return "contains "+s;
 	}
};

class OmitRule {
	int input;
	unique_ptr<OmitRule_impl> rule;
public:
	OmitRule()=default;
	OmitRule(const pt::ptree& tree) {
		input=column_number_from_info(tree,"column");
		auto match=tree.get<string>("match",{});
		auto contains=tree.get<string>("contains",{});
		if (!match.empty()) rule=make_unique<MatchOmitRule>(match);
		else if (!contains.empty()) rule=make_unique<ContainsOmitRule>(contains);
		else throw PropertyTreeException(tree, "either 'match' or 'contains' expected"s);
	}
	bool omit(const CSVLine& line) const {
		return rule->omit(line[input]);
	}
	string to_string() const {
		return "omit if column "+std::to_string(input+1)+" "+rule->to_string();
	}
};

	

//Format of CSV lines produced by the work script.
class CSVSchema {
	friend class CSVReader;
	int columns;								//number of columns in each CSV line
	int primary_input_column;		//input of the column representing the primary input
	vector<int> secondary_input_columns;	//indices of columns representing secondary indices
	vector<ColumnForOutput> for_output;	//non-input columns that are put in the database
	vector<OmitRule> omit_rules;	//rules to determine whether an entry should be omitted in the db
	vector<int> range_columns;	//indices of columns representing ranges (subset of secondary_input_columns)
public:
	CSVSchema()=default;
	CSVSchema(const pt::ptree& tree) {
		columns=tree.get<int>("columns");
		for (auto& v: tree.get_child("columns")) {
			if (v.first!="output") throw PropertyTreeException(tree,"'output' expected, found "s+v.first);
			for_output.push_back(v.second);		
		}
		for (auto& v : tree.get_child("omitrules",pt::ptree{})) {
			if (v.first!="condition") throw PropertyTreeException(tree,"'output' expected, found "s+v.first);
			omit_rules.emplace_back(v.second);
		}
		primary_input_column=column_number_from_info(tree, "inputcolumns");
		for (auto& v : tree.get_child("inputcolumns")) {
			int column=column_number_from_info(v.second);
			if (v.first=="rangeinputcolumn") range_columns.push_back(column);
			else if (v.first!="textinputcolumn") throw PropertyTreeException(tree,v.first+" unexpected"s);
			secondary_input_columns.push_back(column);
		}
	}
	string to_string() const {
		string s="CSV schema with "+std::to_string(columns)+" columns\n";
		s+="primary input column "+std::to_string(primary_input_column+1)+"\n";
		for (int input: secondary_input_columns) {
			string type=is_range_column(input)? "range" : "text";
			s+="secondary input column "+std::to_string(input+1)+ " of type "+type+"\n";
		}
		for (auto& c: for_output) s+=c.to_string()+"\n";
		for (auto& c: omit_rules) s+=c.to_string()+"\n";
		return s;	
	}
	int input_columns() const {return columns;}
	const vector<ColumnForOutput>& output_columns() const {return for_output;}
	int no_secondary_input_columns() const {return secondary_input_columns.size();}
	bool is_range_column(int input) const {return std::find(range_columns.begin(),range_columns.end(),input)!=range_columns.end();}
};

#endif
