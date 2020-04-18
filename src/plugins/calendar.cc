#include <iostream>

#include "api.hh"

using std::string;
using std::cerr;
using std::endl;

const string PNAME = "calendar";
const string PVERSION = "v0.0.1";
const string TABNAME = "&Calendar";

void *tab{nullptr};

bool start(void *ui) {
	cerr << "Loading: " << PNAME << " " << PVERSION << "" << endl;
#ifndef NDEBUG
	cerr << "DEBUG build" << endl;
#endif
	tab = create_tab(ui, TABNAME);
	return true;
}

bool update() {
#ifndef NDEBUG
//	cerr << PNAME << ": Update" << endl;
#endif
	return true;
}

bool stop() {
#ifndef NDEBUG
	cerr << PNAME << ": Stop" << endl;
#endif
	return true;
}

