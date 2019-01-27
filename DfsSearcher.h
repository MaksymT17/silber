#pragma once

#include"ISearcher.hpp"

namespace silber {
	class DfsSearcher : public ISearcher {
	public:
		DfsSearcher() = default;
		virtual void search(SearchInfo& info, const Matrix<uint16_t>& data) override;
	};
}