#pragma once
#include"Types.hpp"

// interface for Searchers
class ISearcher
{
public:
	virtual ~ISearcher() = default;

	virtual void search(SearchInfo& info, const Matrix<uint8_t>& data) = 0;
};