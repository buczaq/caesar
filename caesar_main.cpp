#include "encoder.h"
#include "decoder.h"
#include <iostream>
#include <thread>
#include <vector>
#include <ctime>
#include <cstdio>
#include <string>
#include <condition_variable>
#include <atomic>
#include <fstream>
#include <algorithm>
#include <ncurses.h>

std::atomic_bool ready;
std::vector<short> keys;
std::vector<std::string> message;
std::vector<std::string> dictionary;
std::vector<std::string> encoded_words;
std::vector<std::string> decoded_words;
std::atomic_int all_decode_operations;
std::atomic_int successfull_decode_operations;
std::atomic<float> success_ratio;

std::mutex keys_mtx;
std::mutex message_mtx;
std::mutex dictionary_mtx;
std::mutex encoded_words_mtx;
std::mutex decoded_words_mtx;
std::mutex semaphore_mtx;

std::condition_variable encoded_words_cv;

std::atomic_int live_decoders;
std::atomic_int live_encoders;
std::atomic_int available_words_to_encode;

bool my_amazing_semaphore(std::atomic_int& variable)
{
	std::unique_lock<std::mutex> semaphore_mtx;
	return variable > 0 ? (--variable, true) : false;
}

void encode(encoder e)
{
	while(live_decoders) {
		if (!my_amazing_semaphore(available_words_to_encode)) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		string word;
		{
			std::unique_lock<std::mutex> lock(message_mtx);
			word = e.download_word();
			//std::cout << "encoder - word is " << word << std::endl;
		}

		short key;
		{
			std::unique_lock<std::mutex> lock(keys_mtx);
			key = e.download_key();
		//	std::cout << "encoder - key is " << key << std::endl;
		}

		std::string encoded_word = e.encode(word, key);

		{
			std::unique_lock<std::mutex> lock(encoded_words_mtx);
			encoded_words.push_back(encoded_word);
		//	std::cout << "encoder - encoded word is " << encoded_word << std::endl;
		}

		encoded_words_cv.notify_one();
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	live_encoders--;
	//std::cout << "encoder dead" << std::endl;
}

void decode(decoder d)
{
	while(!ready) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		//std::cout << "decoder - waiting..." << std::endl;
	}

	int iterations { 500 };

	while(iterations) {
		string encoded_word;
		{
			std::unique_lock<std::mutex> lock(encoded_words_mtx);
			encoded_words_cv.wait(lock);
			encoded_word = d.download_encoded_word();
			//std::cout << "decoder - encoded word is " << encoded_word << std::endl;
		}
		bool valid_word_found { false };

		while(!valid_word_found) {
			short key;
			{
				std::unique_lock<std::mutex> lock(keys_mtx);
				key = d.download_key();
				//std::cout << "decoder - key is " << key << std::endl;
			}


			//std::cout << encoded_word << std::endl;

			std::string word = d.decode(encoded_word, key);
			all_decode_operations++;

			if(std::find(dictionary.begin(), dictionary.end(), word) != dictionary.end()) {
				valid_word_found = true;
			}

			if(valid_word_found)
			{
				successfull_decode_operations++;
				std::unique_lock<std::mutex> lock(decoded_words_mtx);
				decoded_words.push_back(word);
				//std::cout << "decoder - found valid word " << word << std::endl;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			iterations--;
			if (!iterations) break;
		}
	}
	//std::cout << "decoder dead" << std::endl;
	live_decoders--;
	//std::cout << "decoders: " << live_decoders << std::endl;
}

void operations_database()
{
	while(live_decoders) {
		successfull_decode_operations = decoded_words.size();
		if (successfull_decode_operations && all_decode_operations) {
			success_ratio = static_cast<float>(successfull_decode_operations) / static_cast<float>(all_decode_operations);
			//std::cout << live_decoders << " decoders." << std::endl;
			//std::cout << succesfull_decode_operations << "/" << all_decode_operations << std::endl;
			//std::cout << (success_ratio * 100) << " % of operations succeded!" << std::endl;
		} else {
			//std::cout << "No sucessfull operations yet." << std::endl;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}

void visualization(WINDOW* w_live,
                   WINDOW* w_visual,
                   WINDOW* w_recently,
                   WINDOW* w_success,
                   WINDOW* w_mode,
                   WINDOW* w_author)
{
	while(live_decoders) {
		werase(w_live);
		werase(w_visual);
		werase(w_recently);
		werase(w_success);
		werase(w_mode);

		mvwprintw(w_live, 1, 1, "%d decoders alive", (int)live_decoders);
		mvwprintw(w_live, 3, 1, "%d encoders alive", (int)live_encoders);
		wrefresh(w_live);


		for (int i = 1; i < 23; i++) {
			int h = rand()%7;
			for (int j = 0; j < h; j++) {
				wmove(w_visual, 6 - j, i);
				waddch(w_visual, (char)219);
			}
		}

		wrefresh(w_visual);

		{
			mvwprintw(w_recently, 1, 0, "recently decoded:");
			std::unique_lock<std::mutex> lock1(decoded_words_mtx);
			if (decoded_words.size()) {
				mvwprintw(w_recently, 3, 4, "%s", decoded_words.back().c_str());
				if (decoded_words.size() > 1) {
					mvwprintw(w_recently, 4, 4, "%s", (decoded_words.rbegin()[1]).c_str());
					if (decoded_words.size() > 2) {
						mvwprintw(w_recently, 5, 4, "%s", (decoded_words.rbegin()[2]).c_str());
					}
				}
			}
		}

		wrefresh(w_recently);

		mvwprintw(w_success, 1, 0, "all decoding tries: %i", (int)all_decode_operations);
		mvwprintw(w_success, 2, 0, "succesfull decodes: %i", (int)successfull_decode_operations);
		mvwprintw(w_success, 4, 0, "success ratio: %.3f%%", (float)success_ratio * 100.0);

		wrefresh(w_success);

		mvwprintw(w_mode, 0, 0, "DECODING MODE");

		wrefresh(w_mode);

		mvwprintw(w_author, 0, 0, "== KRZYSZTOF BUCZAK, 2018 - feel free to steal! ==");

		wrefresh(w_author);

		std::this_thread::sleep_for(std::chrono::milliseconds(300));
	}
}

int main()
{
	srand(time(NULL));

	live_decoders = 45;
	live_encoders = 5;

	std::unique_lock<std::mutex> lock1 (keys_mtx);
	for (int i = 0; i < 1000000; i++) {
		keys.push_back(rand()%26 + 1);
	}
	lock1.unlock();

	std::string line;
	{
		std::lock_guard<std::mutex> lock(message_mtx);
	    std::ifstream infile;
    	infile.open("message.txt");

	    infile >> line;
	    while (!infile.eof()) {
	    	message.push_back(line);
	    	infile >> line;
    	}
    	infile.close();
    	available_words_to_encode = message.size();
	}

	{
		std::lock_guard<std::mutex> lock(dictionary_mtx);
	    std::ifstream infile;
    	infile.open("dictionary.txt");

	    infile >> line;
	    while (!infile.eof()) {
	    	dictionary.push_back(line);
	    	infile >> line;
    	}
    	infile.close();
	}

	successfull_decode_operations = 0;
	all_decode_operations = 0;

    initscr();
    curs_set(0);


	WINDOW* w_live = newwin(7, 25, 1, 1);
    WINDOW* w_visual = newwin(7, 25, 8, 1);
    WINDOW* w_recently = newwin(7, 25, 1, 26);
    WINDOW* w_success = newwin(6, 25, 8, 26);
    WINDOW* w_mode = newwin(1, 25, 14, 26);
    WINDOW* w_author = newwin(1, 50, 16, 0);
	
	WINDOW* w_general = newwin(16, 52, 0, 0);
	box(w_general, 0, 0);
	wrefresh(w_general);

	ready = true;

	encoder e {};
	decoder d {};
	// TODO: remove testing in final product
	/*for (int i = 0; i < 10; i++) {
		std::cout << keys.back();
		std::cout << e.encode("abcdefghijklmnopqrstuvwxyz", keys.back()) << std::endl;
		keys.pop_back();
	}*/
	std::vector<std::thread> encoders;
	std::vector<std::thread> decoders;
	std::thread database { operations_database };
	std::thread visual { visualization,
	                     w_live,
	                     w_visual,
	                     w_recently,
	                     w_success,
	                     w_mode,
	                     w_author };

	for (int i = 0; i < 5; i++) {
		encoders.push_back(std::thread { encode, e });
	}

	for (int i = 0; i < 45; i++) {
		decoders.push_back(std::thread { decode, d });
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}

	for (int i = 0; i < 5; i++) {
		encoders[i].join();
	}

	for (int i = 0; i < 45; i++) {
		decoders[i].join();
	}

	database.join();
	visual.join();

	endwin();
}
