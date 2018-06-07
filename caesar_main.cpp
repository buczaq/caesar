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

std::atomic_bool ready;
std::vector<short> keys;
std::vector<std::string> message;
std::vector<std::string> dictionary;
std::vector<std::string> encoded_words;

std::mutex keys_mtx;
std::mutex message_mtx;
std::mutex dictionary_mtx;
std::mutex encoded_words_mtx;

std::condition_variable encoded_words_cv;

std::atomic_int live_decoders;

void encode(encoder e)
{
	while(!ready) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		std::cout << "encoder - waiting..." << std::endl;
	}

	while(live_decoders) {
		string word;
		{
			std::unique_lock<std::mutex> lock(message_mtx);
			word = e.download_word();
		//	std::cout << "encoder - word is " << word << std::endl;
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
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	std::cout << "encoder dead" << std::endl;
}

void decode(decoder d)
{
	while(!ready) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		std::cout << "decoder - waiting..." << std::endl;
	}

	int iterations { 500 };

	while(iterations) {
		string encoded_word;
		{
			std::unique_lock<std::mutex> lock(encoded_words_mtx);
			encoded_words_cv.wait(lock);
			encoded_word = d.download_encoded_word();
		//	std::cout << "decoder - encoded word is " << encoded_word << std::endl;
		}
		bool valid_word_found { false };

		while(!valid_word_found) {
			short key;
			{
				std::unique_lock<std::mutex> lock(keys_mtx);
				key = d.download_key();
			//	std::cout << "decoder - key is " << key << std::endl;
			}


			//std::cout << encoded_word << std::endl;

			std::string word = d.decode(encoded_word, key);

			if(std::find(dictionary.begin(), dictionary.end(), word) != dictionary.end()) {
				valid_word_found = true;
			}

			if(valid_word_found)
			{
				std::unique_lock<std::mutex> lock(message_mtx);
				message.push_back(word);
				std::cout << "decoder - found valid word " << word << std::endl;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			iterations--;
			if (!iterations) break;
		}
	}
	//Sstd::cout << "decoder dead" << std::endl;
	live_decoders--;
	std::cout << "decoders: " << live_decoders << std::endl;
}

int main()
{
	srand(time(NULL));

	live_decoders = 45;

	std::unique_lock<std::mutex> lock1 (keys_mtx);
	for (int i = 0; i < 100000; i++) {
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

	for (int i = 0; i < 5; i++) {
		encoders.push_back(std::thread { encode, e });
	}

	for (int i = 0; i < 45; i++) {
		decoders.push_back(std::thread { decode, d });
	}

	for (int i = 0; i < 5; i++) {
		encoders[i].join();
	}

	for (int i = 0; i < 45; i++) {
		decoders[i].join();
	}
}
