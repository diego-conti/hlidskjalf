#ifndef UI_H
#define UI_H
#include "stdincludes.h"

class Computation;

class ThreadUIHandle  {
public:
	virtual void computations_added(int assigned_computations, megabytes memory_limit) =0;
	virtual void thread_started(megabytes memory) =0;
	virtual void thread_stopped(megabytes memory) =0;
	virtual void bad_computation(const Computation& computation, megabytes memory_limit)  =0;	
};

class UserInterface {
public:
	virtual void loaded_computations(const string& input_file) =0;
	virtual void unpacked_computations() =0;
	virtual void removed_computations_in_db(int eliminated) =0;
	virtual void removed_precalculated(int eliminated) =0;
	virtual void aborted_computations(int eliminated) =0;	
	virtual void resurrected(int resurrected, megabytes memory_limit) =0;
	virtual void assigned_computations(int assigned) =0;	
	virtual void tick(int packed_computations, int unpacked_computations, int bad, int abandoned)=0;
	virtual void print_computation(const Computation& computation) =0;
	virtual void update_bad(const vector<pair<megabytes,int>>& memory_limits)=0;
	
	virtual void detach() const =0;
		
	virtual unique_ptr<ThreadUIHandle> make_thread_handle(int thread) =0;	//the handle becomes invalid when the UI is destroyed
};



class ThreadNoUIHandle : public ThreadUIHandle {
public:
	void computations_added(int assigned_computations, megabytes memory_limit)  {}
	void thread_started(megabytes memory)  {}
	void thread_stopped(megabytes memory)  {}
	void bad_computation(const Computation& computation, megabytes memory_limit)  {}
};

class NoUserInterface : public UserInterface {
public:
	void loaded_computations(const string& input_file) override {}
	void unpacked_computations() override {}
	void removed_computations_in_db(int eliminated) override {}
	void removed_precalculated(int eliminated) override {}
	void aborted_computations(int eliminated) override {}
	void resurrected(int resurrected,  megabytes memory_limit) override {}
	void assigned_computations(int assigned) override {}
	void tick(int packed_computations, int unpacked_computations, int bad, int abandoned) override {}
	void print_computation(const Computation& computation) override {}
	void update_bad(const vector<pair<megabytes,int>>& memory_limits) override {}
	void detach() const override {}
	unique_ptr<ThreadUIHandle> make_thread_handle(int thread) {return make_unique<ThreadNoUIHandle>();}
	
	static NoUserInterface& singleton() {
		static NoUserInterface singleton;
		return singleton;
	}		
};

#endif
