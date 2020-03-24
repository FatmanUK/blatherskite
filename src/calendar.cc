#include <iostream>

#include "fascia.hh"

using std::string;
using std::cerr;
using std::endl;

const string PNAME = "calendar";
const string PVERSION = "v0.0.1";

extern "C" bool start(void *) {
	cerr << "Loading: " << PNAME << " " << PVERSION << "" << endl;
#ifndef NDEBUG
	cerr << PNAME << ": Start" << endl;
#endif
	return true;
}

extern "C" bool update() {
#ifndef NDEBUG
//	cerr << PNAME << ": Update" << endl;
#endif
	return true;
}

extern "C" bool stop() {
#ifndef NDEBUG
	cerr << PNAME << ": Stop" << endl;
#endif
	return true;
}

