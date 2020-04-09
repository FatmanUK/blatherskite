#include <iostream>

#include "api.hh"

using std::string;
using std::cerr;
using std::endl;

const string PNAME = "mailbox";
const string PVERSION = "v0.0.1";
const string TABNAME = "E-Mail";

Fl_Group *tab;

bool start(void *ptr) {
	cerr << "Loading: " << PNAME << " " << PVERSION << "" << endl;
	tab = add_tab(ptr, TABNAME);
	// add ui bits to fascia f
	// it will garbage collect
	tab->end();
	test2(PNAME);
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

