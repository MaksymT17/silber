
#include"BinarySearcher.h"

template<typename T>
int binarySearching(std::vector<T>& arr, int l, int r, T x)
{
	if (r >= l) {
		int mid = l + (r - l) / 2;

		if (arr[mid] == x)
			return mid;

		if (arr[mid] > x)
			return binarySearching(arr, l, mid - 1, x);

		return binarySearching(arr, mid + 1, r, x);
	}

	return -1;
}

template<typename T>
void bubbleSort(std::vector<T>& arr)
{
	const size_t end = arr.size() - 1;
	bool swapped;
	for (size_t i = 0; i < arr.size() - 1; i++) {
		swapped = false;
		for (size_t j = 0; j < end - i; j++) {
			if (arr[j] > arr[j + 1]) {
				std::swap(arr[j], arr[j + 1]);
				swapped = true;
			}
		}

		if (swapped == false)
			break;
	}
}

namespace silber {

	int BinarySearcher::binarySearch(std::vector<int> arr, const int element, bool ordered) {
		if (!ordered)
			bubbleSort(arr);

		return binarySearching(arr, 0, arr.size() - 1, element);
	}

	int BinarySearcher::binarySearch(std::vector<size_t> arr, const size_t element, bool ordered) {
		if (!ordered)
			bubbleSort(arr);

		return binarySearching(arr, 0, arr.size() - 1, element);
	}
}