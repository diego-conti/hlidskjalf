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

#ifndef COMPUTATION_H
#define COMPUTATION_H
#include "stdincludes.h"
#include "csvschema.h"

//we use the input fields defined by the schema to define the computation
class Computation {
	int primary_input_;
	CSVLine secondary_inputs_;
public:
	Computation() = default;
	Computation(int primary_input, const vector<string>& secondary_inputs) : primary_input_{primary_input}, secondary_inputs_{CSVLine::from_vector(secondary_inputs)} {}
	Computation(int primary_input, const CSVLine& secondary_inputs) : primary_input_{primary_input}, secondary_inputs_{secondary_inputs} {}

	string to_string() const {return std::to_string(primary_input_)+";"+secondary_inputs_.to_string();}
	bool operator==(const Computation& other) const {
		return primary_input_==other.primary_input_ && secondary_inputs_==other.secondary_inputs_;
	}
	bool operator<(const Computation& other) const {
		if (primary_input_<other.primary_input_) return true;
		else if (primary_input_>other.primary_input_) return false;
		else return secondary_inputs_<other.secondary_inputs_;
	}
	int primary_input() const {return primary_input_;}
};

template<typename T>
void erase(list<T>& container, T object) 
{
	container.remove(object);
}
template<typename T>
void erase(set<T>& container, T object) 
{
	container.erase(object);
}

template<typename CSVReader,typename T>
void eliminate_computations(istream& is, T& computations, const CSVSchema& schema) {
	int count=0;
	while (has_data_after_skipping_empty_lines(is)) {
		try {
				CSVLine input{get_line_with_balanced_curly_braces(is)};    			
				erase(computations,CSVReader::extract_computation(input,schema));
		}
		catch (const Exception& e) {
			cout<<e.what();
			throw;
		}
	}
}


#endif
