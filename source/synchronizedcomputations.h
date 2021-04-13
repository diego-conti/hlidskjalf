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
#include "hash.h"

using std::future;
using std::thread;
using std::mutex;
using std::unique_lock;
using std::promise;

template<typename Iterator> Iterator n_th_element_or_end(Iterator begin, Iterator end, int n) {
		if (n==0) return begin;
		assert(n>0);
		auto result=begin;
		std::advance(result,n);
		return result == begin ? end : result;
//note: in theory C++20 allows writing simply
//	return std::advance(begin,n,end); 
}

class UserInterface {
public:
  virtual void computations_added_to_thread(int overall_unassigned_computations, int assigned_computations, megabytes memory_limit) const=0;
  virtual void bad_computation(const Computation& computation, megabytes memory_limit) const =0;
	virtual void removed_computations_in_db(int computations) const=0;
	virtual	void removed_precalculated(int computations) const=0;
  virtual void total_computations(int computations) const =0;
  virtual void unpacked_computations(int computations) const=0;
	virtual void aborted_computations(int computations) const=0;
	virtual void tick() const=0;
	virtual void print_computation(const Computation& computation) const=0;
	virtual	void thread_started(megabytes memory) const=0;
	virtual void thread_stopped(megabytes memory) const=0;
};

class NoUserInterface : public UserInterface {
public:
	void computations_added_to_thread(int overall_unassigned_computations, int assigned_computations, megabytes memory_limit) const override {}
	void bad_computation(const Computation& computation, megabytes memory_limit) const override  {}
	void removed_computations_in_db(int computations) const override{}
	void removed_precalculated(int computations) const override {}
  void total_computations(int computations) const override{}
  void unpacked_computations(int computations) const override{}
	void aborted_computations(int computations) const override {}
	void tick() const override{}
	void print_computation(const Computation& computation) const override {}
	void thread_started(megabytes memory) const override {}
	void thread_stopped(megabytes memory) const override {}
};

class AbortedComputations {
	map<megabytes,list<Computation>> computations_by_memory_limit;
public:
	void insert(const Computation& computation, megabytes memory_limit) {
		computations_by_memory_limit[memory_limit].push_back(computation);
	}
	void insert(Computation&& computation, megabytes memory_limit) {
		computations_by_memory_limit[memory_limit].push_back(std::move(computation));
	}
	list<Computation> remove_exceeding_memory_limit(megabytes memory_limit) {
		list<Computation> result;
		for (auto& p : computations_by_memory_limit)
			if (p.first>memory_limit) 
				result.splice(result.end(),p.second);			
		return result;
	}
	list<Computation> extract_within_memory_limit(megabytes memory_limit, int no_computations) {
		list<Computation> result;
		for (auto& p : computations_by_memory_limit)
			if (p.first<memory_limit && !p.second.empty()) 
				result.splice(result.end(),p.second,p.second.begin(),n_th_element_or_end(p.second.begin(),p.second.end(),no_computations-result.size()));
		return result;
	}
	megabytes lowest_effective_memory_limit() const {
		int lowest=std::numeric_limits<int>::max();
		for (auto& p:computations_by_memory_limit) if (p.first<lowest) lowest=p.first;
		return lowest;
	}
	bool empty() const {
		return all_of(computations_by_memory_limit.begin(),computations_by_memory_limit.end(),[](auto& pair) {return pair.second.empty();});
	}
};

using AssignedComputations = unordered_set<Computation,boost::hash<Computation>>;

class SynchronizedComputations {
	unordered_set<Computation,boost::hash<Computation> > computations;
	AbortedComputations bad;
	deque<ComputationTemplate> packed_computations;
	mutex computations_mtx, bad_mtx, packed_computations_mtx;
	unique_ptr<UserInterface> ui=make_unique<NoUserInterface>();

	void synchronized_add_computations_to_do(AssignedComputations& assigned_computations, int computations_per_process, megabytes memory_limit) {
			int to_add=max(0,computations_per_process- static_cast<int>(assigned_computations.size()));
			bad_mtx.lock();
			auto resurrected=bad.extract_within_memory_limit(memory_limit, to_add);
			bad_mtx.unlock();	
			to_add-=resurrected.size();
			assigned_computations.insert(resurrected.begin(),resurrected.end());
			if (!computations.empty()) {
				auto i=computations.begin(),j=n_th_element_or_end(computations.begin(),computations.end(),to_add);
				assigned_computations.insert(i,j);
				computations.erase(i,j);
			}
	}

	set<int> unpack_computations(int threshold) {
			set<int> primary_ids;
			packed_computations_mtx.lock();
			while (computations.size()<threshold && !packed_computations.empty()) {
				primary_ids.insert(packed_computations.front().primary_input());
				auto computation_instances=packed_computations.front().computation_instances();
				computations.insert(computation_instances.begin(),computation_instances.end());
				packed_computations.pop_front();
			}
			packed_computations_mtx.unlock();
			ui->unpacked_computations(computations.size());
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
	void eliminate_precalculated(const string& output_dir,const CSVSchema& schema) {
    for (auto& x : boost::filesystem::directory_iterator(output_dir))
    	if (boost::filesystem::is_regular_file(x)) {
				std::ifstream f{x.path().native()};
    		eliminate_computations<CSVReader>(f,computations,schema);
    	}	
    ui->removed_precalculated(computations.size());
	}	
protected:
//to be called at initialization or when a new input_file is provided through the UI
	set<int> load_computations(const string& input_file,const CSVSchema& schema, int max_computations_in_template) {
		ifstream s{input_file};
		set<int> primary_ids;
		packed_computations_mtx.lock();
		int total=0;
		while (has_data_after_skipping_empty_lines(s)) {
			CSVLine input{get_line_with_balanced_curly_braces(s)};		
			auto computation_template=CSVReader::extract_computation_template(input,schema);
			total+=computation_template.no_computations();
			for (auto& part : computation_template.split(max_computations_in_template))
				packed_computations.push_back(part);				
		}
		packed_computations_mtx.unlock();
		ui->total_computations(total);
		return primary_ids;
	}
//unpack computation templates into computations and remove those already processed
	void unpack_computations_and_remove_already_processed(int min_threshold, int max_threshold, const optional<SimpleDatabaseView>& db_view,const string& output_dir,const CSVSchema& schema) {	
		unique_lock<mutex> lck{computations_mtx};
		while (computations.size()<min_threshold && !packed_computations.empty()) {
			auto primary_ids=unpack_computations(max_threshold);
			if (db_view) eliminate_computations_in_db(db_view.value(),primary_ids);
			eliminate_precalculated(output_dir,schema);
		}
	}
	int last_used_id(const string& output_dir) const {
		int last_process_id=0;
    for (auto& x : boost::filesystem::directory_iterator(output_dir))
    	if (boost::filesystem::is_regular_file(x)) {
    		try {
    			auto process_number=stoi(x.path().stem().native());
    			last_process_id=max(process_number,last_process_id);
    		}
    		catch (std::invalid_argument& e) {}
    	}	
   	return last_process_id;	
	}
	virtual int to_valhalla(AbortedComputations& computations) =0;
public:
	template<typename InterfaceType, typename... Args>
	void create_user_interface(Args&&... args)  {
		ui=make_unique<InterfaceType>(std::forward<Args>(args)...);
	}

	void mark_as_bad(Computation computation, megabytes memory_limit) {	
		unique_lock<mutex> lock{bad_mtx};		
		ui->bad_computation(computation,memory_limit);
		bad.insert(std::move(computation),memory_limit);		
	}
	void add_computations_to_do(AssignedComputations& assigned_computations, int computations_per_process, megabytes memory_limit) {
		computations_mtx.lock();
		synchronized_add_computations_to_do(assigned_computations,computations_per_process,memory_limit);
		computations_mtx.unlock();
		ui->computations_added_to_thread(computations.size(),assigned_computations.size(),memory_limit);
	}
	void flush_bad() {
		unique_lock<mutex> lock{bad_mtx};
		int removed=to_valhalla(bad);
		if (removed) ui->aborted_computations(removed);
		else ui->tick();
	}
	void print_computations() const {
		for (auto& x: computations) ui->print_computation(x);
	}
	int no_computations() const {
		return computations.size();
	}
	megabytes lowest_effective_memory_limit() {
		if (!computations.empty() || !packed_computations.empty()) return 0;
		unique_lock<mutex> lock{bad_mtx};
		return bad.lowest_effective_memory_limit();
	}
	bool finished() {
		if (computations.empty() && packed_computations.empty()) {
			unique_lock<mutex> lock{bad_mtx,std::try_to_lock};
			return lock? bad.empty() : false;
		}
		else return false;
	}
	UserInterface& get_ui() {
		return *ui;
	}
};





#endif
