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
#include "streamui.h"
#include "workerthread.h"

using namespace std;


void loop_flush_bad(future<void> terminateFuture) {
	future_status status;
	do {
		status=terminateFuture.wait_for(std::chrono::seconds(10));
		ComputationRunner::singleton().check_memory_and_flush_bad();
	} while (status!=future_status::ready);
}

thread create_flush_thread(future<void>&& terminate_signal) {
	thread flush_thread{loop_flush_bad,move(terminate_signal)};	
	return flush_thread;
}


void launch_threads(const Parameters& parameters) {
	try {
		promise<void> terminate_signal;	
		thread flush_thread=create_flush_thread(terminate_signal.get_future());
		WorkerThreads worker_threads{parameters};
		worker_threads.join();
		terminate_signal.set_value();
		flush_thread.join();
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

int main(int argv, char** argc) {
	try {
		Parameters parameters=command_line_parameters(argv,argc);		
		cout<<"BEGIN: "<<command_line(argc,argc+argv)<<endl;
		ComputationRunner::singleton().create_user_interface<StreamUserInterface>(cout);
		if (parameters.operating_mode==OperatingMode::BATCH_MODE)	{
			ComputationRunner::singleton().init(parameters);
			ComputationRunner::singleton().print_computations();
		}
		else 
			launch_threads(parameters);
	
		boost::filesystem::remove_all(parameters.communication_parameters.huginn);
		cout<<"END: "<<command_line(argc,argc+argv)<<endl;	
		return 0;
	}
	catch (const Exception& e) {
		cerr<<e.what()<<endl;
		return 1;
	}
}


