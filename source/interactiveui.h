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
	WindowHandle status_window, msg_window, bad_window, memory_window, input_window;
	Controller* controller;
	void print_status(int packed_computations, int unpacked_computations, int bad, int abandoned) {
		auto overall_computations=packed_computations+unpacked_computations+bad;
		status_window<<clear<<"Packed computations: "<<packed_computations<<"\tUnpacked computations: "<<unpacked_computations<<"\tAborted computations to retry: "<<bad<<		"\tTotal unassigned computations: "<<overall_computations<<"\tAbandoned computations: "<<abandoned<<release;
	}
	void print_key_mappings() {
		input_window<<clear<<"(press Q to quit, L to load new computations, arrow up/down to change total memory limit, PGUP/PGDOWN to change perthread lower memory limit)"<<release;	
	}
public:
	InteractiveUserInterface() : layout{screen} {
 		status_window=layout.create_window(1,WindowType::CHILD);
 		msg_window=layout.create_window(1,WindowType::CHILD);
		auto bad_and_memory_windows=layout.create_windows(1,WindowType::CHILD,WindowType::CHILD);
		bad_window=bad_and_memory_windows[0];
		memory_window=bad_and_memory_windows[1];
		input_window=layout.create_window(1,WindowType::CHILD);
		auto header=layout.create_window(1,WindowType::CHILD);
		header<<clear<<"THREAD\tMEMORY"<<release;
		print_key_mappings();
	}
	void update_bad(const vector<pair<megabytes,int>>& memory_limits) override {
		bad_window<<clear<<"bad:\t";
		if (memory_limits.empty()) bad_window<<"none";
		else for (auto& p: memory_limits) bad_window<<p.first<<"MB:"<<p.second<<" ";
		bad_window<<release;
	}
	void loaded_computations(const string& input_file) override {
		msg_window<<clear<<"loaded new computations from "<<input_file<<release;
	}
	void unpacked_computations()  override {
		msg_window<<clear<<"computations unpacked"<<release;
	}
	void removed_computations_in_db(int eliminated)  override {
		msg_window<<clear<<"eliminated "<<eliminated<<" computations from database"<<release;	
	}
	void removed_precalculated(int eliminated)  override {
		msg_window<<clear<<"eliminated "<<eliminated<<" already-performed computations from work directory"<<release;		
	}
	void aborted_computations(int eliminated)  override {
		msg_window<<clear<<"moved "<<eliminated<<" to valhalla"<<release;		
	}
	void resurrected(int resurrected, megabytes memory_limit)  override {
		msg_window<<clear<<"resurrected "<<resurrected<<" computations to thread with "<<memory_limit<<"MB"<<release;			
	}
	void assigned_computations(int assigned)  override {
		msg_window<<clear<<"assigned "<<assigned<<" computations"<<release;
	}
	void tick(int packed_computations, int unpacked_computations, int bad, int abandoned) override {
		print_status(packed_computations,unpacked_computations,bad, abandoned);
		optional<int> key_pressed;
		if (controller) 
			while ((key_pressed=screen.key_pressed()))
				controller->on_key_pressed(key_pressed.value());
		if (screen.size_changed()) controller->on_screen_size_changed(screen.dimensions().width, screen.dimensions().height);
		screen.refresh();
	}
	void attach_controller(Controller* controller=nullptr) override {
		this->controller=controller;
	}	
	string get_filename() override {
		input_window<<clear<<"Filename: "<<refresh;
		auto filename=screen.get_string(input_window);
		auto input_file=boost::filesystem::path(filename);			
		while (!boost::filesystem::exists(input_file) || !boost::filesystem::is_regular_file(input_file)) {
			input_window<<clear<<filename<<" does not exist; enter new filename:"<<refresh;
			filename=screen.get_string(input_window);
			input_file=boost::filesystem::path(filename);						
		}
		print_key_mappings();	
		return filename;
	}
	void print_computation(const Computation& computation) override {	}
	void display_memory_limit(pair<megabytes,megabytes> memory) override {
		memory_window<<clear<<"Total limit: "<<memory.first<<"MB\tLower limit per thread: "<<memory.second<<"MB"<<release;
	}
	unique_ptr<ThreadUIHandle> make_thread_handle(int thread) override;
};

class ThreadInteractiveUIHandle : public ThreadUIHandle {
	WindowHandle status_window;
	WindowHandle msg_window;
	int thread;
	void print_thread_id(megabytes memory_limit) {
		status_window<<clear<<thread<<"\t"<<memory_limit<<"MB\t";
	}
public:
	ThreadInteractiveUIHandle(const array<WindowHandle,2>& windows,int thread) : status_window{windows[0]}, msg_window{windows[1]}, thread{thread} {	
	}
 	void computations_added(int assigned_computations, megabytes memory) override {
		print_thread_id(memory);
		if (assigned_computations) {
			status_window<<"working on "<<assigned_computations<<" computations"<<release;
		}
		else status_window<<release; 
 	}
	void thread_started(megabytes memory) override {
		print_thread_id(memory);
		status_window<<"waiting computations..."<<release;
	}
	void thread_stopped(megabytes memory) override {
		print_thread_id(memory);
		status_window<<"(paused)"<<release;
	}
	void bad_computation(const Computation& computation, megabytes memory_limit) override {
		msg_window<<clear<<"could not complete "<<computation.to_string()<<" with "<<memory_limit<<" MB of memory"<<release;
	}
};

unique_ptr<ThreadUIHandle> InteractiveUserInterface::make_thread_handle(int thread) {	
	return make_unique<ThreadInteractiveUIHandle>(layout.create_windows(1,WindowType::CHILD,WindowType::CHILD), thread);
}


#endif
