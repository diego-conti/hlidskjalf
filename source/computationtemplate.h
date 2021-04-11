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
	virtual int no() const =0;
	virtual list<unique_ptr<Field>> split(int parts) const=0;
	virtual unique_ptr<Field> copy() const=0;
};

class TextField : public Field {
	string text;
public:
	TextField(const string& s) : text{s} {}
	vector<string> to_strings() const override {
		return {text};
	}	
	int no() const override {return 1;}
	list<unique_ptr<Field>> split(int parts) const override {
		list<unique_ptr<Field>> result;
		result.push_back(copy());
		return result;
	};
	unique_ptr<Field> copy() const override {
		return make_unique<TextField>(*this);
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
	int no() const override {return max-min+1;}
	list<unique_ptr<Field>> split(int parts) const override {
		int step=(max-min+1)/parts;
		if (step==0) step=1;
		int begin=min;
		list<unique_ptr<Field>> result;
		while (begin + 2*step<max) {
			result.push_back(make_unique<RangeField>(begin,begin+step-1));
			begin+=step;
		}
		result.push_back(make_unique<RangeField>(begin,max));
		return result;
	}
	unique_ptr<Field> copy() const override {
		return make_unique<RangeField>(*this);
	}
};

class ComputationTemplate {
	int primary_input_;
	vector<unique_ptr<Field>> secondary_inputs_;
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
	int largest_field() const {
		int largest=0;
		int max_size=secondary_inputs_[0]->no();
		for (int i=1;i<secondary_inputs_.size();++i)
			if (secondary_inputs_[i]->no()>max_size) {
				max_size=secondary_inputs_[i]->no();
				largest=i;
			}
		return largest;
	}
public:
	ComputationTemplate(int primary_input) : primary_input_{primary_input} {}
	ComputationTemplate(ComputationTemplate&&)=default;
	ComputationTemplate(const ComputationTemplate& other)  : primary_input_{other.primary_input_} {
		for (auto& i : other.secondary_inputs_)
			secondary_inputs_.push_back(i->copy());
	}
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
	int primary_input() const {return primary_input_;}
	int no_computations() const {
		int n=1;
		for (auto& field : secondary_inputs_) n*=field->no();
		return n;	
	}
	vector<ComputationTemplate> split(int max_size) const {
		int parts=no_computations()/max_size+1;
		if (parts == 1) return {*this};
		vector<ComputationTemplate> result;
		result.reserve(parts);
		int i=largest_field();
		auto split_fields=secondary_inputs_[i]->split(parts);
		for (auto& split_field : 	split_fields) {
			ComputationTemplate t{*this};
			t.secondary_inputs_[i]=std::move(split_field);
			result.push_back(std::move(t));
		}
		return result;
	}
};

#endif
