#pragma once

#include"ISearcher.hpp"

class DfsSearcher : public ISearcher {
public:
	DfsSearcher() = default;
	virtual void search(SearchInfo& info, const Matrix<uint8_t>& data) override;
};