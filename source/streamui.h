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
#include "synchronizedcomputations.h"

class StreamUserInterface : public UserInterface {
	ostream& os;
	mutable mutex lock;
	mutable int pos=0;
public:
	StreamUserInterface(ostream& os) : os{os} {}
	void computations_added_to_thread(int overall_unassigned_computations, int assigned_computations, megabytes memory_limit) const override {
		unique_lock<mutex> lck{lock};
		if (assigned_computations) 
			os<<"process with "<<memory_limit<<"MB started with "<<assigned_computations<<" computations, ";
		os<<overall_unassigned_computations<<" unassigned computations remain"<<endl;
	}	
	void bad_computation(const Computation& computation, megabytes memory_limit) const override {
		unique_lock<mutex> lck{lock};
		os<<"could not complete "<<computation.to_string()<<" with "<<memory_limit<<" MB of memory"<<endl;
	}
	void removed_computations_in_db(int computations) const override {
		unique_lock<mutex> lck{lock};
		os<<"eliminated "<<computations<<" computations from database"<<endl;
	}	
	void removed_precalculated(int computations) const override {
		unique_lock<mutex> lck{lock};
		os<<computations<<" computations remain after eliminating those already calculated"<<endl;
	}	
	void total_computations(int computations) const override {
		unique_lock<mutex> lck{lock};
		os<<"Total number of computations loaded: "<<computations<<endl;	
	}	
	void unpacked_computations(int computations) const override {
		unique_lock<mutex> lck{lock};
		os<<"processing "<<computations<<" computations"<<endl;	
	}	
	void aborted_computations(int computations) const override {
		unique_lock<mutex> lck{lock};
		os<<computations<<" moved to valhalla"<<endl;
	}
	void tick() const override {
    static char bars[] = { '/', '-', '\\', '|' };	
		unique_lock<mutex> lck{lock};
		os<<bars[pos]<<"\r";
		os.flush();
		pos = (pos + 1) % 4;
	}
	void print_computation(const Computation& computation) const override {
		unique_lock<mutex> lck{lock};
		os<<computation.to_string()<<endl;
	}
};
#endif
