#pragma once

#include <string>
#include <vector>
#include <iostream>

using std::string;

class encoder {
public:
	/*
	 * @desc: get key from shared resource
	 *
	 */
	short download_key();

	/*
	 * @desc: get word from shared resource
	 *
	 */
	string download_word();

	/*
	 * @desc: encode word
	 *
	 */
	string encode(string word, short key);
};