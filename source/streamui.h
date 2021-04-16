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

#ifndef STREAM_UI_H
#define STREAM_UI_H
#include "ui.h"

class StreamUserInterface : public UserInterface {
	friend class ThreadStreamUIHandle;
	ostream& os;
	mutable mutex lock;
	int pos=0;
public:
	StreamUserInterface(ostream& os) : os{os} {}
	void loaded_computations(const string& input_file) override {
		unique_lock<mutex> lck{lock};
		os<<"loaded computations from "<<input_file<<endl;
	}
	void unpacked_computations() override {
		unique_lock<mutex> lck{lock};
		os<<"unpacked computations"<<endl;
	}
	void removed_computations_in_db(int eliminated) override {
		unique_lock<mutex> lck{lock};
		os<<"eliminated "<<eliminated<<" computations from database"<<endl;	
	}
	void removed_precalculated(int eliminated) override {
		unique_lock<mutex> lck{lock};
		os<<"eliminated "<<eliminated<<" already-performed computations from work directory"<<endl;
	}
	void aborted_computations(int eliminated) override {
		unique_lock<mutex> lck{lock};
		os<<eliminated<<" moved to valhalla"<<endl;
	}
	void resurrected(int resurrected,  megabytes memory_limit) override {
		unique_lock<mutex> lck{lock};
		os<<"resurrected "<<resurrected<<" computations to thread with "<<memory_limit<<"MB"<<endl;			
	}
	void assigned_computations(int assigned) override {
		unique_lock<mutex> lck{lock};
		os<<"assigned "<<assigned<<" computations to a process"<<endl;
	}
	void tick(int packed_computations, int unpacked_computations, int bad, int abandoned) override {
    static char bars[] = { '/', '-', '\\', '|' };	
		unique_lock<mutex> lck{lock};
		os<<bars[pos]<<"\r";
		os.flush();
		pos = (pos + 1) % 4;
	}	
	void update_bad(const vector<pair<megabytes,int>>& memory_limits) override {}
	unique_ptr<ThreadUIHandle> make_thread_handle(int thread);
	void print_computation(const Computation& computation) override {
		unique_lock<mutex> lck{lock};
		os<<computation.to_string()<<endl;
	}
	void detach() const override {
		unique_lock<mutex> lck{lock};	//block until any ongoing operation is finished
	}
};

class ThreadStreamUIHandle : public ThreadUIHandle {
	const StreamUserInterface* ui;
	int thread;
	void print_thread_id() const {
		ui->os<<thread<<":\t";
	}
public:
	ThreadStreamUIHandle(const StreamUserInterface* ui, int thread) : ui{ui}, thread{thread} {}
 	void computations_added(int assigned_computations, megabytes memory_limit) override {
		if (assigned_computations) {
			unique_lock<mutex> lck{ui->lock};
			print_thread_id();
			ui->os<<"process with "<<memory_limit<<"MB started with "<<assigned_computations<<" computations"<<endl; 	
		}
 	}
	void thread_started(megabytes memory) override {
		unique_lock<mutex> lck{ui->lock};
		print_thread_id();
		ui->os<<"thread started with a limit of "<<memory<<"MB"<<endl;
	}
	void thread_stopped(megabytes memory) override {
		unique_lock<mutex> lck{ui->lock};
		print_thread_id();
		ui->os<<"stopped thread with a limit of "<<memory<<"MB"<<endl;
	}
	void bad_computation(const Computation& computation, megabytes memory_limit) override {
		unique_lock<mutex> lck{ui->lock};
		print_thread_id();
		ui->os<<"could not complete "<<computation.to_string()<<" with "<<memory_limit<<" MB of memory"<<endl;	
	}
};

unique_ptr<ThreadUIHandle> StreamUserInterface::make_thread_handle(int thread)  {
	return make_unique<ThreadStreamUIHandle>(this, thread);
}


#endif
