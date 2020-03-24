#include <iostream>
#include <vector>

#include "dirent.h"
#include "dlfcn.h"
#include "signal.h"

#include "fascia.hh"

using std::cerr;
using std::endl;
using std::vector;
using std::string;

bool ends_with(string &haystack, string needle) {
	if (haystack.length() < needle.length()) {
		return false;
	}
	size_t pos = haystack.length() - needle.length();
	return 0 == haystack.compare(pos, needle.length(), needle);
}

vector<string> enumerate_plugins(string &plugins_dir) {
#ifdef _WIN32
	string ext = ".dll";
#else
	string ext = ".so";
#endif
	vector<string> rv{};
	struct dirent *drnt;
	// segfault if dir doesn't exist - check for this
	DIR *dr{opendir(plugins_dir.c_str())};
	while ((drnt = readdir(dr)) != 0) {
		rv.push_back(drnt->d_name);
		auto iter = rv.end();
		--iter;
		if (!ends_with(rv.back(), ext)) {
			rv.erase(iter);
		}
	}
	return rv;
}

Fascia *Fascia::self{nullptr};

Fascia::Fascia(vector<string> &p) : plugins{p}, handles{}, updates{}, self_destruct{false} {
	self = this;
}

Fascia::~Fascia() { }

bool Fascia::start(string &plugins_dir) {
	void *ptr{nullptr};
	signal(SIGINT, Fascia::handle_signal);
	ui_start();
	for (auto &p : plugins) {
		ptr = dlopen((plugins_dir + "/" + p).c_str(), RTLD_NOW);
		if (ptr == nullptr) return false;
		handles.push_back(ptr);
	}
	for (auto &h : handles) {
		ptr = dlsym(h, "start");
		if (ptr == nullptr) return false;
		if (!(*(fnstart)ptr)(this)) return false;
		ptr = dlsym(h, "update");
		if (ptr == nullptr) return false;
		updates.push_back(ptr);
	}
	return true;
}

bool Fascia::update() {
	if (self_destruct) return false;
	for (auto &u : updates) {
		if (!(*(fnupdate)u)()) return false;
	}
	ui_update();
	return true;
}

bool Fascia::stop() {
	void *ptr{nullptr};
	for (auto &h : handles) {
		ptr = dlsym(h, "stop");
		if (ptr == nullptr) return false;
		if (!(*(fnstop)ptr)()) return false;
		dlclose(h);
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
}

void Fascia::ui_update() {
}

void Fascia::ui_stop() {
}

#define SIGHUP  1   /* Hangup the process */ 
#define SIGINT  2   /* Interrupt the process */ 
#define SIGQUIT 3   /* Quit the process */ 

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

int main(int, char **) {
	string plugins_dir{"/usr/lib/blatherskite"};
#ifndef NDEBUG
	plugins_dir = "/home/fatman/blatherskite/build-debug/bin/plugins";
	cerr << "DEBUG build" << endl;
#endif
	auto plugins = enumerate_plugins(plugins_dir);
	Fascia f{plugins};
	if (f.start(plugins_dir)) {
		f.load_config();
		while (f.update());
		f.save_config();
		if (!f.stop()) {
			// error
		}
	} else {
		// error
	}
	return 0;
}

