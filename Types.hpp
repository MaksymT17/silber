#pragma once

#include <new>
#include <vector>
#include <iostream>

namespace silber {

	const uint16_t SHIFT = 1u;
	
	template<class T>
	class Matrix
	{
	public:
		Matrix(uint16_t width, uint16_t height)
			: mWidth(width),
			mHeight(height) {
			// throw project defined bad alloc exception
			try {
				mData = std::vector<T>(width * height);
			}
			catch (const std::bad_alloc& e) {
				printf("bad_alloc [%s]\n", e.what());
			}
			catch (const std::length_error& e) {
				printf("std::length_error[%s]\n", e.what());
			}
			catch (...) {
				printf("Allocation failed. Reason: unknown.\n");
			}
		}

		uint16_t getWidth()const { return mWidth; }
		uint16_t getHeight()const { return mHeight; }

		T& operator()(uint16_t x, uint16_t y) {
			return mData[y * mWidth + x];
		}

		T operator()(uint16_t x, uint16_t y) const {
			return mData[y * mWidth + x];
		}

	private:
		uint16_t mWidth;
		uint16_t mHeight;
		std::vector<T> mData;
	};

	struct Position {
		uint16_t x, y;
	};

	using Positions = std::vector<Position>;

	struct SearchInfo {
		Position start;
		Position search;
		uint16_t distance;
	};

	struct SearchingData {
		SearchInfo& info;
		const Matrix<uint16_t>& data;
	};
}