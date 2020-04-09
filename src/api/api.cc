#include "api.hh"

#include <iostream>
#include <sstream>

#include "dlfcn.h"
#include "signal.h"

#define SIGHUP  1   /* Hangup the process */ 
#define SIGINT  2   /* Interrupt the process */ 
#define SIGQUIT 3   /* Quit the process */ 

using std::cerr;
using std::endl;
using std::vector;
using std::string;
using std::runtime_error;
using std::ostringstream;

void throw_custom(string err, string file, int line, string func) {
	ostringstream ostr{};
	ostr << err;
	ostr << " at ";
	ostr << file << ":" << line << " (";
	ostr << func << " function)";
	throw runtime_error(ostr.str());
}

void test() {
	std::cout << "OI" << endl;
	cerr << "Test success!" << endl;
}

void test2(std::string s) {
	std::cout << "OI " << s << endl;
	cerr << "Test success!" << endl;
}

Fascia *Fascia::self{nullptr}; // TO DO: make Singleton? RAII

Fascia::Fascia(vector<string> &p) : plugins{p}, plugin_handles{} {
	self = this;
}

Fascia::~Fascia() { }

bool Fascia::start(string &plugins_dir) {
	void *ptr{nullptr};
	signal(SIGINT, Fascia::handle_signal);
	ui_start();
	for (auto &p : plugins) {
		// locate functions
		plugin_handles.emplace_back();
		plugin_handles.back().name = p;
		ptr = dlopen((plugins_dir + "/" + p).c_str(), RTLD_NOW);
		THROW_IF_NULLPTR(ptr, string{"Failed to open plugin " + p + ". Cause: " + dlerror()});
		plugin_handles.back().handle = ptr;
		ptr = dlsym(plugin_handles.back().handle, "start");
		THROW_IF_NULLPTR(ptr, string{"Failed to locate start function in plugin " + p + ". Cause: " + dlerror()});
		plugin_handles.back().start = ptr;
		ptr = dlsym(plugin_handles.back().handle, "update");
		THROW_IF_NULLPTR(ptr, string{"Failed to locate update function in plugin " + p + ". Cause: " + dlerror()});
		plugin_handles.back().update = ptr;
		ptr = dlsym(plugin_handles.back().handle, "stop");
		THROW_IF_NULLPTR(ptr, string{"Failed to locate stop function in plugin " + p + ". Cause: " + dlerror()});
		plugin_handles.back().stop = ptr;
		// call start function
		ptr = plugin_handles.back().start;
		THROW_IF_FALSE((*(fnstart)ptr)(this), string{"Failed to start plugin " + p});
	}
	return true;
}

bool Fascia::update() {
	if (self_destruct) return false;
	for (auto &p : plugin_handles) {
		THROW_IF_FALSE((*(fnupdate)p.update)(), string{"Failed to update plugin " + p.name});
	}
	ui_update();
	return true;
}

bool Fascia::stop() {
	void *ptr{nullptr};
	for (auto &p : plugin_handles) {
		THROW_IF_FALSE((*(fnstop)p.stop)(), string{"Failed to stop plugin " + p.name});
		dlclose(p.handle);
		p.name = "";
		p.handle = nullptr;
		p.start = nullptr;
		p.update = nullptr;
		p.stop = nullptr;
	}
	ui_stop();
	return true;
}

void Fascia::die() {
	self_destruct = true;
}

void Fascia::reload() {
}

void Fascia::load_config() {
}

void Fascia::save_config() {
}

void Fascia::ui_start() {
/*
	int num_args = 0;
	char *args = nullptr;
	char **argptr = &args;
	gtk_init(&num_args, &argptr);
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "Bob");
	gtk_window_set_default_size(GTK_WINDOW(window), 100, 100);
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), 0);
	button = gtk_button_new_with_label("Click me!");
	gtk_container_add(GTK_CONTAINER(window), button);
	gtk_widget_show_all(window);
*/
}

void Fascia::ui_update() {
/*
	// kill this on window close
	// apparently not
	self_destruct = gtk_main_iteration_do(gtk_false()); // update gtk, nonblocking
	cerr << "quit: " << std::boolalpha << self_destruct << endl;
*/
//	if (fltk::ready()) {
//		fltk::check();
//	}
}

void Fascia::ui_stop() {
}

void Fascia::handle_signal(int s) {
	if (self == nullptr) return;
	switch (s) {
		case SIGINT: {
			self->die();
		} break;
		case SIGHUP: {
			self->reload();
		} break;
		case SIGQUIT: {
			self->die();
		} break;
		default: {
		}
	}
}

