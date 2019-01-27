#pragma once

#include"ISearcher.hpp"

namespace silber {
	class BfsSearcher : public ISearcher {
	public:
		BfsSearcher() = default;
		virtual void search(SearchInfo& info, const Matrix<uint16_t>& data) override;
	};
}