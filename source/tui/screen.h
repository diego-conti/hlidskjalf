#ifndef SCREEN_H
#define SCREEN_H

#include "window.h"
#include <list>

class Screen {
	mutable std::mutex mtx;
	std::list<WindowHandle> windows;
public:
	Screen() {
		initscr();			/* Start curses mode 		*/
		raw();				/* Line buffering disabled	*/
		keypad(stdscr, TRUE);		/* We get F1, F2 etc..		*/
		cbreak();
		noecho();			/* Don't echo() while we do getch */
		refresh();	
		nodelay(stdscr, TRUE);
	}
	Dimensions dimensions() {
		Dimensions result;
		getmaxyx(stdscr, result.height, result.width);	
		return result;
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
	bool user_has_pressed(int code) const {
		int ch;
		while (true) {
			ch=getch();
			if (ch==code) return true;
			if (ch==ERR) return false;
		};
	}	
	void refresh() const {
		std::unique_lock<std::mutex> lock{mtx};
		for (auto& x: windows) x.refresh();
	}
	~Screen() {
		auto lock=GlobalTuiLock::scoped_lock(mtx);
		endwin();
	}
};

#endif
