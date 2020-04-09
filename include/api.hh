#pragma once

#define THROW_IF_FALSE(c, m) if ((c) == false) \
	throw_custom((m), __FILE__, __LINE__, __FUNCTION__)

#define THROW_IF_NULLPTR(c, m) if ((c) == nullptr) \
	throw_custom((m), __FILE__, __LINE__, __FUNCTION__)

// Standard stringizing macros.
#define XSTR(s) STR(s)
#define STR(s) #s

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

#include <string>
#include <vector>

#include "FL/Fl.H"

extern "C" {
	APICALL void test();
	APICALL void test2(std::string);
}

struct PluginHandles {
	std::string name;
	void *handle;
	void *start;
	void *update;
	void *stop;
};

class Fascia {
	typedef bool (*fnstart)(void *);
	typedef bool (*fnupdate)();
	typedef bool (*fnstop)();
	private:
		static Fascia *self;
		std::vector<std::string> plugins;
		std::vector<PluginHandles> plugin_handles;
		bool self_destruct;
	public:
		Fascia(std::vector<std::string> &);
		~Fascia();
		bool start(std::string &);
		bool update();
		bool stop();
		void die();
		void reload();
		void load_config();
		void save_config();
	private:
		static void handle_signal(int);
		void ui_start();
		void ui_update();
		void ui_stop();
};

void throw_custom(std::string, std::string, int, std::string);

