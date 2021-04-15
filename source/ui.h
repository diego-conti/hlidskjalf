#ifndef UI_H
#define UI_H
#include "stdincludes.h"

class Computation;

class ThreadUIHandle  {
public:
	virtual void computations_added(int assigned_computations, megabytes memory_limit) const=0;
	virtual void thread_started(megabytes memory) const=0;
	virtual void thread_stopped(megabytes memory) const=0;
	virtual void bad_computation(const Computation& computation, megabytes memory_limit) const =0;	
};

class UserInterface {
public:
	virtual void loaded_computations(const string& input_file,int packed_computations, int unpacked_computations, int bad) const=0;
	virtual void unpacked_computations(int packed_computations, int unpacked_computations, int bad) const=0;
	virtual void removed_computations_in_db(int eliminated,int packed_computations, int unpacked_computations, int bad) const=0;
	virtual void removed_precalculated(int eliminated,int packed_computations, int unpacked_computations, int bad) const=0;
	virtual void aborted_computations(int eliminated,int packed_computations, int unpacked_computations, int bad) const=0;	
	virtual void resurrected(int resurrected,int packed_computations, int unpacked_computations, int bad) const=0;
	virtual void assigned_computations(int assigned,int packed_computations, int unpacked_computations, int bad) const=0;	
	virtual void tick() const=0;
	virtual void print_computation(const Computation& computation) const=0;
	
	virtual void detach() const =0;
		
	virtual unique_ptr<ThreadUIHandle> make_thread_handle(int thread) =0;	//the handle becomes invalid when the UI is destroyed
};



class ThreadNoUIHandle : public ThreadUIHandle {
public:
	void computations_added(int assigned_computations, megabytes memory_limit) const {}
	void thread_started(megabytes memory) const {}
	void thread_stopped(megabytes memory) const {}
	void bad_computation(const Computation& computation, megabytes memory_limit) const {}
};

class NoUserInterface : public UserInterface {
public:
	 void loaded_computations(const string& input_file,int packed_computations, int unpacked_computations, int bad) const override {}
	 void unpacked_computations(int packed_computations, int unpacked_computations, int bad) const override {}
	 void removed_computations_in_db(int eliminated,int packed_computations, int unpacked_computations, int bad) const override {}
	 void removed_precalculated(int eliminated,int packed_computations, int unpacked_computations, int bad) const override {}
	 void aborted_computations(int eliminated,int packed_computations, int unpacked_computations, int bad) const override {}
	void resurrected(int resurrected,int packed_computations, int unpacked_computations, int bad) const override {}
	void assigned_computations(int assigned,int packed_computations, int unpacked_computations, int bad) const override {}
	void tick() const override {}
	void print_computation(const Computation& computation) const override {}

	void detach() const override {}
	unique_ptr<ThreadUIHandle> make_thread_handle(int thread) {return make_unique<ThreadNoUIHandle>();}
	
	static NoUserInterface& singleton() {
		static NoUserInterface singleton;
		return singleton;
	}		
};

#endif
