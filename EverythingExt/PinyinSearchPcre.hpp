#pragma once
#include <stdexcept>
#include "PinyinSearch.hpp"

class PinyinSearchPcre : public PinyinSearch {
public:
    // throw std::runtime_error for unsupported Everything version
    PinyinSearchPcre();
    ~PinyinSearchPcre() override;
};