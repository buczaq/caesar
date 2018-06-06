#pragma once

#include <string>
#include <vector>

using std::string;

class decoder {
public:
	/*
	 * @desc: get key from shared resource
	 *
	 */
	short download_key();

	/*
	 * @desc: get encoded word from shared resource
	 *
	 */
	string download_encoded_word();

	/*
	 * @desc: decode word
	 *
	 */
	string decode(string word, short key);
};