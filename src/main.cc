#include <iostream>
#include <vector>
#include <sstream>
#include <system_error>

#include "api.hh"
#include "dirent.h"

using std::cerr;
using std::endl;
using std::vector;
using std::string;
using std::exception;

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
		auto plugins = enumerate_plugins(plugins_dir);
		Fascia f{plugins};
		bool status = f.start(plugins_dir);
		THROW_IF_FALSE(status, "Failed to start fascia");
		f.load_config();
		while (f.update());
		f.save_config();
		status = f.stop();
		THROW_IF_FALSE(status, "Failed to stop fascia");
	} catch(exception &e) {
		cerr << e.what() << endl;
		return 1;
	}
	return 0;
}

