#include "api.hh"

#include <iostream>
#include <sstream>
#include <system_error>

#include "dlfcn.h"
#include "signal.h"
#include "sys/stat.h"
#include "unistd.h" // access

#include "FL/Fl_Button.H"
#include "FL/Fl_Tabs.H"

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

class UiPimpl {
	typedef bool (*fnstart)(void *);
	typedef bool (*fnupdate)();
	typedef bool (*fnstop)();
	private:
		static UiPimpl *self;
		bool self_destruct;
		std::vector<std::string> plugins;
		std::vector<PluginHandles> plugin_handles;
		unique_ptr<Fl_Window> win;
		unique_ptr<Fl_Button> btn_quit;
		unique_ptr<Fl_Tabs> tabs;
		std::vector<unique_ptr<Fl_Group>> tabgroups;
//		unique_ptr<Fl_Group> noresize;
	public:
		UiPimpl(std::vector<std::string> &);
		~UiPimpl();
		bool start(string &);
		bool update();
		bool stop();
		void die();
		void reload();
		Fl_Group *add_tab(const string &);
	private:
		static void handle_signal(int);
		static void callback_quit(Fl_Widget *, void *);
		static void callback_tabs(Fl_Widget *, void *);
		void load_config();
		void save_config();
};

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

string expand_bash_tilde(string path) {
	// expand the bash "~" filesystem mark in the passed path
	auto iter = path.find('~');
	if (iter == string::npos) {
		return path;
	}
	string new_path{};
	if (path[iter+1] == '/') {
		// just ~ by itself - get username
		new_path = path.substr(0, iter);
		new_path += "/home/";
		new_path += getenv("USER");
		new_path += path.substr(iter+1);
		path = new_path;
		return path;
	}
	// ~ plus username
	// TO DO
	return path;
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

Fl_Group *add_tab(void *ptr, const string &label) {
	return reinterpret_cast<UiPimpl *>(ptr)->add_tab(label);
}

UiPimpl *UiPimpl::self{nullptr};

UiPimpl::UiPimpl(std::vector<std::string> &p)
	: self_destruct{false}, plugins{p}, plugin_handles{} {
	self = this;
}

UiPimpl::~UiPimpl() { }

// interesting fltk links
// https://www.fltk.org/doc-2.0/html/group__themes.html#ga0
// https://www.fltk.org/doc-1.3/classFl.html#a43e6e0bbbc03cad134d928d4edd48d1d
// https://www.fltk.org/doc-1.3/group__fl__events.html

bool UiPimpl::start(string &plugins_dir) {
	void *ptr{nullptr};
	signal(SIGINT, UiPimpl::handle_signal);
	// Start the main UI.
	win = make_unique<Fl_Window>(800, 600, "blatherskite");
	win->border(true);
	win->size_range(20, 20, 1900, 1200, 1, 1); // fixes the float issue :) 
	tabs = make_unique<Fl_Tabs>(10, 10, 780, 580);
	tabs->callback(UiPimpl::callback_tabs);
	tabs->user_data(this);
	win->resizable(tabs.get()); // window resizes tabs
	// Load main plugins.
	for (auto &p : plugins) {
		// locate functions
		plugin_handles.emplace_back();
		plugin_handles.back().name = p;
		ptr = dlopen((plugins_dir + "/" + p).c_str(), RTLD_NOW);
		ostringstream errmsg{};
		errmsg << "Failed to open plugin " << p;
		errmsg << ". Cause: ";
		THROW_IF_NULLPTR(ptr, errmsg.str() + dlerror());
		plugin_handles.back().handle = ptr;
		ptr = dlsym(plugin_handles.back().handle, "start");
		errmsg.str("");
		errmsg << "Failed to locate start function in plugin ";
		errmsg << p << ". Cause: ";
		THROW_IF_NULLPTR(ptr, errmsg.str() + dlerror());
		plugin_handles.back().start = ptr;
		ptr = dlsym(plugin_handles.back().handle, "update");
		errmsg.str("");
		errmsg << "Failed to locate update function in plugin ";
		errmsg << p << ". Cause: ";
		THROW_IF_NULLPTR(ptr, errmsg.str() + dlerror());
		plugin_handles.back().update = ptr;
		ptr = dlsym(plugin_handles.back().handle, "stop");
		errmsg.str("");
		errmsg << "Failed to locate stop function in plugin ";
		errmsg << p << ". Cause: ";
		THROW_IF_NULLPTR(ptr, errmsg.str() + dlerror());
		plugin_handles.back().stop = ptr;
		// call start function
		ptr = plugin_handles.back().start;
		errmsg.str("");
		errmsg << "Failed to start plugin " << p;
		THROW_IF_FALSE((*(fnstart)ptr)(this), errmsg.str());
	}
	// finished adding tabs
	tabs->end();
//	noresize = make_unique<Fl_Group>(720, 550, 70, 40);
	btn_quit = make_unique<Fl_Button>(720, 550, 70, 40, "Quit");
	btn_quit->callback(UiPimpl::callback_quit);
	btn_quit->user_data(this);
//	btn_quit->resize(0);
//	noresize->resizable(0); // never resize button
//	noresize->end();
//	btn_quit->show();
	win->end();
	win->show();
	callback_tabs(reinterpret_cast<Fl_Widget *>(tabs.get()), nullptr);
	//load_config();
	return true;
}

bool UiPimpl::update() {
	if (self_destruct) return false;
	for (auto &p : plugin_handles) {
		ostringstream errmsg{};
		errmsg << "Failed to update plugin " << p.name;
		THROW_IF_FALSE((*(fnupdate)p.update)(), errmsg.str());
	}
	Fl::check(); // Update UI.
	return true;
}

bool UiPimpl::stop() {
	void *ptr{nullptr};
	for (auto &p : plugin_handles) {
		ostringstream errmsg{};
		errmsg << "Failed to stop plugin " << p.name;
		THROW_IF_FALSE((*(fnstop)p.stop)(), errmsg.str());
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

void UiPimpl::callback_quit(Fl_Widget *, void *d) {
	reinterpret_cast<UiPimpl *>(d)->die();
}

void UiPimpl::callback_tabs(Fl_Widget *w, void *) {
//	reinterpret_cast<UiPimpl *>(d)->die();
	Fl_Tabs *t = reinterpret_cast<Fl_Tabs *>(w);
	t->selection_color(FL_WHITE);
}

void UiPimpl::handle_signal(int s) {
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

void UiPimpl::die() {
	self_destruct = true;
}

void UiPimpl::reload() {
	load_config();
}

void UiPimpl::load_config() {
}

void UiPimpl::save_config() {
}

Fl_Group *UiPimpl::add_tab(const string &label) {
	cerr << "Tab: " << label << endl;
	auto t = new Fl_Group(10, 10, 780, 540, label.c_str());
	tabs->resizable(t);
	return t;
}

////////////////////////////////////////////////////////////////////////

Fascia::Fascia(std::vector<std::string> &p)
		: pimpl{make_unique<UiPimpl>(p)} { }

Fascia::~Fascia() { }

bool Fascia::start(std::string &plugins_dir) {
	return pimpl->start(plugins_dir);
}

bool Fascia::update() {
	return pimpl->update();
}

bool Fascia::stop() {
	return pimpl->stop();
}

void Fascia::die() {
	pimpl->die();
}

void Fascia::reload() {
	pimpl->reload();
}

