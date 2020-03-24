#include <iostream>

#include "fascia.hh"

using std::string;
using std::cerr;
using std::endl;

const string PNAME = "mailbox";
const string PVERSION = "v0.0.1";

extern "C" bool start(void *f) {
	cerr << "Loading: " << PNAME << " " << PVERSION << "" << endl;
#ifndef NDEBUG
	cerr << PNAME << ": Start" << endl;
#endif
	auto fascia{reinterpret_cast<Fascia *>(f)};
	// add ui bits to fascia f
	// it will garbage collect
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

