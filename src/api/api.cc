#include "api.hh"

#include "unistd.h" // access
#include "dlfcn.h"
#include "signal.h"
#include "sys/stat.h"

#include <iostream>
#include <sstream>
#include <system_error>
#include <map>

using std::unique_ptr;
using std::make_unique;
using std::shared_ptr;
using std::make_shared;
using std::string;
using std::ostringstream;
using std::runtime_error;
using std::vector;
using std::map;

#include "FL/Fl_Tabs.H"
#include "FL/Fl_Button.H"
#include "FL/Fl_Pack.H"
#include "FL/Fl_PNG_Image.H"
#include "FL/Fl_Tree.H"

const unsigned int WIN_W=800;
const unsigned int WIN_H=600;
const unsigned int WIN_WMAX=20000;
const unsigned int WIN_HMAX=20000;
const unsigned int WIN_WMIN=1;
const unsigned int WIN_HMIN=1;

// random API
bool ends_with(string &haystack, string needle) {
	if (haystack.length() < needle.length()) {
		return false;
	}
	size_t pos = haystack.length() - needle.length();
	return 0 == haystack.compare(pos, needle.length(), needle);
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

void throw_custom(string err, string file, int line, string func) {
	ostringstream ostr{};
	ostr << err;
	ostr << " at ";
	ostr << file << ":" << line << " (";
	ostr << func << " function)";
	throw runtime_error(ostr.str());
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

void *load_image(const string &filename) {
	auto ptr{new Fl_PNG_Image{filename.c_str()}};
	return ptr;
}

// Pimpl
class UiPimpl {
	typedef bool (*fnstart)(void *);
	typedef bool (*fnupdate)();
	typedef bool (*fnstop)();
	private:
		static UiPimpl *self;
		bool self_destruct;
		vector<string> plugins;
		vector<PluginHandles> plugin_handles;
		unique_ptr<Fl_Window> win;
		map<string, Fl_Pack *> tabmap;
	public:
		UiPimpl(std::vector<std::string> &);
		~UiPimpl();
		bool start(string &);
		bool update();
		bool stop();
		void add_tab(const string &);
		void die();
		void reload();
		void *get_tab(const string &);
	private:
		static void handle_signal(int);
		static void cb_quit(Fl_Widget *, void *);
		static void cb_tabs(Fl_Widget *, void *);
		void load_config();
		void save_config();
};

// UI API
void *create_tab(void *ui, const string &label) {
	auto pimpl{reinterpret_cast<UiPimpl *>(ui)};
	pimpl->add_tab(label);
	return pimpl->get_tab(label);
}

void *add_pack(void *ptr, char type) {
	auto parent{reinterpret_cast<Fl_Group *>(ptr)};
	auto t{new Fl_Pack(10, 10, 780, 540, nullptr)};
	parent->add(t);
	t->spacing(2);
	t->type(type);
	parent->resizable(t);
	return t;
}

void *add_hpack(void *ptr) {
	return add_pack(ptr, 0);
}

void *add_vpack(void *ptr) {
	return add_pack(ptr, 1);
}

void *add_tree(void *ptr, const string &root) {
	auto parent{reinterpret_cast<Fl_Group *>(ptr)};
	auto tree{new Fl_Tree(0, 0, 300, 10)};
	tree->root_label(root.c_str());
	parent->add(tree);
//	parent->resizable(tree);
	return tree;
}

void *add_treeitem(void *ui, void *ptr1, void *ptr2, const string &item, void *imgptr) {
	auto pimpl{reinterpret_cast<UiPimpl *>(ui)};
	auto tree{reinterpret_cast<Fl_Tree *>(ptr1)};
	auto parent{reinterpret_cast<Fl_Tree_Item *>(ptr2)};
	void *ti{tree->add(parent, item.c_str())};
//	auto img{pimpl->get_image(c)};
	auto img{reinterpret_cast<Fl_Image *>(imgptr)};
	reinterpret_cast<Fl_Tree_Item *>(ti)->usericon(img);
	return ti;
}

void *get_tree_root(void *ptr) {
	auto tree{reinterpret_cast<Fl_Tree *>(ptr)};
	return tree->root();
}

// Pimpl impl
UiPimpl *UiPimpl::self{nullptr};

UiPimpl::UiPimpl(std::vector<std::string> &p)
	: self_destruct{false}, plugins{p}, plugin_handles{},
	  win{nullptr}, tabmap{} {
	self = this;
}

UiPimpl::~UiPimpl() {
	// TO DO: delete tabmap
}

bool UiPimpl::start(string &plugins_dir) {
	void *ptr{nullptr};
	signal(SIGINT, UiPimpl::handle_signal);
	// Start the main UI.
	win = make_unique<Fl_Window>(WIN_W, WIN_H, "blatherskite");
	win->border(true);
	win->size_range(WIN_WMIN, WIN_HMIN, WIN_WMAX, WIN_HMAX, 1, 1); // fixes the float issue :) 
	auto tabs = new Fl_Tabs{10, 10, WIN_W - 20, WIN_H - 20};
	tabs->callback(UiPimpl::cb_tabs);
	tabs->user_data(this);
	win->resizable(tabs); // window resizes tabs
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
	tabs->end();
// TO DO: make this a menu?
	auto btn_quit = new Fl_Button{WIN_W - 80, WIN_H - 50, 70, 40, "&Quit"};
	btn_quit->callback(UiPimpl::cb_quit);
	btn_quit->user_data(this);
	win->show();
	cb_tabs(tabs, nullptr);
	load_config();
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

void UiPimpl::die() {
	self_destruct = true;
}

void UiPimpl::reload() {
	load_config();
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

void UiPimpl::cb_quit(Fl_Widget *, void *d) {
	reinterpret_cast<UiPimpl *>(d)->die();
}

void UiPimpl::cb_tabs(Fl_Widget *w, void *) {
	Fl_Tabs *t = reinterpret_cast<Fl_Tabs *>(w);
	t->selection_color(FL_WHITE);
}

void UiPimpl::load_config() {
}

void UiPimpl::save_config() {
}

void UiPimpl::add_tab(const string &label) {
	auto t = new Fl_Pack(10, 10, 780, 540, label.c_str());
	t->spacing(2);
	tabmap[label] = t;
	auto tabs = reinterpret_cast<Fl_Tabs *>(win->child(0));
	tabs->add(t);
	tabs->resizable(t);
}

void *UiPimpl::get_tab(const string &label) {
	auto iter = tabmap.find(label);
	THROW_IF_FALSE(iter != tabmap.end(), "Tab not found.");
	return tabmap.at(label);
}

/*
#include "FL/Fl_Box.H"


#include "FL/Fl_Text_Display.H"
//include "FL/Fl_Check_Browser.H"
#include "FL/Fl_Table.H"

// FLTK-type callback function. Cast passed cb_fn_ptrs to this.
typedef void (*fltk_cb_fn)(Fl_Widget *, void *);

#define SIGHUP  1   // Hangup the process
#define SIGINT  2   // Interrupt the process
#define SIGQUIT 3   // Quit the process

using std::cerr;
using std::endl;

void test() {
	cerr << "Test success!" << endl;
}

void test2(std::string s) {
	cerr << "Test success " << s << "!" << endl;
}


/
void *create_tab(void *ptr, const string &label) {
	auto pimpl{reinterpret_cast<UiPimpl *>(ptr)};
	auto rv{pimpl->add_tab(label)};
	rv->end();
	return reinterpret_cast<void *>(rv);
}
/

void add_button(void *ui, const string &label, cb_fn_ptr fn, const string &btn_label) {
	auto pimpl{reinterpret_cast<UiPimpl *>(ui)};
	pimpl->add_button(label, reinterpret_cast<fltk_cb_fn>(fn), btn_label);
}

/
void add_button(void *grp, cb_fn_ptr fn, const string &label) {
	// convert group to pack, first
	// TO DO: detect if group first?
	auto pck = reinterpret_cast<Fl_Group *>(grp)->array()[0];
	auto btn = new Fl_Button(20, 20, 70, 40, label.c_str());
	btn->callback(reinterpret_cast<fltk_cb_fn>(fn), nullptr);
	reinterpret_cast<Fl_Pack *>(pck)->add(btn);
}
/

void *add_status_bar(void *ptr) {
	auto parent{reinterpret_cast<Fl_Group *>(ptr)};
	auto sb{new Fl_Box(0, 0, 100, 20)};
	sb->box(FL_FLAT_BOX);
	sb->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	sb->color(48-2);
	sb->copy_label("Yaay!");
	parent->add(sb);
	return sb;
}

void *add_list(void *ptr) {
	auto parent{reinterpret_cast<Fl_Group *>(ptr)};
	auto list{new Fl_Table(0, 0, 50, 150, nullptr)};
	list->rows(10);
	list->row_height_all(20);
	list->row_resize(0);
	list->cols(9);
	list->col_header(1);
	list->col_resize(1);
*
	// monospaced font
	auto list{new Fl_Check_Browser(0, 0, 250, 150, nullptr)};
	// ¬ conversation * flagged @ attachment N unread J marked junk
//	list->format_char('f'); // Check Browser doesn't have this :(
	list->add("¬ * @ N J Cron <_aud@cfg> if ! OUT ... (Cron Daemon) 31/10/2019 00:17");
	list->add("¬ * @ N J Cron <_aud@cfg> if ! OUT ... (Cron Daemon) 31/10/2019 00:17");
	list->add("¬ * @ N J Cron <_aud@cfg> if ! OUT ... (Cron Daemon) 31/10/2019 00:17");
	list->add("¬ * @ N J Cron <_aud@cfg> if ! OUT ... (Cron Daemon) 31/10/2019 00:17");
	list->add("¬ * @ N J Cron <_aud@cfg> if ! OUT ... (Cron Daemon) 31/10/2019 00:17");
	list->add("¬ * @ N J Cron <_aud@cfg> if ! OUT ... (Cron Daemon) 31/10/2019 00:17");
	list->add("¬ * @ N J Cron <_aud@cfg> if ! OUT ... (Cron Daemon) 31/10/2019 00:17");
*
	parent->add(list);
	return list;
}

void *add_textbox(void *ptr) {
	// needs to be vertical scroll not horizontal
	auto parent{reinterpret_cast<Fl_Group *>(ptr)};
	auto text{new Fl_Text_Display(0, 0, 50, 150, nullptr)};
	parent->add(text);
	text->wrap_mode(Fl_Text_Display::WRAP_AT_BOUNDS, 0);
	auto buf = new Fl_Text_Buffer();
	buf->text("Email email email email email email email email. Email email email email email email email email. Email email email email email email email email.\nEmail email email email email email email email. Email email email email email email email email. Email email email email email email email email.\nEmail email email email email email email email. Email email email email email email email email. Email email email email email email email email.\nEmail email email email email email email email. Email email email email email email email email. Email email email email email email email email.\nEmail email email email email email email email. Email email email email email email email email. Email email email email email email email email.\nEmail email email email email email email email. Email email email email email email email email. Email email email email email email email email.");
	text->buffer(buf);
	return text;
}

class UiPimpl {
	typedef bool (*fnstart)(void *);
	typedef bool (*fnupdate)();
	typedef bool (*fnstop)();
	private:
		static UiPimpl *self;
		bool self_destruct;
		vector<string> plugins;
		vector<PluginHandles> plugin_handles;
		unique_ptr<Fl_Window> win;

		unique_ptr<Fl_Tabs> tabs;
		map<string, Fl_Pack *> tabsmap;
	public:
		UiPimpl(std::vector<std::string> &);
		~UiPimpl();
		bool start(string &);
		bool update();
		bool stop();
		void die();
		void reload();
		shared_ptr<void> &add_tab(const string &);
		void add_button(const string &, fltk_cb_fn, const string &);
//		Fl_Image *get_image(char);
	private:
		static void handle_signal(int);
		static void callback_quit(Fl_Widget *, void *);
		static void callback_tabs(Fl_Widget *, void *);
		void load_config();
		void save_config();
};

// interesting fltk links
// https://www.fltk.org/doc-2.0/html/group__themes.html#ga0
// https://www.fltk.org/doc-1.3/classFl.html#a43e6e0bbbc03cad134d928d4edd48d1d
// https://www.fltk.org/doc-1.3/group__fl__events.html

void UiPimpl::add_button(const string &plbl, fltk_cb_fn fn, const string &label) {
	auto pack{tabsmap.at(plbl)};
	auto btn = new Fl_Button(20, 20, 70, 40, label.c_str());
	btn->callback(fn, nullptr);
	pack->add(btn);
}
*/
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

