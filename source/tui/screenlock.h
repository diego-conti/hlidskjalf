#ifndef SCREENLOCK_H
#define SCREENLOCK_H
#include <mutex>
//prevent simultaneous access to ncurses functions
class GlobalTuiLock {
	static std::mutex& mtx() {
		static std::mutex mtx;
		return mtx;
	}
public:
	static void lock() {mtx().lock();}
	static void unlock() {mtx().unlock();}
	static auto unique_lock() {return std::unique_lock(mtx());}
	template<typename... T>
	static auto scoped_lock(T&&... args) {return std::scoped_lock(mtx(),std::forward<T>(args)...);}
	template<typename... T>
	static auto scoped_lock(std::adopt_lock_t t, T&&... args) {return std::scoped_lock(t,mtx(),std::forward<T>(args)...);}
};

#endif
