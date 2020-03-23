#include <iostream>

using std::string;
using std::cerr;
using std::endl;

const string PNAME = "mailbox";

extern "C" bool start() {
	cerr << PNAME << ": Start" << endl;
	return true;
}

extern "C" bool update() {
	cerr << PNAME << ": Update" << endl;
	return true;
}

extern "C" bool stop() {
	cerr << PNAME << ": Stop" << endl;
	return true;
}

