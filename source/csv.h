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

#ifndef CSV_LINE_H
#define CSV_LINE_H

#include <boost/filesystem.hpp>
#include "exception.h"

class CSVLine {
	friend std::size_t hash_value(const CSVLine&);
	vector<string> values;	
public:
	CSVLine()=default;	
	CSVLine(string csv_line) {
		int i=0;
		while (i<csv_line.size()) {
			assert (i<csv_line.size());
			int colon_position=csv_line.find(';',i);		
			if (colon_position==string::npos) colon_position=csv_line	.size();
			values.push_back(csv_line.substr(i,colon_position-i));
			i=colon_position+1;
		}	
	}
	template<typename T> static CSVLine from_vector(T&& v) {
		CSVLine result;
		result.values=std::forward<T>(v);
		return result;
	}

	bool match(const CSVLine& line, int up_to) const {
		for (int i=0;i<up_to;++i)
			if (values[i]!=line.values[i]) return false;
		return true;
	}
	string to_string() const {
		if (values.empty()) return {};
		string result=values[0];
		for (int i=1;i<values.size();++i) result+=";"+values[i];
		return result;
	}
	string operator[] (int n) const {return values[n];}
	int size() const {return values.size();}
	bool operator==(const CSVLine& other) const {
		return values==other.values;
	}
	bool operator<(const CSVLine& other) const {
		return values<other.values;
	}
};

bool has_data_after_skipping_empty_lines(std::istream& is) {
		while (is.peek()=='\n')	is.get();
		return is && is.peek()!=EOF; 
}

ifstream open_file(string filename) {
		auto input_file=boost::filesystem::path(filename);
		if (!boost::filesystem::exists(input_file))
			throw FileException(filename,"not existing");
		else if (!boost::filesystem::is_regular_file(input_file)) 
			throw FileException(filename,"is not a file");
		return ifstream{filename};
}

class SeekException : public Exception {
	string line;
public:
	SeekException(const CSVLine& csv_line) : line{csv_line.to_string()} {}
	string what() const noexcept override {
		return "No matching string found in CSV for "+line;
	}
};


string get_line_with_balanced_curly_braces(std::istream& s) {
	string line;
	auto c=s.get();
	bool open_brace=false;
	while (s && !s.eof()  && (c!='\n' || open_brace)) {
		if (c=='{') open_brace=true;
		else if (c=='}') open_brace=false;
		if (c!='\n') line+=c;
		c=s.get();
	}
	return line;
}

class CSV {
	vector<CSVLine> lines;
	CSV()= default;
	int seek(const CSVLine& line, int match_up_to) const {
		for (int i=0;i<lines.size();++i)
			if (line.match(lines[i],match_up_to)) return i;
		throw SeekException{line};
	}	
public:
	CSV(ifstream& s) {
		while (has_data_after_skipping_empty_lines(s)) 
			lines.emplace_back(get_line_with_balanced_curly_braces(s));		
	}
	static CSV from_file(string filename) {
		auto file=open_file(filename);
		return CSV{file};	
	}
	void to_file(string filename) const {
		ofstream os{filename};
		for (auto& x : lines) 
			os<<x.to_string()<<std::endl;
	}
	auto begin() const {return lines.begin();}
	auto end() const {return lines.end();}
};

#endif
