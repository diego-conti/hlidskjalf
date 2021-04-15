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

#ifndef INTERACTIVE_UI_H
#define INTERACTIVE_UI_H
#include "ui.h"
#include "tui/layout.h"

class InteractiveUserInterface : public UserInterface {
	friend class ThreadInteractiveUIHandle;
	Screen screen;
	VerticalLayout layout;
	mutable WindowHandle status_window;
	mutable WindowHandle msg_window;
	void print_status(int packed_computations, int unpacked_computations, int bad) const {
		auto overall_computations=packed_computations+unpacked_computations+bad;
		status_window<<clear<<"Packed computations:\t"<<packed_computations<<"\tUnpacked computations:\t"<<unpacked_computations<<"\tAborted computations to retry:\t"<<bad<<		"\tTotal unassigned computations:\t"<<overall_computations<<release;
	}
public:
	InteractiveUserInterface() : layout{screen}, status_window{layout.create_window(1,WindowType::CHILD)}, msg_window{layout.create_window(1,WindowType::STANDALONE)} {
		auto header=layout.create_window(2,WindowType::STANDALONE);
		header<<clear<<"Press Q to quit\nTHREAD\tMEMORY"<<refresh;
	}
	void loaded_computations(const string& input_file,int packed_computations, int unpacked_computations, int bad) const override {
		print_status(packed_computations,unpacked_computations,bad);
		msg_window<<clear<<"loaded new computations from "<<input_file<<refresh;
	}
	void unpacked_computations(int packed_computations, int unpacked_computations, int bad) const override {
		print_status(packed_computations,unpacked_computations,bad);
		msg_window<<clear<<"computations unpacked"<<refresh;
	}
	void removed_computations_in_db(int eliminated,int packed_computations, int unpacked_computations, int bad) const override {
		print_status(packed_computations,unpacked_computations,bad);
		msg_window<<clear<<"eliminated "<<eliminated<<" computations from database"<<refresh;	
	}
	void removed_precalculated(int eliminated,int packed_computations, int unpacked_computations, int bad) const override {
		print_status(packed_computations,unpacked_computations,bad);
		msg_window<<clear<<"eliminated "<<eliminated<<" already-performed computations from work directory"<<refresh;		
	}
	void aborted_computations(int eliminated,int packed_computations, int unpacked_computations, int bad) const override {
		print_status(packed_computations,unpacked_computations,bad);
		msg_window<<clear<<"moved "<<eliminated<<" to valhalla"<<refresh;		
	}
	void resurrected(int resurrected,int packed_computations, int unpacked_computations, int bad) const override {
		print_status(packed_computations,unpacked_computations,bad);
		msg_window<<clear<<"resurrected "<<resurrected<<" computations"<<refresh;			
	}
	void assigned_computations(int assigned,int packed_computations, int unpacked_computations, int bad) const override {
		print_status(packed_computations,unpacked_computations,bad);
		msg_window<<clear<<"assigned "<<assigned<<" computations"<<refresh;				
	}
	void tick() const override {
		if (screen.user_has_pressed('q')) throw 0;
		screen.refresh();
	}
	
	void print_computation(const Computation& computation) const override {
	}
	void detach() const override {}

	unique_ptr<ThreadUIHandle> make_thread_handle(int thread) override;
};

class ThreadInteractiveUIHandle : public ThreadUIHandle {
	WindowHandle status_window;
	WindowHandle msg_window;
	int thread;
	void print_thread_id(megabytes memory_limit) const {
		status_window<<clear<<thread<<"\t"<<memory_limit<<"MB\t";
	}
public:
	ThreadInteractiveUIHandle(const array<WindowHandle,2>& windows,int thread) : status_window{windows[0]}, msg_window{windows[1]}, thread{thread} {	
	}
 	void computations_added(int assigned_computations, megabytes memory) const override {
		print_thread_id(memory);
		if (assigned_computations) {
			status_window<<"working on "<<assigned_computations<<" computations"<<release;
		}
 	}
	void thread_started(megabytes memory) const override {
		print_thread_id(memory);
		status_window<<"started"<<release;
	}
	void thread_stopped(megabytes memory) const override {
		print_thread_id(memory);
		status_window<<"stopped"<<release;
	}
	void bad_computation(const Computation& computation, megabytes memory_limit) const override {
		msg_window<<clear<<"could not complete "<<computation.to_string()<<" with "<<memory_limit<<" MB of memory"<<refresh;
	}
};

unique_ptr<ThreadUIHandle> InteractiveUserInterface::make_thread_handle(int thread) {	
	return make_unique<ThreadInteractiveUIHandle>(layout.create_windows(1,WindowType::CHILD,WindowType::STANDALONE), thread);
}


#endif
