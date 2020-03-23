#include <iostream>
#include <vector>

#include "dirent.h"
#include "dlfcn.h"

using std::cerr;
using std::endl;
using std::vector;
using std::string;

bool ends_with(std::string &haystack, std::string needle) {
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

class Fascia {
	typedef bool (*fnstart)();
	typedef bool (*fnupdate)();
	typedef bool (*fnstop)();
	private:
		vector<string> plugins;
		vector<void *> handles;
		vector<void *> updates;
	public:
		Fascia(vector<string> &);
		~Fascia();
		bool start(string &);
		bool update();
		bool stop();
};

Fascia::Fascia(vector<string> &p) : plugins{p} { }

Fascia::~Fascia() { }

bool Fascia::start(string &plugins_dir) {
	void *ptr{nullptr};
	for (auto &p : plugins) {
		ptr = dlopen((plugins_dir + "/" + p).c_str(), RTLD_NOW);
		if (ptr == nullptr) return false;
		handles.push_back(ptr);
	}
	for (auto &h : handles) {
		ptr = dlsym(h, "start");
		if (ptr == nullptr) return false;
		if (!(*(fnstart)ptr)()) return false;
		// find update
		ptr = dlsym(h, "update");
		if (ptr == nullptr) return false;
		updates.push_back(ptr);
	}
	return true;
}

bool Fascia::update() {
	for (auto &u : updates) {
		if (!(*(fnupdate)u)()) return false;
	}
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
	return true;
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
		while (f.update());
		if (!f.stop()) {
			// error
		}
	} else {
		// error
	}
	return 0;
}

