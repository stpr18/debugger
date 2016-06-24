#pragma once
#include <cstdint>
#include <array>

class Aes
{
private:
	static const size_t TABLE_SIZE = 256;
	static const size_t KEY_SIZE = 4;
	static const size_t ROUND_SIZE = 10;
	static const size_t ROUND_KEY_SIZE = ROUND_SIZE + 1;

public:
	using table_t = std::array < uint32_t, TABLE_SIZE >;
	using key_t = std::array < uint32_t, KEY_SIZE >;
	using text_t = std::array < uint32_t, 4 >;

	Aes() = delete;
	~Aes() = delete;

	static void EncryptWithTable(const key_t &key, const text_t &in, text_t &out);
	static void EncryptWithInst(const key_t &key, const text_t &in, text_t &out);
	static void EncryptWithApi(const key_t &key, const text_t &in, text_t &out);

private:
	static const uint32_t kRcon[];
	static const uint8_t kSbox[];
	static const table_t kTable[];
};
