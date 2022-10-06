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
#include "ui.h"
#include "hash.h"
#include "csvreader.h"

template<typename Iterator> Iterator n_th_element_or_end(Iterator begin, Iterator end, int n) {
	assert(n>=0);
	while (begin!=end && n) {
		++begin;
		--n;
	}
	return begin;
//note: in theory C++20 allows writing simply
//	return std::advance(begin,n,end); 
}


class AbortedComputations {
	map<megabytes,list<Computation>> computations_by_memory_limit;
	int size_=0;
	mutable mutex mtx;
public:
	void insert(const Computation& computation, megabytes memory_limit) {
		unique_lock<mutex> lock{mtx};
		computations_by_memory_limit[memory_limit].push_back(computation);
		++size_;
	}
	void insert(Computation&& computation, megabytes memory_limit) {
		unique_lock<mutex> lock{mtx};
		computations_by_memory_limit[memory_limit].push_back(std::move(computation));
		++size_;
	}
	list<Computation> remove_exceeding_memory_limit(megabytes memory_limit) {
		unique_lock<mutex> lock{mtx};
		list<Computation> result;
		for (auto& p : computations_by_memory_limit)
			if (p.first>=memory_limit) 
				result.splice(result.end(),p.second);			
		size_-=result.size();
		return result;
	}
	list<Computation> extract_within_memory_limit(megabytes memory_limit, int no_computations) {
		unique_lock<mutex> lock{mtx};
		list<Computation> result;
		for (auto& p : computations_by_memory_limit)
			if (p.first<memory_limit && !p.second.empty()) 
				result.splice(result.end(),p.second,p.second.begin(),n_th_element_or_end(p.second.begin(),p.second.end(),no_computations-result.size()));
		size_-=result.size();
		return result;
	}
	megabytes lowest_effective_memory_limit() const {
		unique_lock<mutex> lock{mtx};
		int lowest=std::numeric_limits<int>::max();
		for (auto& p:computations_by_memory_limit) if (p.first<lowest && !p.second.empty()) lowest=p.first;
		if (lowest==std::numeric_limits<int>::max()) lowest=0;
		return lowest;
	}
	vector<pair<megabytes,int>> summary() const {
		unique_lock<mutex> lock{mtx};
		vector<pair<megabytes,int>> result;
		for (auto & p:computations_by_memory_limit) if (p.second.size()) result.emplace_back(p.first,p.second.size());
		return result;		
	}
	void clear() {computations_by_memory_limit.clear(); size_=0;}
	int size() const {return size_;}
	bool empty() const {
		return size_==0;
	}
};

using AssignedComputations = unordered_set<Computation,boost::hash<Computation>>;

class UnpackedComputations {
	unordered_set<Computation,boost::hash<Computation> > computations;
	mutex mtx;
public:
	int unpack(const ComputationTemplate& packed) {
		auto computation_instances=packed.computation_instances();
		computations.insert(computation_instances.begin(),computation_instances.end());	
		return computation_instances.size();
	}
	int eliminate_computations_in_db(const SimpleDatabaseView& db_view, const set<int>& group_orders) {
		int size=computations.size();
		auto eliminate_function=[this] (int primary_input, const FieldsInDB& secondary_inputs, const FieldsInDB& data) {
				Computation computation{primary_input,static_cast<CSVLine>(secondary_inputs)};
				computations.erase(computation);			
		};
		db_view.iterate_through_entries(eliminate_function,group_orders);
		return size-computations.size();
	}
	int eliminate_precalculated(const string& output_dir,const CSVSchema& schema, const atomic<bool>& terminate) {
		int size=computations.size();
    for (auto& x : boost::filesystem::directory_iterator(output_dir))
    	if (!terminate && boost::filesystem::is_regular_file(x)) {
				std::ifstream f{x.path().native()};
    		eliminate_computations<CSVReader>(f,computations,schema);
    	}
		return size-computations.size();    		
	}
	void assign (int to_add, AssignedComputations& assigned_computations) {	
		auto i=computations.begin(),j=n_th_element_or_end(computations.begin(),computations.end(),to_add);
		assigned_computations.insert(i,j);
		computations.erase(i,j);	
	}
	auto unique_lock() {
		return std::unique_lock(mtx);
	}
	auto scoped_lock(mutex& m) {
		return std::scoped_lock(mtx,m);
	}
	void clear() {
		computations.clear();
	}
	auto begin() const {return computations.begin();}
	auto end() const {return computations.end();}	
	int size() const {return computations.size();}
	bool empty() const {return computations.empty();}
	void insert(UnpackedComputations&& other) {
		computations.insert(other.begin(),other.end());
		other.clear();
	}
};

class PackedComputations {
	int size_=0;
	deque<ComputationTemplate> packed_computations;
	mutex mtx;
public:
	int size() const {
		return size_;
	}
	bool empty() const {return packed_computations.empty();}

	void load(istream& s, const CSVSchema& schema, int max_computations_in_template) {
		unique_lock<mutex> lock{mtx};
		while (has_data_after_skipping_empty_lines(s)) {
			string input;
			getline(s,input);
			auto computation_template=CSVReader::extract_computation_template(input,schema);
			size_+=computation_template.no_computations();
			for (auto& part : computation_template.split(max_computations_in_template))
				packed_computations.push_back(part);
		}
	}
	
	set<int> unpack(int threshold, UnpackedComputations& computations) {
		unique_lock<mutex> lock{mtx};
		set<int> primary_ids;
		while (computations.size()<threshold && !packed_computations.empty()) {
			primary_ids.insert(packed_computations.front().primary_input());
			size_-=computations.unpack(packed_computations.front());
			packed_computations.pop_front();
		}
		return primary_ids;
	}
	void clear() {
		unique_lock<mutex> lock{mtx};
		packed_computations.clear();
	}	
};

class SynchronizedComputations {
	AbortedComputations bad;
	UnpackedComputations computations;
	PackedComputations packed_computations;
	UserInterface* ui=&NoUserInterface::singleton();
	int abandoned=0;
	atomic<bool> should_terminate;
	atomic<int> unpacking_threads=0;

	void synchronized_add_computations_to_do(AssignedComputations& assigned_computations, int computations_per_process, megabytes memory_limit) {
	}
protected:
	void terminate() {
		should_terminate=true;
		packed_computations.clear();
		bad.clear();
		auto lock=computations.unique_lock();
		computations.clear();
	}
	bool terminating() const {return should_terminate;}
//to be called at initialization or when a new input_file is provided through the UI
	void load_computations(const string& input_file,const CSVSchema& schema, int max_computations_in_template) {
		ifstream s{input_file};
		packed_computations.load(s,schema,max_computations_in_template);
		ui->loaded_computations(input_file);
	}
//unpack computation templates into computations and remove those already processed
	void unpack_computations_and_remove_already_processed(int min_threshold, int max_threshold, const optional<SimpleDatabaseView>& db_view,const string& output_dir,const CSVSchema& schema, ThreadUIHandle& thread_ui) {	
		++unpacking_threads;
		UnpackedComputations unpacked;
		while (computations.size()+unpacked.size()<min_threshold && !packed_computations.empty() && !should_terminate) {
			thread_ui.unpacking_computations();
			auto primary_ids=packed_computations.unpack(max_threshold, unpacked);
			thread_ui.unpacked_computations(unpacked.size());
			if (!unpacked.size()) continue;
			if (db_view) {
				int eliminated=unpacked.eliminate_computations_in_db(db_view.value(),primary_ids);
				thread_ui.removed_computations_in_db(eliminated);
			}
			int eliminated=unpacked.eliminate_precalculated(output_dir,schema,should_terminate);
	    thread_ui.removed_precalculated(eliminated);
			auto lock=computations.unique_lock();
			computations.insert(std::move(unpacked));
		}
		--unpacking_threads;
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
	void attach_user_interface(UserInterface* interface=&NoUserInterface::singleton()) {
		ui=interface;
		ui->update_bad(bad.summary());
	}
	void mark_as_bad(Computation computation, megabytes memory_limit) {	
		bad.insert(std::move(computation),memory_limit);		
	}
	void add_computations_to_do(AssignedComputations& assigned_computations, int computations_per_process, megabytes memory_limit) {
			int to_add=max(0,computations_per_process- static_cast<int>(assigned_computations.size()));
			auto resurrected=bad.extract_within_memory_limit(memory_limit, to_add);
			to_add-=resurrected.size();
			if (resurrected.size()) ui->resurrected(resurrected.size(),memory_limit);
			ui->update_bad(bad.summary());
			assigned_computations.insert(resurrected.begin(),resurrected.end());
			if (!computations.empty()) {
				auto lock=computations.unique_lock();
				computations.assign(to_add,assigned_computations);
			}
			if (assigned_computations.size()) ui->assigned_computations(assigned_computations.size());
	}
	void tick() {
		int removed=to_valhalla(bad);
		abandoned+=removed;
		if (removed) ui->aborted_computations(removed);
		else ui->tick(packed_computations.size(), computations.size(),bad.size(),abandoned);
	}
	void print_computations() 	 {
		auto lock=computations.unique_lock();
		for (auto& x: computations) ui->print_computation(x);
		computations.clear();
	}
	int no_computations() const {
		return computations.size();
	}
	megabytes lowest_effective_memory_limit() {
		if (!computations.empty() || !packed_computations.empty()) return 0;
		return bad.lowest_effective_memory_limit();
	}
	bool finished() {
		auto result=computations.empty() && !unpacking_threads && packed_computations.empty()  && bad.empty();
		return result || should_terminate;
	}
};





#endif
