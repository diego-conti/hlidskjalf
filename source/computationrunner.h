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
#include <boost/asio/io_service.hpp>
#include "parameters.h"

constexpr int COMPUTATIONS_TO_STORE_IN_MEMORY=1024*1024;


class Processes {
	mutex mtx;
	list<boost::process::child*> processes;
public:
	void add(boost::process::child* child) {
		unique_lock<mutex> lck{mtx};
		processes.push_back(child);		
	}
	void remove(boost::process::child* child) {
		unique_lock<mutex> lck{mtx};
		processes.remove(child);		
	}	
	void terminate() {		
		unique_lock<mutex> lck{mtx};
		for (auto child: processes)
			child->terminate();
		processes.clear();
	}	
	int size() const {
		return processes.size();
	}		
};

class MagmaRunner {
	string magma_script;
	string magma_path;	
	Processes processes;

	void write_computations_to_do(const string& data_filename,const AssignedComputations& computations) {
		ofstream file{data_filename,std::ofstream::trunc};
		for (auto x: computations) file<<x.to_string()<<endl;
		file.close();	
	}

	enum class LineType {LINE,PART,OVER,INVALID};
	pair<LineType, string> parse_line(const string& line) const {
		auto first_five_chars=line.substr(0,5);
		if (first_five_chars=="LINE "s) return {LineType::LINE,line.substr(5)};
		else if (first_five_chars=="PART "s) return {LineType::PART,line.substr(5)};
		else if (first_five_chars=="OVER"s) return {LineType::OVER,{}};
		else return {LineType::INVALID,{}};	
	}
	
	void add_line(vector<string>& lines, const string& line, string& last_string) const {
		auto type_and_string=parse_line(line);
		switch (type_and_string.first) {
			case LineType::LINE: 
				lines.push_back(type_and_string.second); 
				break;
			case LineType::PART:
				last_string+=type_and_string.second; 
				break;
			case LineType::OVER:
				lines.push_back(last_string); 
				last_string.clear();
				break;				
			default:
				break;
		}
	}

	string launch_child(const string& command_line,std::chrono::duration<int> timeout) {
		boost::asio::io_service ios;
		std::future<std::string> data;
		auto child=boost::process::child{command_line, boost::process::std_in.close(), boost::process::std_out > data, ios};
		processes.add(&child);		
		bool terminated_early=ios.run_for(timeout);
		processes.remove(&child);
		return data.get();
	}

	//returns empty vector if timeout
 	vector<string> launch_child_and_read_data(const string& command_line, std::chrono::duration<int> timeout) {
		cout<<"starting process :"<<command_line<<endl;
		auto whole_result=launch_child(command_line,timeout);
 		vector<string> result;
 		std::stringstream s{whole_result};
		string line,last_string;
		while (s && std::getline(s, line) && !line.empty())  {
			cout<<line<<endl;
  			add_line(result,line,last_string);    
		}
		for (auto l: result) cout<<l<<endl;
		cout<<endl;
		return result;
 	}


public:
	MagmaRunner(const string& magma_script) : magma_script{magma_script}, magma_path{::magma_path()} {}

	string script_version() const {
	  boost::process::ipstream is;
	  cout<<magma_path+ " -b "+ScriptParameters{magma_script,".","printVersion:=true","."}.script_invocation("x","x",0)<<endl;
 		boost::process::system (magma_path+ " -b "+ScriptParameters{magma_script,".","printVersion:=true","."}.script_invocation("x","x",0),boost::process::std_out > is);
    std::string line;
    string script_version;
    while (std::getline(is, line) && !line.empty()) 
	    script_version=line;
		return script_version;
	}

	 std::vector<std::string> invoke_magma_script(const string& process_id,const AssignedComputations& computations,const Parameters& parameters, megabytes memory_limit, std::chrono::duration<int> timeout) {						
		auto data_filename=parameters.communication_parameters.huginn+"/"+process_id+".data";
		write_computations_to_do(data_filename,computations);
		return launch_child_and_read_data(magma_path+" -b "+parameters.script_parameters.script_invocation(process_id,data_filename, memory_limit),timeout);	
	}
	void terminate_all() {
		processes.terminate();	
	}
	int running() const {return processes.size();}
};


//TODO replace inheritance with a data member
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
	
//return the number of computation to assign to a process with a fixed memory_limit; this defaults to the parameter indicated in the command line, but it can be reduced if few computations remain to be done. If the only computations that remain to be done are the previously aborted computations, then the default parameter for the others. For the large thread, the default parameter is divided by the square of the number of threads
	int no_computations_to_assign(megabytes memory_limit) {
		int nthreads=parameters.computation_parameters.nthreads;
		int new_computations=no_computations();
		int computations_per_process=(large_thread(memory_limit))? parameters.computation_parameters.computations_per_process/(nthreads*nthreads) : parameters.computation_parameters.computations_per_process;
		if (new_computations) return min(computations_per_process,new_computations/parameters.computation_parameters.nthreads);
		else return computations_per_process;
	}
protected:
	int to_valhalla(AbortedComputations& computations) override {
		int memory_limit=parameters.computation_parameters.total_memory_limit;
		auto removed=computations.remove_exceeding_memory_limit(memory_limit);
		if (!removed.empty()) {
			ofstream valhalla_file{parameters.communication_parameters.valhalla,std::ofstream::app};
			for (auto& computation : removed) 
				valhalla_file<<computation.to_string()<<";"<<memory_limit<<";"<<script_version<<endl;
			return removed.size();
		}
		else return 0;
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
		load_computations(parameters.input_parameters.input_file);
		last_process_id=SynchronizedComputations::last_used_id(parameters.script_parameters.output_dir);
	}
	void load_computations(const string& file) {
		SynchronizedComputations::load_computations(file,schema,COMPUTATIONS_TO_STORE_IN_MEMORY/2);
	}
	
	static ComputationRunner& singleton() {
		static ComputationRunner computations_to_do;
		return computations_to_do;
	}
	
	void print_computations(ThreadUIHandle& thread_ui) {
		do {
			int to_unpack=parameters.computation_parameters.total_memory_limit*1024*1024/128; //assume each Computation takes 128 bytes
			unpack_computations_and_remove_already_processed(to_unpack,to_unpack, create_db_view(),parameters.script_parameters.output_dir, schema,thread_ui);
			SynchronizedComputations::print_computations();
		} while (!finished());
	}
	
	void add_computations_to_do(AssignedComputations& assigned_computations, megabytes memory_limit,ThreadUIHandle& thread_ui) {
			int min_threshold=parameters.computation_parameters.computations_per_process*parameters.computation_parameters.nthreads;
			if (no_computations()<min_threshold) {
				unpack_computations_and_remove_already_processed(parameters.computation_parameters.computations_per_process,COMPUTATIONS_TO_STORE_IN_MEMORY, create_db_view(), parameters.script_parameters.output_dir, schema,thread_ui);	
			}
			auto computations_per_process=no_computations_to_assign(memory_limit);
			if (computations_per_process==0 && assigned_computations.empty()) computations_per_process=1;
			SynchronizedComputations::add_computations_to_do(assigned_computations,computations_per_process,memory_limit);
			thread_ui.computations_added(assigned_computations.size(),memory_limit);
	}
	
	void tick() {	
		auto limit=parameters.computation_parameters.lowest_free_memory_bound_in_kb;	
		if (limit && free_kb_of_memory()<limit) 
			throw	OutOfMemoryException{free_kb_of_memory()};
		SynchronizedComputations::tick();
	}
	
	int assign_id() {
		return ++last_process_id;
	}
	
	void terminate() {
		SynchronizedComputations::terminate();
		magma_runner->terminate_all();
	}
	void set_no_computations(int ncomputations) {
		if (ncomputations>0) parameters.computation_parameters.computations_per_process=ncomputations;
	}
	AssignedComputations compute(const string& process_id, AssignedComputations computations, megabytes memory_limit) const {
		auto output_filename=parameters.script_parameters.output_dir+"/"+process_id+parameters.script_parameters.work_output_extension;		
		if (terminating()) return AssignedComputations{};
		auto data=	magma_runner->invoke_magma_script(process_id, computations,parameters,memory_limit,parameters.computation_parameters.timeout);
		ofstream output{output_filename,std::ofstream::app};		
		for (auto& line : data) {
			int size=computations.size();
			erase(computations,CSVReader::extract_computation(line,schema));
			if (computations.size()==size) std::cerr<<"cannot find computation "<<line<<endl;
			output<<line<<endl;
		}
		return terminating()? AssignedComputations{} : computations;
		//ui->completed_computations(data.size());
	}
	bool large_thread(megabytes memory_limit) {
		return memory_limit > 2*parameters.computation_parameters.base_memory_limit && memory_limit > 2*lowest_effective_memory_limit();
	}
	bool finished() {
		return SynchronizedComputations::finished() && !magma_runner->running();
	}

};

#endif 
