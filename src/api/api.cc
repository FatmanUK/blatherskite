#include "api.hh"

#include <iostream>
#include <sstream>
#include <system_error>
#include <memory>

#include "dlfcn.h"
#include "signal.h"
#include "sys/stat.h"
#include "unistd.h" // access

#include "FL/Fl.H"
#include "FL/Fl_Window.H"
#include "FL/Fl_Button.H"

#define SIGHUP  1   /* Hangup the process */ 
#define SIGINT  2   /* Interrupt the process */ 
#define SIGQUIT 3   /* Quit the process */ 

using std::string;
using std::cerr;
using std::endl;
using std::ostringstream;
using std::runtime_error;
using std::vector;
using std::unique_ptr;
using std::make_unique;

void throw_custom(string err, string file, int line, string func) {
	ostringstream ostr{};
	ostr << err;
	ostr << " at ";
	ostr << file << ":" << line << " (";
	ostr << func << " function)";
	throw runtime_error(ostr.str());
}

void test() {
	cerr << "Test success!" << endl;
}

void test2(std::string s) {
	cerr << "Test success " << s << "!" << endl;
}

void expand_bash_tilde(string &path) {
	// expand the bash "~" filesystem mark in the passed path
	auto iter = path.find('~');
	if (iter == string::npos) {
		return;
	}
	string new_path{};
	if (path[iter+1] == '/') {
		// just ~ by itself - get username
		new_path = path.substr(0, iter);
		new_path += "/home/";
		new_path += getenv("USER");
		new_path += path.substr(iter+1);
		path = new_path;
		return;
	}
	// ~ plus username
	// TO DO
	return;
}

bool ends_with(string &haystack, string needle) {
	if (haystack.length() < needle.length()) {
		return false;
	}
	size_t pos = haystack.length() - needle.length();
	return 0 == haystack.compare(pos, needle.length(), needle);
}

bool path_is_extant_dir(string path) {
	path += '/';
	struct stat sb;
	return stat(path.c_str(), &sb) == 0;
}

bool dir_allows_read(string path) {
	return access(path.c_str(), R_OK) == 0;
}

bool dir_allows_write(string path) {
	return access(path.c_str(), W_OK) == 0;
}

bool dir_allows_exec(string path) {
	return access(path.c_str(), X_OK) == 0;
}

unique_ptr<Fl_Window> win{nullptr};
unique_ptr<Fl_Button> quit{nullptr};

Fascia *Fascia::self{nullptr}; // TO DO: make Singleton? RAII

Fascia::Fascia(vector<string> &p) : plugins{p}, plugin_handles{} {
	self = this;
}

Fascia::~Fascia() { }

bool Fascia::start(string &plugins_dir) {
	void *ptr{nullptr};
	signal(SIGINT, Fascia::handle_signal);
	// Start the main UI.
	Fl_Window::default_xclass(nullptr);
	win = make_unique<Fl_Window>(300, 200, "Testing");
	win->begin();
	quit = make_unique<Fl_Button>(10, 150, 70, 30, "Quit");
	quit->callback(Fascia::callback_quit);
	quit->user_data(this);
	win->end();
	win->set_non_modal();
	win->show();
	// Load main plugins.
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
	Fl::check(); // Update UI.
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
	// Nothing to do to quit FLTK?
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

void Fascia::callback_quit(Fl_Widget *, void *d) {
	reinterpret_cast<Fascia *>(d)->die();
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

