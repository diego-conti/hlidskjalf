#ifndef INTERACTIVECONTROLLER_H
#define INTERACTIVECONTROLLER_H
#include "memorymanager.h"
#include "ui.h"
#include "computationrunner.h"
class InteractiveController : public Controller {
	UserInterface* ui;
public:
	InteractiveController(UserInterface* ui) : ui{ui} {
		ui->attach_controller(this);
	}
	void on_key_pressed(int keyCode) override {
		switch (keyCode) {
			case 'q' : 
				ComputationRunner::singleton().terminate(); 
				break;
			case 'l': 
				ComputationRunner::singleton().load_computations(ui->get_filename("Enter .comp file to load:"));
				break;
			case 'c': 
				ComputationRunner::singleton().set_no_computations(ui->get_number("Computations per process:"));
				break;
			case KEY_DOWN : 
				ui->display_memory_limit(MemoryManager::singleton().increase_memory_limit(-128,0));
				break;
			case KEY_UP	: 
				ui->display_memory_limit(MemoryManager::singleton().increase_memory_limit(128,0));
				break;
			case KEY_NPAGE: 
				ui->display_memory_limit(MemoryManager::singleton().increase_memory_limit(0,-128));
				break;
			case KEY_PPAGE: 
				ui->display_memory_limit(MemoryManager::singleton().increase_memory_limit(0,128));
				break;
			default:
				break;
		}	
	}
	void on_screen_size_changed(int width, int height) override {}	
	~InteractiveController() {ui->attach_controller();}
};

#endif
