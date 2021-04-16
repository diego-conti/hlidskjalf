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

#include "exception.h"
#include "computationrunner.h"
#include "parameters.h"
#include "interactiveui.h"
#include "streamui.h"
#include "workerthread.h"

using namespace std;


void loop_ui(future<void> terminateFuture) {
	future_status status;
	do {
		status=terminateFuture.wait_for(std::chrono::milliseconds(200));
		ComputationRunner::singleton().tick();
	} while (status!=future_status::ready);
}

thread create_ui_thread(future<void>&& terminate_signal) {
	thread ui_thread{loop_ui,move(terminate_signal)};	
	return ui_thread;
}


void launch_threads(const Parameters& parameters, UserInterface* ui) {
	try {
		promise<void> terminate_signal;	
		thread ui_thread=create_ui_thread(terminate_signal.get_future());
		WorkerThreads worker_threads{parameters,ui};
		worker_threads.join();
		terminate_signal.set_value();
		ui_thread.join();
	}
	catch (const Exception& e) {
		cout<<e.what()<<endl;
		throw;
	}
}


string command_line(char** begin, char** end) {
	string result;
	while (begin!=end) result+=*begin++ + " "s;
	return result;
}

int run(const Parameters& parameters,UserInterface* ui) {
	try {
		if (parameters.operating_mode==OperatingMode::BATCH_MODE)	{
			ComputationRunner::singleton().init(parameters);
			ComputationRunner::singleton().print_computations();
		}
		else launch_threads(parameters,ui);		
	}
	catch (const Exception& e) {
		cerr<<e.what()<<endl;
		return 1;
	}
	return 0;
}

unique_ptr<UserInterface> create_ui(const Parameters& parameters) {
	if (parameters.console) return make_unique<StreamUserInterface>(cout);
	else return  make_unique<InteractiveUserInterface>();
}

int create_ui_and_run(const Parameters& parameters, const string& command_line) {
	cout<<"BEGIN: "<<command_line<<endl;
	auto ui=create_ui(parameters);
	ComputationRunner::singleton().attach_user_interface(ui.get());
	auto return_code=run(parameters,ui.get());
	ComputationRunner::singleton().attach_user_interface();
	boost::filesystem::remove_all(parameters.communication_parameters.huginn);
	cout<<(return_code? "TERMINATED: " : "END: ");
	cout<<command_line<<endl;	
	return return_code;
}



int main(int argv, char** argc) {
	try {
		Parameters parameters=command_line_parameters(argv,argc);		
		return create_ui_and_run(parameters, command_line(argc,argc+argv));
	}
	catch (const Exception& e) {
		cerr<<e.what()<<endl;
		return 1;
	}
}


