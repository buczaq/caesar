#include "decoder.h"

extern std::vector<short> keys;
extern std::vector<std::string> encoded_words;

string decoder::decode(string word, short key)
{
	auto len = word.length();
	for (auto i = 0; i < len; i++) {
		unsigned char temp = static_cast<unsigned char>(word[i] - key);
		(temp >= 'a') ? word[i] = temp : word[i] = static_cast<unsigned char>(temp + 26);
	}
	return word;
}

short decoder::download_key()
{
	short k = keys.back();
	keys.pop_back();
	return k;
}

std::string decoder::download_encoded_word()
{
	std::string w = encoded_words.back();
	encoded_words.pop_back();
	return w;
}
