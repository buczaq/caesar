#include "encoder.h"

extern std::vector<short> keys;
extern std::vector<std::string> message;

std::string encoder::encode(std::string word, short key)
{
	auto len = word.length();
	for (auto i = 0; i < len; i++) {
		unsigned char temp = static_cast<unsigned char>(word[i] + key);
		(temp <= 'z') ? (word[i] = temp) : (word[i] = static_cast<unsigned char>(temp - 26));
	}
	return word;
}

short encoder::download_key()
{
	short k = keys.back();
	keys.pop_back();
	return k;
}

std::string encoder::download_word()
{
	std::string w = message.back();
	message.pop_back();
	return w;
}
