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

#ifndef CSV_READER_H
#define CSV_READER_H

#include "csvschema.h"
#include "db.h"
#include "computationtemplate.h"

struct CSVReader {
	static const OmitRule* omit_reason_or_null(const CSVLine& csvline, const CSVSchema& schema) {
		if (csvline.size()!=schema.input_columns()) throw CSVException("wrong numer of columns",csvline.to_string());
		for (auto& column: schema.omit_rules) 
				if (column.omit(csvline)) return &column;
		return nullptr;
	}
	
	static void insert_entry(const CSVLine& csvline, const CSVSchema& schema, Database& db) {
		int primary_id=stoi(csvline[schema.primary_input_column]);
		vector<string> secondary_id;
		secondary_id.reserve(schema.secondary_input_columns.size());
		for (auto input: schema.secondary_input_columns)
			secondary_id.push_back(csvline[input]);
		vector<string> values;
		values.reserve(schema.for_output.size());
		for (auto& column: schema.output_columns()) 
			values.push_back(column.from_csv_line(csvline));
		db.insert(primary_id,secondary_id,values);
	};
	
	static Computation extract_computation(const CSVLine& csvline, const CSVSchema& schema) {
		vector<string> result;
		int primary_id=stoi(csvline[schema.primary_input_column]);
		for (auto input: schema.secondary_input_columns)
			result.push_back(csvline[input]);
		return {primary_id,move(result)};
	}
	
	static ComputationTemplate extract_computation_template(const CSVLine& csvline, const CSVSchema& schema) {
		ComputationTemplate result{stoi(csvline[schema.primary_input_column])};
		for (auto input: schema.secondary_input_columns)
			if (schema.is_range_column(input)) {
				result.add_range(csvline[input]);
			}
			else result.add_text(csvline[input]);
		return result;
	}
	
};


#endif
