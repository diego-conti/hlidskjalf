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

#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <boost/program_options.hpp>
#include "stdincludes.h"
#include "csvschema.h"
#include "system.h"

namespace po = boost::program_options;

//parameters for script invocation
struct ScriptParameters {
	string script;
	string output_dir;
	string flags;
	string work_output_extension;

	string script_invocation(const string& process_id, const string& data_filename, megabytes memory_limit) const {
		return "megabytes:="s+to_string(memory_limit)
			+" outputPath:="s +output_dir
			+" processId:="s +process_id
			+" dataFile:="s +data_filename 
			+ " "s +flags
			+ " "s +script;
	}
};

struct InputParameters {
	string input_file;
	string schema;	//path to schema
	string db;				//path to db
};

struct ComputationParameters {
	int nthreads;
	int computations_per_process;
	int lowest_free_memory_bound_in_kb=0;
	megabytes base_memory_limit;
	megabytes total_memory_limit;
	std::chrono::duration<int> base_timeout;
};

struct CommunicationParameters {
	string valhalla;	//files where interrupted computations are stored
	string huginn;		//directory where files to communicate with processes are stored	
};

enum class OperatingMode {
	NORMAL, BATCH_MODE
};

struct Parameters {
	OperatingMode operating_mode=OperatingMode::NORMAL;
	bool stdio;
	ScriptParameters script_parameters;
	InputParameters input_parameters;
	ComputationParameters computation_parameters;
	CommunicationParameters communication_parameters;
};


class InvalidParametersException : public Exception {
	string description;
public:
	InvalidParametersException(const po::options_description& options_description) {
		std::stringstream s;
		s<<options_description<<endl;
		description=s.str();
	}	

	string what() const noexcept override {
		return description;
	}
};

Parameters command_line_parameters(int argv, char** argc) {
	po::options_description desc("Allowed options");
	desc.add_options()
    ("help", "produce help message")
    ("batch-mode", "print out a list of computations to do, without launching any actual calculation")
    ("stdio", "use stdio output rather than ncurses TUI")

			//script parameters	
		("script", po::value<string>(), "work script filename")		
    ("workoutput", po::value<string>(), "output directory of work script (defaults to the stem of <computations>, i.e. <computations> without the extension)")
		("flags", po::value<string>()->default_value(""), "flags to be passed to work script")
		("extension", po::value<string>()->default_value(".work"), "extension of files generated by the work script")

			//input parameters			
    ("computations", po::value<string>(), "input file containing the list of computations")
    ("schema", po::value<string>(), "info file defining the CSV schema")
    ("db", po::value<string>()->default_value(""), "directory containing database of already performed computations; if empty string or unspecified, database is not used. Computations listed in the database are not repeated")
    
			//computation parameters
    ("nthreads", po::value<int>()->default_value(10), "number of worker threads and magma processes to be run")
    ("workload", po::value<int>()->default_value(100), "computations per process")
    ("free-memory", po::value<int>()->default_value(0), "if set, quit all computations when system free memory goes below this threshold in GB")
    ("total-memory", po::value<int>()->default_value(4), "total memory limit in GB for all threads")
    ("memory", po::value<int>()->default_value(128), "base memory limit in MB for each thread")
	("base-timeout", po::value<int>()->default_value(0),"base timeout limit in seconds, or 0 for no limit")
			
			//communication parameters
    ("valhalla", po::value<string>() , "file where unterminated computations are to be stored (defaults to <output>.valhalla)");

	po::variables_map vm;
	po::store(po::parse_command_line(argv, argc, desc), vm);
	po::notify(vm);    	

	if (vm.count("help") || !vm.count("computations") || !vm.count("script") || !vm.count("schema"))
		throw InvalidParametersException(desc);
	string output_dir;	
	if (!vm.count("workoutput")) output_dir=boost::filesystem::path(vm["computations"].as<string>()).stem().native();
	else output_dir=vm["workoutput"].as<string>();
	string valhalla;
	if (!vm.count("valhalla")) valhalla=output_dir+".valhalla";
	else valhalla=vm["valhalla"].as<string>();
	
	Parameters result;
	if (vm.count("batch-mode")) result.operating_mode=OperatingMode::BATCH_MODE;
	result.stdio=vm.count("stdio");
	result.script_parameters={vm["script"].as<string>(), output_dir,vm["flags"].as<string>(), vm["extension"].as<string>()};
	result.input_parameters={vm["computations"].as<string>(), vm["schema"].as<string>(),vm["db"].as<string>()};
	result.computation_parameters={vm["nthreads"].as<int>(), vm["workload"].as<int>(),  vm["free-memory"].as<int>()*1024*1024, vm["memory"].as<int>(), vm["total-memory"].as<int>()*1024, std::chrono::seconds(vm["base-timeout"].as<int>())};
	result.communication_parameters={valhalla,random_non_existing_file()};
	return result;	
}
#endif
