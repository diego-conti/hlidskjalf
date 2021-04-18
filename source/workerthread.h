#ifndef WORKER_THREAD_H
#define WORKER_THREAD_H
#include "memorymanager.h"
#include "ui.h"

constexpr class large_tag_t {} large_tag;

class WorkerThread {
	int process_id;
	string process_id_as_string;
	unique_ptr<ThreadUIHandle> ui_handle;
	thread thread_;
	AssignedComputations computations_to_do;	

	enum class LoopExitCondition {REDUCE_MEMORY_LIMIT, RAISE_MEMORY_LIMIT};
	
	void main_loop(megabytes memory_limit) {
		while (memory_limit) {
			ui_handle->thread_started(memory_limit);
			auto loop_exit_condition=loop_compute(memory_limit);	
			ui_handle->thread_stopped(memory_limit);
			memory_limit= MemoryManager::singleton().resize(memory_limit);
		}
	}

	LoopExitCondition loop_compute(megabytes memory_limit) {
		while (true) {
			ComputationRunner::singleton().add_computations_to_do(computations_to_do,memory_limit);
			if (computations_to_do.empty()) return LoopExitCondition::RAISE_MEMORY_LIMIT;
			ui_handle->computations_added(computations_to_do.size(),memory_limit);
			int no_computations=computations_to_do.size();
			computations_to_do=ComputationRunner::singleton().compute(process_id_as_string,computations_to_do,memory_limit);
			if (!computations_to_do.empty()) {
				auto bad=computations_to_do.begin();
				ui_handle->bad_computation(*bad,memory_limit);
				ComputationRunner::singleton().mark_as_bad(*bad,memory_limit);
				computations_to_do.erase(bad);
			}
			else ui_handle->finished_computations(no_computations-computations_to_do.size(),memory_limit);
			if (ComputationRunner::singleton().large_thread(memory_limit)) return LoopExitCondition::REDUCE_MEMORY_LIMIT;
		}	
	}
	WorkerThread(const WorkerThread&) =delete;
	WorkerThread(WorkerThread&&) =delete;
public:
	WorkerThread(UserInterface* ui) : 
		process_id{ComputationRunner::singleton().assign_id()}, 
		process_id_as_string{to_string(process_id)},
		ui_handle{ui->make_thread_handle(process_id)},
		thread_{&WorkerThread::main_loop,this,MemoryManager::singleton().start()}
	{}
	WorkerThread(UserInterface* ui,large_tag_t) : 
		process_id{ComputationRunner::singleton().assign_id()}, 
		process_id_as_string{to_string(process_id)},
		ui_handle{ui->make_thread_handle(process_id)},
		thread_{&WorkerThread::main_loop,this,MemoryManager::singleton().start_large_thread()}	
	{}
	void join() {thread_.join();}
};

class WorkerThreads {
	vector<unique_ptr<WorkerThread>> threads;
public:
	WorkerThreads(const Parameters& parameters, UserInterface* ui) {	 
		try {
			ComputationRunner::singleton().init(parameters);
			MemoryManager::singleton().set_total_limit(parameters.computation_parameters.total_memory_limit);
			MemoryManager::singleton().set_base_memory_limit(parameters.computation_parameters.base_memory_limit);
		}
		catch (Exception& e) {
			cout<<e.what()<<endl;
			throw;
		}
		for (int i=1;i<parameters.computation_parameters.nthreads;++i)
			threads.push_back(make_unique<WorkerThread>(ui));
		threads.push_back(make_unique<WorkerThread>(ui,large_tag));
	}
	void join() {
		for (auto& t: threads) t->join();
	}
};



#endif
