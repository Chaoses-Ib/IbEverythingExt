#pragma once
#include <vector>
#define IB_PINYIN_ENCODING 8
#include <IbPinyinLib/Pinyin.hpp>

int match(const char8_t* pattern, const char8_t* subject, int length, std::vector<pinyin::PinyinFlagValue>& flags, int* offsets, int offsetcount);