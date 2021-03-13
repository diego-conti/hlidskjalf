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

#ifndef COMPUTATION_TEMPLATE_H
#define COMPUTATION_TEMPLATE_H
#include "computation.h"


class Field {
public:
	virtual vector<string> to_strings() const=0;	
};

class TextField : public Field {
	string text;
public:
	TextField(const string& s) : text{s} {}
	vector<string> to_strings() const override {
		return {text};
	}	
};

class RangeField : public Field {
	int min,max;
public:
	RangeField(int min, int max) : min{min},max{max} {}
	vector<string> to_strings() const override {
		vector<string> result;
		result.reserve(max-min+1);
		for (int i = min; i<=max;++i)
			result.push_back(std::to_string(i));
		return result;
	}		
};

class ComputationTemplate {
	int primary_input_;
	vector<unique_ptr<Field>> secondary_inputs_;
public:
	ComputationTemplate(int primary_input) : primary_input_{primary_input} {}
	void add_range(const string& field) {
		auto i=field.find("..");
		if (i==string::npos) throw CSVException("invalid range",field);
		try {
			int min=std::stoi(field.substr(0,i));
			int max=std::stoi(field.substr(i+2));
			if (min>max) throw 0;
			secondary_inputs_.push_back(make_unique<RangeField>(min,max));	
		}
		catch (...) {
			throw CSVException("range should be of the form min..max with min<=max",field);
		}
	}
	void add_text(const string& field) {
		secondary_inputs_.push_back(make_unique<TextField>(field));
	}	
	list<Computation> computation_instances() const {	
		vector<string> starting_with;
		starting_with.reserve(secondary_inputs_.size());
		return computation_instances(starting_with,secondary_inputs_.begin());
	}
	list<Computation> computation_instances(vector<string> starting_with, vector<unique_ptr<Field>>::const_iterator to_assign) const {
		list<Computation> result;
		if (to_assign==secondary_inputs_.end()) return {Computation{primary_input_,starting_with}};	
		starting_with.resize(starting_with.size()+1);
		auto next=to_assign;
		++next;
		for (auto& field : (*to_assign)->	to_strings()) {
			starting_with.back()=field;
			result.splice(result.begin(),computation_instances(starting_with,next));
		}
		return result;
	}	
	int primary_input() const {return primary_input_;}
};

#endif
