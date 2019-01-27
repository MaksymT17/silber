#pragma once

#include"Types.hpp"

namespace silber {

	class BinarySearcher {
	public:
		BinarySearcher() = default;

		static int binarySearch(std::vector<int> arr, const int element, bool ordered);
		static int binarySearch(std::vector<size_t> arr, const size_t element, bool ordered);
	};
}