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
		if (n==0) return begin;
		assert(n>0);
		auto result=begin;
		std::advance(result,n);
		return result == begin ? end : result;
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
			if (p.first>memory_limit) 
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
		for (auto& p:computations_by_memory_limit) if (p.first<lowest) lowest=p.first;
		return lowest;
	}
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
	void eliminate_computations_in_db(const SimpleDatabaseView& db_view, const set<int>& group_orders) {
		auto eliminate_function=[this] (int primary_input, const FieldsInDB& secondary_inputs, const FieldsInDB& data) {
				Computation computation{primary_input,static_cast<CSVLine>(secondary_inputs)};
				computations.erase(computation);			
		};
		db_view.iterate_through_entries(eliminate_function,group_orders);
	}
	void eliminate_precalculated(const string& output_dir,const CSVSchema& schema) {
    for (auto& x : boost::filesystem::directory_iterator(output_dir))
    	if (boost::filesystem::is_regular_file(x)) {
				std::ifstream f{x.path().native()};
    		eliminate_computations<CSVReader>(f,computations,schema);
    	}	
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
	int size() const {return computations.size();}
	bool empty() const {return computations.empty();}
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
			CSVLine input{get_line_with_balanced_curly_braces(s)};		
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
};

class SynchronizedComputations {
	AbortedComputations bad;
	UnpackedComputations computations;
	PackedComputations packed_computations;
	const UserInterface* ui=&NoUserInterface::singleton();

	void synchronized_add_computations_to_do(AssignedComputations& assigned_computations, int computations_per_process, megabytes memory_limit) {
	}
protected:
//to be called at initialization or when a new input_file is provided through the UI
	void load_computations(const string& input_file,const CSVSchema& schema, int max_computations_in_template) {
		ifstream s{input_file};
		packed_computations.load(s,schema,max_computations_in_template);
		ui->total_computations(packed_computations.size());
	}
//unpack computation templates into computations and remove those already processed
	void unpack_computations_and_remove_already_processed(int min_threshold, int max_threshold, const optional<SimpleDatabaseView>& db_view,const string& output_dir,const CSVSchema& schema) {	
		auto lock=computations.unique_lock();
		while (computations.size()<min_threshold && !packed_computations.empty()) {
			auto primary_ids=packed_computations.unpack(max_threshold, computations);
			ui->unpacked_computations(computations.size());
			if (db_view) {
				int size=computations.size();
				computations.eliminate_computations_in_db(db_view.value(),primary_ids);
				int eliminated=size-computations.size();
				ui->removed_computations_in_db(eliminated);
			}
			computations.eliminate_precalculated(output_dir,schema);
	    ui->removed_precalculated(computations.size());
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
	void display_total_computations() {
		ui->total_computations(computations.size()+packed_computations.size()+bad.size());
	}
public:
	void attach_user_interface(const UserInterface* interface=&NoUserInterface::singleton()) {
		if (ui) ui->detach();
		ui=interface;
	}
	void mark_as_bad(Computation computation, megabytes memory_limit) {	
		bad.insert(std::move(computation),memory_limit);		
	}
	void add_computations_to_do(AssignedComputations& assigned_computations, int computations_per_process, megabytes memory_limit) {
			int to_add=max(0,computations_per_process- static_cast<int>(assigned_computations.size()));
			auto resurrected=bad.extract_within_memory_limit(memory_limit, to_add);
			to_add-=resurrected.size();
			assigned_computations.insert(resurrected.begin(),resurrected.end());
			if (!computations.empty()) {
				auto lock=computations.unique_lock();
				computations.assign(to_add,assigned_computations);
			}
	}
	void flush_bad() {
		int removed=to_valhalla(bad);
		if (removed) ui->aborted_computations(removed);
		else ui->tick();
	}
	void print_computations() const {
	//TODO unpack computations and print everything to the ui, which presumably is a streamui
		throw 0;
	}
	int no_computations() const {
		return computations.size();
	}
	megabytes lowest_effective_memory_limit() {
		if (!computations.empty() || !packed_computations.empty()) return 0;
		return bad.lowest_effective_memory_limit();
	}
	bool finished() {
		return computations.empty() && packed_computations.empty() && bad.empty();
	}
};





#endif
