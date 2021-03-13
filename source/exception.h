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

#ifndef EXCEPTION_H 
#define EXCEPTION_H

#include "stdincludes.h"

class Exception {
public:
	virtual string what() const noexcept =0;
};

class FileException : public Exception {
	string path;
	string error;
public:
	FileException(const string& path, const string& error) : path{path}, error{error} {}
	string what() const noexcept override {
		return "File error: file="+ path+", "+error;
	}
};

class CSVException : public Exception {
	string error,line;
public: 
	CSVException(const string& error, const string& line) : error{error}, line{line} {}
	string what() const noexcept override {
		return "CSV parse error: "+error+", when parsing "+line;
	}	
};

class OutOfMemoryException : public Exception {
	int free_kb_of_memory;
public:
	OutOfMemoryException(int 	free_kb_of_memory) : free_kb_of_memory{free_kb_of_memory} {}
	string what() const noexcept override {
		return "out of memory: free RAM "s+to_string(free_kb_of_memory)+"kB";
	}
};
#endif
