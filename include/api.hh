#pragma once

#include <memory>
#include <string>
#include <vector>

#include "FL/Fl.H"
#include "FL/Fl_Window.H"

// These need to be defines so that __LINE__ and __FILE__ work.
#define THROW_IF_FALSE(c, m) if ((c) == false) \
	throw_custom((m), __FILE__, __LINE__, __FUNCTION__)
#define THROW_IF_NULLPTR(c, m) if ((c) == nullptr) \
	throw_custom((m), __FILE__, __LINE__, __FUNCTION__)

// Standard stringizing macros. Useful for defining strings in the build
// system.
#define XSTR(s) STR(s)
#define STR(s) #s

// Configure the compiler to export/import functions using the same
// API header for plugins and the main program.
#ifdef API
	#ifdef _WIN32
		#define APICALL __declspec(dllexport)
	#else
		#define APICALL __attribute__((visibility("default")))
	#endif
#else
	#ifdef _WIN32
		#define APICALL __declspec(dllimport)
		#define PLUGINCALL __declspec(dllexport)
	#else
		#define APICALL 
		#define PLUGINCALL __attribute__((visibility("default")))
	#endif

	extern "C" {
		PLUGINCALL bool start(void *);
		PLUGINCALL bool update();
		PLUGINCALL bool stop();
	}
#endif

// Exported API functions.
extern "C" {
	APICALL void test();
	APICALL void test2(std::string);
	// message, file, line, function name
	APICALL void throw_custom(std::string, std::string, int, std::string);
	APICALL std::string expand_bash_tilde(std::string);
	APICALL bool ends_with(std::string &, std::string);
	APICALL bool path_is_extant_dir(std::string);
	APICALL bool dir_allows_read(std::string);
	APICALL bool dir_allows_write(std::string);
	APICALL bool dir_allows_exec(std::string);
	APICALL Fl_Group *add_tab(void *, const std::string &);
}

struct PluginHandles {
	std::string name;
	void *handle;
	void *start;
	void *update;
	void *stop;
};

class UiPimpl;

class Fascia {
	private:
		std::unique_ptr<UiPimpl> pimpl;
	public:
		Fascia(std::vector<std::string> &);
		~Fascia();
		bool start(std::string &);
		bool update();
		bool stop();
		void die();
		void reload();
};

