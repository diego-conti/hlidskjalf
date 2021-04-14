#ifndef WORKER_THREAD_H
#define WORKER_THREAD_H
#include "computationrunner.h"
#include <condition_variable>
#include "ui.h"

class MemoryManager {
	mutex suspension_mtx;		//mutex used to lock access to all the data in this class
	megabytes allocated,limit,base_memory_limit;
	std::condition_variable suspension;
	int suspended_threads=0;
	
	bool finished() const {
		return ComputationRunner::singleton().finished();
	}
	//return the megabytes that should be allocated or nullopt if not enough memory is available to start another process
	optional<megabytes> to_request() const {
		auto max_request=limit-allocated;
		auto lowest=max(ComputationRunner::singleton().lowest_effective_memory_limit(),base_memory_limit);
		if (max_request<=lowest) return nullopt;
		if (suspended_threads==1) return max_request;
		if (lowest>limit/3) return max_request;	//there is no room for another thread, so give all memory to this one
		return min(lowest*2,max_request);				
	}
	MemoryManager()=default;
	//start a thread with allocated amount of memory, or wait for enough memory to be freed. If memory is nullopt, just wait.
	megabytes start_and_unlock(optional<megabytes> memory) {
		if (finished()) return 0;
		if (memory && allocated+memory.value()<=limit) {
			allocated+=memory.value();
			--suspended_threads; 
			suspension_mtx.unlock();
			return memory.value();
		}
		else return wait_and_allocate();		
	}	
	megabytes wait_and_allocate() {
		unique_lock<mutex> lck{suspension_mtx, std::adopt_lock};
		while (true) {
			suspension.wait_for(lck,std::chrono::seconds(10));
			if (finished()) return 0;
			auto request= to_request();
			if (request) return start_and_unlock(request.value());			
		}
	}	
	void release(megabytes n) {
		unique_lock<mutex> lck{suspension_mtx};
		allocated-=n;
		++suspended_threads;
		suspension.notify_one();
	}
public:
	static MemoryManager& singleton() {
		static MemoryManager memory_manager;
		return memory_manager;
	}
	megabytes start() {
		suspension_mtx.lock();
		++suspended_threads;
		return start_and_unlock(base_memory_limit);
	}
	megabytes start_large_thread() {
		suspension_mtx.lock();
		++suspended_threads;
		return start_and_unlock(max(base_memory_limit,limit-allocated));
	}	
	megabytes resize(megabytes actual_size) {
		release(actual_size);
		suspension_mtx.lock();
		return start_and_unlock(to_request());
	}
	void set_total_limit(megabytes n) {
		unique_lock<mutex> lck{suspension_mtx};
		limit=n;
	}
	void set_base_memory_limit(megabytes n) {
		unique_lock<mutex> lck{suspension_mtx};
		base_memory_limit=n;
	}
};

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
			computations_to_do=ComputationRunner::singleton().compute(process_id_as_string,computations_to_do,memory_limit);
			if (!computations_to_do.empty()) {
				auto bad=computations_to_do.begin();
				ui_handle->bad_computation(*bad,memory_limit);
				ComputationRunner::singleton().mark_as_bad(*bad,memory_limit);
				computations_to_do.erase(bad);
			}
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
