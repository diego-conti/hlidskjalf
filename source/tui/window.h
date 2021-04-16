#ifndef WINDOW_H
#define WINDOW_H

#include <ncurses.h>
#include <memory>
#include <string>
#include "screenlock.h"
#include <sstream>

struct Dimensions {
	int width,height;
};
inline bool operator==(Dimensions D1, Dimensions D2) {return D1.width==D2.width && D1.height ==D2.height;}

struct Position {
	int x,y;
};
inline bool operator==(Position D1, Position D2) {return D1.x==D2.x && D1.y ==D2.y;}

class Window;

class WindowHandle {
	friend class Window;
	std::shared_ptr<Window> ptr;
	WindowHandle(const std::shared_ptr<Window>& ptr) : ptr{ptr} {}
public:	
	WindowHandle()=default;
	void refresh() const;
	void clear();
	void lock();
	void unlock();
	template<typename  T> 
	void append_to_buffer(T&& s);
	string get_string();
	Window& window() {return *ptr;}
	bool operator==(const WindowHandle& other) const {return ptr==other.ptr;}
};


//output is performed only by one thread. window creation is done by any thread but synchronized with screenlock.
//writing to a window only puts text in an interal buffer which is protected by a mutex
//invoking refresh (from the UI thread) puts the buffer in the ncurses window and refreshes it
//to consider: why use ncurses windows?
//do I need invalidation?
class Window {
	WINDOW* window=nullptr;	
	std::stringstream content;
	mutable std::mutex mtx;	
	mutable bool valid;
	Window(const Window&)=delete;
	Window(Window&& other) {
		window=other.window;
		other.window=nullptr;
	}	
	void invalidate() const {valid=false;}
	class Construct {};
public:
	//constructor takes a tag to prevent allocation from outside this class
	Window(Dimensions size, Position position,Construct)  {
			auto lock=GlobalTuiLock::unique_lock();
			window=newwin(size.height,size.width,position.y,position.x);
			invalidate();
	}
	Window()=default;	
	~Window() {
		auto lock=GlobalTuiLock::unique_lock();
		if (window) delwin(window);
	}
	template<typename  T> 
	void append_to_buffer(T&& s) {
		content<<s;
		invalidate();
	}
	void lock() {mtx.lock();}
	void unlock() {mtx.unlock();}	
//should only be called by UI thread
	void refresh() const {
		if (!valid) {
			std::unique_lock<std::mutex> lock{mtx};
			werase(window);
			wprintw(window,content.str().c_str());
			wrefresh(window);
			valid=true;
		}
	}
	void clear() {
		content.str(string{});
		invalidate();
	}
	string get_string() {
		char buffer[1024];
		wgetnstr(window,buffer,1024);
		return buffer;
	}	
	static WindowHandle create_window(Dimensions size, Position position) {
		return std::make_shared<Window>(size,position,Construct{});
	}
};

void WindowHandle::clear() {ptr->clear();}
void WindowHandle::refresh() const {ptr->refresh();}
void WindowHandle::lock() {ptr->lock();}
void WindowHandle::unlock() {ptr->unlock();}
string WindowHandle::get_string() {return ptr->get_string();}

template<typename  T> 
void WindowHandle::append_to_buffer(T&& s) {
	ptr->append_to_buffer(std::forward<T>(s));
}


template<typename T>
WindowHandle& operator<<(WindowHandle& window, T&& t) {
	window.append_to_buffer(t);
	return window;
}

void append(WindowHandle& window) {window.lock();}
void clear(WindowHandle& window) {window.lock(); window.clear();}
void release(WindowHandle& window) {window.unlock();}
void refresh(WindowHandle& window) {window.unlock(); window.refresh();}

typedef void (* WindowManipulator)(WindowHandle&);

WindowHandle& operator<<(WindowHandle& window, WindowManipulator f) {
	f(window);
	return window;
}

#endif


