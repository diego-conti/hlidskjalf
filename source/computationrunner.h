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
#ifndef COMPUTATION_RUNNER_H
#define COMPUTATION_RUNNER_H
#include "synchronizedcomputations.h"
#include <boost/process.hpp>
#include "parameters.h"

constexpr int COMPUTATIONS_TO_STORE_IN_MEMORY=1024*1024;


class MagmaRunner {
	string magma_script;
	string magma_path;	
public:
	MagmaRunner(const string& magma_script) : magma_script{magma_script}, magma_path{::magma_path()} {}

	string script_version() const {
	  boost::process::ipstream is; //reading pipe-stream
 		boost::process::system (magma_path+ " -b printVersion:=true "+ magma_script,boost::process::std_out > is);
    std::string line;
    string script_version;
    while (std::getline(is, line) && !line.empty()) 
	    script_version=line;
		return script_version;
	}

	void invoke_magma_script(const string& process_id,const list<Computation>& computations,const Parameters& parameters) const {
		auto data_filename=parameters.communication_parameters.huginn+"/"+process_id+".data";
		ofstream file{data_filename,std::ofstream::trunc};
		for (auto x: computations) file<<x.to_string()<<endl;
		file.close();
		boost::process::child child{magma_path+" -b "+parameters.script_parameters.script_invocation(process_id,data_filename), boost::process::std_out > boost::process::null};
		child.wait();	
	//if the program is terminated, the destructor of child is invoked and the child process is terminated
	}
};

class ComputationRunner : public SynchronizedComputations {
	static constexpr int NO_ID=-1;
	Parameters parameters;
	string script_version;
	int last_process_id;
	unique_ptr<MagmaRunner> magma_runner;
	CSVSchema schema;
	
	static void verify_files_exist(const Parameters& parameters) {
		auto input_file=boost::filesystem::path(parameters.input_parameters.input_file);
		if (!boost::filesystem::exists(input_file) || !boost::filesystem::is_regular_file(input_file)) 
			throw FileException(parameters.input_parameters.input_file,"not existing or not a file");
		auto valhalla_file=boost::filesystem::path(parameters.communication_parameters.valhalla);
		if (boost::filesystem::exists(valhalla_file)) {
			if (!boost::filesystem::is_regular_file(valhalla_file))
				throw FileException(parameters.communication_parameters.valhalla,"not a file");
			else cout<<"Warning: "<<parameters.communication_parameters.valhalla<<" exists; skipped computations will be appended"<<endl;
		}
		create_dir_if_needed(parameters.script_parameters.output_dir);
		create_dir_if_needed(parameters.communication_parameters.huginn);		
	}
protected:
	void to_valhalla(set<Computation>& computations) override {
		ofstream valhalla_file{parameters.communication_parameters.valhalla,std::ofstream::app};
		for (auto& computation : computations) 
				valhalla_file<<computation.to_string()<<";"<<parameters.script_parameters.memory_per_thread<<";"<<script_version<<endl;
	}
	optional<SimpleDatabaseView> create_db_view() const {
		return !parameters.input_parameters.db.empty()? make_optional<SimpleDatabaseView>(parameters.input_parameters.db,schema.no_secondary_input_columns()) : nullopt;
	}
public:
	void init(const Parameters& parameters) {	
		this->parameters=parameters;
		pt::ptree tree;
		pt::read_info(parameters.input_parameters.schema,tree);
		schema=CSVSchema{tree};		
		magma_runner=make_unique<MagmaRunner>(parameters.script_parameters.script);
		script_version=magma_runner->script_version();
		verify_files_exist(parameters);
		load_computations(parameters.input_parameters.input_file,schema,COMPUTATIONS_TO_STORE_IN_MEMORY/2);
		last_process_id=SynchronizedComputations::last_used_id(parameters.script_parameters.output_dir);
	}
	
	static ComputationRunner& singleton() {
		static ComputationRunner computations_to_do;
		return computations_to_do;
	}

	void add_computations_to_do(list<Computation>& assigned_computations) {
			if (no_computations()<parameters.computation_parameters.computations_per_process*parameters.computation_parameters.nthreads)
				unpack_computations_and_remove_already_processed(COMPUTATIONS_TO_STORE_IN_MEMORY, create_db_view(), parameters.script_parameters.output_dir, schema);	
			auto computations_per_process=min(parameters.computation_parameters.computations_per_process, static_cast<int>(no_computations())/parameters.computation_parameters.nthreads);
			if (computations_per_process==0 && assigned_computations.empty()) computations_per_process=1;
			SynchronizedComputations::add_computations_to_do(assigned_computations,computations_per_process);
	}
	
	void check_memory_and_flush_bad() {	
		auto limit=parameters.computation_parameters.lowest_free_memory_bound_in_kb;	
		if (limit && free_kb_of_memory()<limit) 
			throw	OutOfMemoryException{free_kb_of_memory()};
		flush_bad();
	}
	
	int assign_id() {
		return ++last_process_id;
	}
	
	
	list<Computation> compute(const string& process_id, list<Computation> computations) const {
		magma_runner->invoke_magma_script(process_id, computations,parameters);
		auto output_filename=parameters.script_parameters.output_dir+"/"+process_id+parameters.script_parameters.work_output_extension;
		ifstream result_file{output_filename};	
		eliminate_computations<CSVReader>(result_file,computations,schema);
		return computations;
	}
	
};

#endif 
