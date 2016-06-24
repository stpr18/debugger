#include <cstdlib>
#include <thread>
#include <chrono>
#include <random>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <windows.h>
#include "aes.h"

template <typename T, std::size_t Size>
void PrintHex(const std::array<T, Size> &ary)
{
	for (T v : ary)
		std::cout << std::hex << std::setw(8) << std::setfill('0') << std::uppercase << v << " " << std::dec;
}

int main()
{
	std::mt19937 random(std::random_device{}());

	OutputDebugString(TEXT("Start"));

	for (;;) {
		Aes::key_t key = {};
		Aes::text_t plain = {};
		Aes::text_t cipher;

		std::generate(std::begin(key), std::end(key), std::ref(random));
		std::generate(std::begin(plain), std::end(plain), std::ref(random));
		Aes::EncryptWithTable(key, plain, cipher);

		std::cout << "Current thread number:" << GetCurrentThreadId() << std::endl;

		std::cout << "K:";
		PrintHex(key);
		std::cout << std::endl;

		std::cout << "P:";
		PrintHex(plain);
		std::cout << std::endl;

		std::cout << "C:";
		PrintHex(cipher);
		std::cout << std::endl;

		OutputDebugString(TEXT("---"));

		//std::cout << "---" << std::endl;

		//std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	OutputDebugString(TEXT("End"));

	return EXIT_SUCCESS;
}

