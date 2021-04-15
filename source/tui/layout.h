#ifndef LAYOUT_H
#define LAYOUT_H
#include "screen.h"
#include <array>

//windows created with the CHILD attribute are refreshed by the Screen object
enum class WindowType {
	CHILD, STANDALONE
};

class VerticalLayout {
	int bottom=0;	
	Screen& screen;
	std::mutex mtx;

	WindowHandle create_window(WindowType type, Dimensions dimensions, Position position) {
		switch (type) {
			case WindowType::STANDALONE : return Window::create_window(dimensions,position);
			case WindowType::CHILD : return screen.create_window(dimensions,position);
			default:
				throw std::logic_error("unexpected WindowType");
		}
	}
public:
	VerticalLayout(Screen& screen) : screen{screen} {}

	WindowHandle create_window(int height, WindowType type) {
		std::unique_lock<std::mutex> lock{mtx};
		auto dimensions=screen.dimensions();
		dimensions.height=height;
		auto position=Position{0,bottom};
		bottom+=height;
		return create_window(type,dimensions,position);
	}

	template<typename... T>
	auto create_windows(int height, T... windowTypes) {
		std::unique_lock<std::mutex> lock{mtx};
		constexpr int n=sizeof...(T);
		std::array<WindowType,n> types={windowTypes...};
		std::array<WindowHandle,n> windows;
		auto dimensions=screen.dimensions();
		dimensions.height=height;
		dimensions.width/=n;
		Position position{0,bottom++};
		for (int i=0;i<n;++i,position.x+=dimensions.width)
			windows[i]=create_window(types[i],dimensions,position);
		return windows;
	}
};


#endif
