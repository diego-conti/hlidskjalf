#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H
#include <condition_variable>
#include "stdincludes.h"
#include "computationrunner.h"


class MemoryManager {
	mutex suspension_mtx;		//mutex used to lock access to all the data in this class
	megabytes allocated=0,limit,base_memory_limit;
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
		if (finished()) {
			suspension_mtx.unlock();
			return 0;
		}
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
	MemoryUse increase_memory_limit(megabytes total_memory_delta, megabytes base_memory_delta) {
		unique_lock<mutex> lck{suspension_mtx};
		if (limit+total_memory_delta>0) limit+=total_memory_delta;
		base_memory_limit=std::max(base_memory_limit+base_memory_delta,ComputationRunner::singleton().lowest_effective_memory_limit());
		if (total_memory_delta>0 || base_memory_delta<0) suspension.notify_one();
		return {limit,base_memory_limit,allocated};
	}
};
#endif
