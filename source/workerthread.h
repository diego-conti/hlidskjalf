#ifndef WORKER_THREAD_H
#define WORKER_THREAD_H
#include "computationrunner.h"
#include <condition_variable>
#include <atomic>

class MemoryManager {
	mutex suspension_mtx;		//mutex used to lock access to all the data in this class
	megabytes allocated,limit,base_memory_limit;
	std::condition_variable suspension;
	int suspended_threads=0;
	
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
	megabytes start_and_unlock(megabytes memory) {
		if (allocated+memory<=limit) {
			allocated+=memory;
			suspension_mtx.unlock();
			ComputationRunner::singleton().get_ui().thread_started(memory);
			return memory;
		}
		else {
			suspension_mtx.unlock();
			return wait_and_allocate();
		}	
	}	
public:
	static MemoryManager& singleton() {
		static MemoryManager memory_manager;
		return memory_manager;
	}
	void release(megabytes n) {
		unique_lock<mutex> lck{suspension_mtx};
		allocated-=n;
		++suspended_threads;
		suspension.notify_one();
		lck.unlock();
		ComputationRunner::singleton().get_ui().thread_stopped(n);
	}
	megabytes start() {
		suspension_mtx.lock();
		return start_and_unlock(base_memory_limit);
	}
	megabytes start_large_thread() {
		suspension_mtx.lock();
		return start_and_unlock(max(base_memory_limit,limit-allocated));
	}	
	megabytes wait_and_allocate() {
		unique_lock<mutex> lck{suspension_mtx};
		while (true) {
			suspension.wait_for(lck,std::chrono::seconds(10));
			if (ComputationRunner::singleton().finished()) return 0;
			auto request= to_request();
			if (request) {
				allocated+=request.value();
				--suspended_threads; 
				lck.unlock();
				ComputationRunner::singleton().get_ui().thread_started(request.value());
				return request.value();
			}
		}		
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
	thread thread_;
	list<Computation> computations_to_do;	
	
	void main_loop(megabytes memory_limit) {
		while (memory_limit) {
			loop_compute(memory_limit);	
			MemoryManager::singleton().release(memory_limit);
			memory_limit=MemoryManager::singleton().wait_and_allocate();
		}
	}

	void loop_compute(megabytes memory_limit) {
		while (true) {
			ComputationRunner::singleton().add_computations_to_do(computations_to_do,memory_limit);
			if (computations_to_do.empty()) return;
			computations_to_do=ComputationRunner::singleton().compute(process_id_as_string,computations_to_do,memory_limit);
			if (!computations_to_do.empty()) {
				ComputationRunner::singleton().mark_as_bad(computations_to_do.front(),memory_limit);
				computations_to_do.pop_front();
			}
			if (ComputationRunner::singleton().large_thread(memory_limit)) return;
		}	
	}
	WorkerThread(const WorkerThread&) =delete;
	WorkerThread(WorkerThread&&) =delete;
public:
	WorkerThread() : 
		process_id{ComputationRunner::singleton().assign_id()}, 
		process_id_as_string{to_string(process_id)},
		thread_{&WorkerThread::main_loop,this,MemoryManager::singleton().start()}
	{}
	WorkerThread(large_tag_t) : 
		process_id{ComputationRunner::singleton().assign_id()}, 
		process_id_as_string{to_string(process_id)},
		thread_{&WorkerThread::main_loop,this,MemoryManager::singleton().start_large_thread()}	
	{}
	void join() {thread_.join();}
};

class WorkerThreads {
	vector<unique_ptr<WorkerThread>> threads;
public:
	WorkerThreads(const Parameters& parameters) {	 
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
			threads.push_back(make_unique<WorkerThread>());
		threads.push_back(make_unique<WorkerThread>(large_tag));
	}
	void join() {
		for (auto& t: threads) t->join();
	}
};



#endif
