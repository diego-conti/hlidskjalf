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

#ifndef SYSTEM_H
#define SYSTEM_H

#include "stdincludes.h"
#include "exception.h"

//system dependent; works on linux
int free_kb_of_memory() {
	int result;
	string ignore;
	ifstream s{"/proc/meminfo"};
	s>>ignore>>ignore>>ignore>>ignore>>result;
	return result;
}

string magma_path() {
	static const string result=boost::process::search_path("magma").native();
	return result;
}

string random_non_existing_file() {
	auto model="huginn-%%%%%%%%%%%%%%%%";
	while (true) {
		auto random_path=boost::filesystem::unique_path(model);
		random_path=boost::filesystem::unique_path(model);
		if (!boost::filesystem::exists(random_path)){
			string result= random_path.native();
			return result;
		}
	}	
}

void create_dir_if_needed(const boost::filesystem::path& path) {
	if (!boost::filesystem::exists(path)) {
		try {
			cout<<"creating directory "<<path.native()<<endl;
			create_directory(path);		
		}
		catch (...) {
			throw FileException(path.native()," error creating directory");
		}
	}
	else if (!boost::filesystem::is_directory(path)) 
		throw FileException(path.native()," is not a directory");				
}

bool file_exists(const string& filename) {
	try {
		auto input_file=boost::filesystem::path(filename);			
		return boost::filesystem::exists(input_file) && boost::filesystem::is_regular_file(input_file);
	}
	catch (...) {
		return false;
	}
}
#endif
