#include <iostream>

#include "api.hh"

using std::string;
using std::cerr;
using std::endl;

const string PNAME = "mailbox";
const string PVERSION = "v0.0.1";

bool start(void *f) {
	cerr << "Loading: " << PNAME << " " << PVERSION << "" << endl;
#ifndef NDEBUG
	cerr << PNAME << ": Start" << endl;
#endif
	auto fascia{reinterpret_cast<Fascia *>(f)};
	// add ui bits to fascia f
	// it will garbage collect
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

