#ifndef WINDOW_H
#define WINDOW_H

#include <ncurses.h>
#include <memory>
#include <string>
#include "screenlock.h"

struct Dimensions {
	int width,height;
};

struct Position {
	int x,y;
};

class Window;

class WindowHandle {
	friend class Window;
	std::shared_ptr<Window> ptr;
	WindowHandle(const std::shared_ptr<Window>& ptr) : ptr{ptr} {}
public:	
	WindowHandle()=default;
	void refresh() const;
	void clear() const;
	void print(const std::string& s) const;	
	void print(const char* s) const;
	template<typename T> void print(const T& t) const{
		print(std::to_string(t));
	}
	Window& window() {return *ptr;}
	bool operator==(const WindowHandle& other) const {return ptr==other.ptr;}
};


//output to a window is not synchronized. The manipulators clear and refresh/release do the synchronization.
class Window {
	WINDOW* window=nullptr;	
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
	void print(const char* s) const {
		wprintw(window,s);
		invalidate();
	}
	void print(const std::string& s) const {
		for (char c: s)
	    waddch(window,c);
		invalidate();
	}
	void refresh() const {
		if (!valid) {
			wrefresh(window);
			valid=true;
		}
	}
	void clear() const {
		wclear(window);
		invalidate();
	}
	~Window() {
		auto lock=GlobalTuiLock::unique_lock();
		if (window) delwin(window);
	}
	static WindowHandle create_window(Dimensions size, Position position) {
		return std::make_shared<Window>(size,position,Construct{});
	}
	bool is_valid() const {return valid;}	
};

void WindowHandle::refresh() const {ptr->refresh();}
void WindowHandle::clear() const {ptr->clear();}
void WindowHandle::print(const std::string& s) const {ptr->print(s);}	
void WindowHandle::print(const char* s) const {ptr->print(s);}	

template<typename T>
const WindowHandle& operator<<(const WindowHandle& window, T&& t) {
	window.print(t);
	return window;
}

void refresh(const WindowHandle& window) {window.refresh(); GlobalTuiLock::unlock();}
void clear(const WindowHandle& window) {GlobalTuiLock::lock(); window.clear();}
void release(const WindowHandle& window) {GlobalTuiLock::unlock();}

typedef void (* WindowManipulator)(const WindowHandle&);

const WindowHandle& operator<<(const WindowHandle& window, WindowManipulator f) {
	f(window);
	return window;
}

#endif


