#include "api.hh"

#include "dirent.h"

#include <system_error>
#include <iostream>

using std::string;
using std::vector;
using std::exception;
using std::cerr;
using std::endl;

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

int main(int, char **) {
#ifndef NDEBUG
	cerr << "DEBUG build" << endl;
#endif
	string plugins_dir{XSTR(PLUGINS_DIR)};
	expand_bash_tilde(plugins_dir);
#ifndef NDEBUG
	cerr << "Looking for plugins in: " << plugins_dir << endl;
#endif
	try {
		bool is_dir = path_is_extant_dir(plugins_dir);
		THROW_IF_FALSE(is_dir, "Plugin dir " + plugins_dir + " not found");
		bool can_read = dir_allows_read(plugins_dir);
		THROW_IF_FALSE(can_read, "Plugin dir " + plugins_dir + " not readable");
		bool can_exec = dir_allows_exec(plugins_dir);
		THROW_IF_FALSE(can_exec, "Plugin dir " + plugins_dir + " not traversible");
		auto plugins = enumerate_plugins(plugins_dir);
		Fascia f{plugins};
		bool status = f.start(plugins_dir);
		THROW_IF_FALSE(status, "Failed to start fascia");
		while (f.update());
		status = f.stop();
		THROW_IF_FALSE(status, "Failed to stop fascia");
	} catch(exception &e) {
		cerr << e.what() << endl;
		return 1;
	}
	return 0;
}

