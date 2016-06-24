#include <emmintrin.h>
#include <wmmintrin.h>
#include <smmintrin.h>
#include "aes.h"

static __m128i KeyExpansionAssist(__m128i temp1, __m128i temp2)
{
	temp2 = _mm_shuffle_epi32(temp2, _MM_SHUFFLE(3, 3, 3, 3));
	temp1 = _mm_xor_si128(temp1, _mm_slli_si128(temp1, 0x4));
	temp1 = _mm_xor_si128(temp1, _mm_slli_si128(temp1, 0x4));
	temp1 = _mm_xor_si128(temp1, _mm_slli_si128(temp1, 0x4));
	return _mm_xor_si128(temp1, temp2);
}

void Aes::EncryptWithInst(const key_t &key, const text_t &in, text_t &out)
{
	std::array < __m128i, ROUND_KEY_SIZE > rkey;

	//KeyExpansion
	rkey[0] = _mm_setr_epi32(key[0], key[1], key[2], key[3]);
	rkey[1] = KeyExpansionAssist(rkey[0], _mm_aeskeygenassist_si128(rkey[0], 0x01));
	rkey[2] = KeyExpansionAssist(rkey[1], _mm_aeskeygenassist_si128(rkey[1], 0x02));
	rkey[3] = KeyExpansionAssist(rkey[2], _mm_aeskeygenassist_si128(rkey[2], 0x04));
	rkey[4] = KeyExpansionAssist(rkey[3], _mm_aeskeygenassist_si128(rkey[3], 0x08));
	rkey[5] = KeyExpansionAssist(rkey[4], _mm_aeskeygenassist_si128(rkey[4], 0x10));
	rkey[6] = KeyExpansionAssist(rkey[5], _mm_aeskeygenassist_si128(rkey[5], 0x20));
	rkey[7] = KeyExpansionAssist(rkey[6], _mm_aeskeygenassist_si128(rkey[6], 0x40));
	rkey[8] = KeyExpansionAssist(rkey[7], _mm_aeskeygenassist_si128(rkey[7], 0x80));
	rkey[9] = KeyExpansionAssist(rkey[8], _mm_aeskeygenassist_si128(rkey[8], 0x1b));
	rkey[10] = KeyExpansionAssist(rkey[9], _mm_aeskeygenassist_si128(rkey[9], 0x36));

	//Encrypt
	__m128i state = _mm_setr_epi32(in[0], in[1], in[2], in[3]);
	state = _mm_xor_si128(state, rkey[0]);
	state = _mm_aesenc_si128(state, rkey[1]);
	state = _mm_aesenc_si128(state, rkey[2]);
	state = _mm_aesenc_si128(state, rkey[3]);
	state = _mm_aesenc_si128(state, rkey[4]);
	state = _mm_aesenc_si128(state, rkey[5]);
	state = _mm_aesenc_si128(state, rkey[6]);
	state = _mm_aesenc_si128(state, rkey[7]);
	state = _mm_aesenc_si128(state, rkey[8]);
	state = _mm_aesenc_si128(state, rkey[9]);
	state = _mm_aesenclast_si128(state, rkey[10]);

	out[0] = _mm_extract_epi32(state, 0);
	out[1] = _mm_extract_epi32(state, 1);
	out[2] = _mm_extract_epi32(state, 2);
	out[3] = _mm_extract_epi32(state, 3);
}
