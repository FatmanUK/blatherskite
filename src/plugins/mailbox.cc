#include <iostream>

#include "api.hh"

#include "FL/Fl_Button.H"

using std::string;
using std::cerr;
using std::endl;

const string PNAME = "mailbox";
const string PVERSION = "v0.0.1";
const string TABNAME = "E-Mail";

Fl_Group *tab;
Fl_Button *btn;

bool start(void *ptr) {
	cerr << "Loading: " << PNAME << " " << PVERSION << "" << endl;
	tab = add_tab(ptr, TABNAME);
	// add ui bits to fascia f
	btn = new Fl_Button(20, 20, 70, 40, "Mail");
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

