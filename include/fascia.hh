#pragma once

#include <vector>
#include <string>

class Fascia {
	typedef bool (*fnstart)(void *);
	typedef bool (*fnupdate)();
	typedef bool (*fnstop)();
	private:
		static Fascia *self;
		std::vector<std::string> plugins;
		std::vector<void *> handles;
		std::vector<void *> updates;
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

