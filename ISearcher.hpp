#pragma once
#include"Types.hpp"

// interface for Searchers
namespace silber {
	class ISearcher
	{
	public:
		virtual ~ISearcher() = default;

		virtual void search(SearchInfo& info, const Matrix<uint16_t>& data) = 0;
	};
}