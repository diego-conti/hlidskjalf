#ifndef SCREEN_H
#define SCREEN_H

#include "window.h"
#include <list>
#include <iostream>

class Screen {
	mutable std::mutex mtx;
	std::list<WindowHandle> windows;
	Dimensions screen_dimensions;
public:
	Screen() {
		initscr();			/* Start curses mode 		*/
		raw();				/* Line buffering disabled	*/
		keypad(stdscr, TRUE);		/* We get F1, F2 etc..		*/
		cbreak();
		noecho();			/* Don't echo() while we do getch */
		refresh();	
		nodelay(stdscr, TRUE);
		getmaxyx(stdscr, screen_dimensions.height, screen_dimensions.width);	
	}
	Dimensions dimensions() {
		return screen_dimensions;
	}
	WindowHandle create_window(Dimensions size, Position position) {
		std::unique_lock<std::mutex> lock{mtx};
		windows.emplace_back(Window::create_window(size,position));
		return windows.back();
	}
	void destroy_window(WindowHandle window) {
		std::unique_lock<std::mutex> lock{mtx};
		windows.remove(window);
	}
	optional<int> key_pressed() const {
		int ch=getch();		
		return ch==ERR? nullopt : make_optional<int>(ch);
	}
	bool size_changed() {	
		Dimensions present_dimensions;
		getmaxyx(stdscr, present_dimensions.height, present_dimensions.width);	
		std::swap(present_dimensions,screen_dimensions);
		return !(present_dimensions==screen_dimensions);
	}
	void refresh() const {
		std::unique_lock<std::mutex> lock{mtx};
		for (auto& x: windows) x.refresh();
	}
	string get_string(WindowHandle window) {
		echo();
		auto result=window.get_string();
		noecho();
		return result;
	}
	~Screen() {
		move(screen_dimensions.height-1,0);
		clear();
		endwin();
	}
};

#endif
