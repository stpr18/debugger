#include "aes.h"
#include "util.h"

void Aes::EncryptWithTable(const key_t &key, const text_t &in, text_t &out)
{
	size_t i, j;
	text_t state;
	std::array < uint32_t, 4 * ROUND_KEY_SIZE > rkey;

	//KeyExpansion
	for (i = 0; i < KEY_SIZE; ++i)
		rkey[i] = key[i];

	uint32_t temp;
	for (; i < 4 * ROUND_KEY_SIZE; ++i) {
		temp = rkey[i - 1];
		if (i % 4 == 0) {
			temp =
				(kTable[2][util::int32toq1(temp)] & 0xff000000) ^
				(kTable[3][util::int32toq2(temp)] & 0x00ff0000) ^
				(kTable[0][util::int32toq3(temp)] & 0x0000ff00) ^
				(kTable[1][util::int32toq0(temp)] & 0x000000ff) ^
				kRcon[i / 4];
		}
		rkey[i] = rkey[i - 4] ^ temp;
	}

	//Encrypt
	for (i = 0; i < 4; ++i)
		out[i] = in[i] ^ rkey[i];

	for (i = 1; i <= (10 - 1); ++i) {
		for (j = 0; j < 4; ++j) {
			state[j] =
				kTable[0][util::int32toq0(out[(j + 0) % 4])] ^
				kTable[1][util::int32toq1(out[(j + 1) % 4])] ^
				kTable[2][util::int32toq2(out[(j + 2) % 4])] ^
				kTable[3][util::int32toq3(out[(j + 3) % 4])] ^
				rkey[4 * i + j];
		}
		out = state;
	}

	for (j = 0; j < 4; ++j) {
		state[j] =
			(kTable[2][util::int32toq0(out[(j + 0) % 4])] & 0xff000000) ^
			(kTable[3][util::int32toq1(out[(j + 1) % 4])] & 0x00ff0000) ^
			(kTable[0][util::int32toq2(out[(j + 2) % 4])] & 0x0000ff00) ^
			(kTable[1][util::int32toq3(out[(j + 3) % 4])] & 0x000000ff) ^
			rkey[4 * 10 + j];
	}
	out = state;
}
