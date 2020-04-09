#include <iostream>

#include "api.hh"

using std::string;
using std::cerr;
using std::endl;

const string PNAME = "notebook";
const string PVERSION = "v0.0.1";

bool start(void *) {
	cerr << "Loading: " << PNAME << " " << PVERSION << "" << endl;
#ifndef NDEBUG
	cerr << PNAME << ": Start" << endl;
#endif
	test();
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

