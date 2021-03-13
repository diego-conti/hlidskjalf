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

#ifndef COMPUTATIONS_TO_DO_H
#define COMPUTATIONS_TO_DO_H

#include "computation.h"
#include "computationtemplate.h"
#include "db.h"
#include "csvreader.h"
#include <thread>
#include <mutex>
#include <future>
using std::future;
using std::thread;
using std::mutex;
using std::promise;

template<typename Iterator> Iterator n_th_element_or_end(Iterator begin, Iterator end, int n) {
		if (n==0) return begin;
		auto result=begin;
		advance(result,n);
		return result == begin ? end : result;
}

class UserInterface {
public:
	virtual void computations_added_to_thread(int overall_unassigned_computations, int assigned_computations) const=0;
	virtual void bad_computation(const Computation& computation) const =0;
	virtual void removed_computations_in_db(int computations) const=0;
	virtual	void removed_precalculated(int computations) const=0;
	virtual void loaded_computations(int computations) const=0;
	virtual void aborted_computations(int computations) const=0;
	virtual void tick() const=0;
	virtual void print_computation(const Computation& computation) const=0;
};

class NoUserInterface : public UserInterface {
public:
	void computations_added_to_thread(int overall_unassigned_computations, int assigned_computations) const override {}
	void bad_computation(const Computation& computation) const override  {}
	void removed_computations_in_db(int computations) const override{}
	void removed_precalculated(int computations) const override {}
  void loaded_computations(int computations) const override{}
	void aborted_computations(int computations) const override {}
	void tick() const override{}
	void print_computation(const Computation& computation) const override {}
};



class SynchronizedComputations {
	set<Computation> computations,bad;
	mutex computations_mtx, bad_mtx;
	unique_ptr<UserInterface> ui=make_unique<NoUserInterface>();

	void synchronized_add_computations_to_do(list<Computation>& assigned_computations, int computations_per_process) {
			int to_add=max(0,computations_per_process- static_cast<int>(assigned_computations.size()));
			auto i=computations.begin(),j=n_th_element_or_end(computations.begin(),computations.end(),to_add);
			assigned_computations.insert(assigned_computations.end(),i,j);
			computations.erase(i,j);
	}
	
protected:
//to be called during initialization:
	set<int> load_computations(const string& input_file,const CSVSchema& schema) {
		ifstream s{input_file};
		set<int> primary_ids;
		while (has_data_after_skipping_empty_lines(s)) {
			CSVLine input{get_line_with_balanced_curly_braces(s)};		
			auto computation_template=CSVReader::extract_computation_template(input,schema);
			primary_ids.insert(computation_template.primary_input());
			auto computation_instances=computation_template.computation_instances();
			computations.insert(computation_instances.begin(),computation_instances.end());
		}
		ui->loaded_computations(computations.size());
		return primary_ids;
	}
	void eliminate_computations_in_db(const SimpleDatabaseView& db_view, const set<int>& group_orders) {
		int eliminated=0;
		auto eliminate_function=[this,&eliminated] (int primary_input, const FieldsInDB& secondary_inputs, const FieldsInDB& data) {
				Computation computation{primary_input,static_cast<CSVLine>(secondary_inputs)};
				eliminated+=computations.erase(computation);			
		};
		db_view.iterate_through_entries(eliminate_function,group_orders);
		ui->removed_computations_in_db(eliminated);
	}
	int eliminate_precalculated_and_determine_last_process_id(const string& output_dir,const CSVSchema& schema) {
		int last_process_id=0;
    for (auto& x : boost::filesystem::directory_iterator(output_dir))
    	if (boost::filesystem::is_regular_file(x)) {
    		try {
    			auto process_number=stoi(x.path().stem().native());
    			last_process_id=max(process_number,last_process_id);
    		}
    		catch (std::invalid_argument& e) {}
				std::ifstream f{x.path().native()};
    		eliminate_computations<CSVReader>(f,computations,schema);
    	}	
    ui->removed_precalculated(computations.size());
   	return last_process_id;
	}
	
	virtual void to_valhalla(set<Computation>& computations) =0;
public:
	template<typename InterfaceType, typename... Args>
	void create_user_interface(Args&&... args)  {
		ui=make_unique<InterfaceType>(std::forward<Args>(args)...);
	}

	void mark_as_bad(Computation computation) {	
		bad_mtx.lock();			
		ui->bad_computation(computation);
		bad.insert(computation);
		bad_mtx.unlock();
	}
	void add_computations_to_do(list<Computation>& assigned_computations, int computations_per_process) {
		computations_mtx.lock();
		int computations_size=computations.size();
		if (computations_size) 
			synchronized_add_computations_to_do(assigned_computations,computations_per_process);
		computations_mtx.unlock();
		if (computations_size)
				ui->computations_added_to_thread(computations_size,assigned_computations.size());
	}
	void flush_bad() {
		bad_mtx.lock();
		if (bad.size()) {
			ui->aborted_computations(bad.size());
			to_valhalla(bad);
			bad.clear();
		}
		else ui->tick();
		bad_mtx.unlock();
	}
	void print_computations() const {
		for (auto& x: computations) ui->print_computation(x);
	}
	int no_computations() const {
		return computations.size();
	}
};





#endif
