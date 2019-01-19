#pragma once
#include <new>
#include <vector>
#include<iostream>

//namespace silber{}

template<class T>
class Matrix
{
public:
	Matrix(size_t width, size_t height)
		: mWidth(width),
		mHeight(height) {
		// throw project defined bad alloc exception
		try {
			mData = std::vector<T>(width * height);
		}
		catch (const std::bad_alloc& e) {
			printf("bad_alloc\n");
		}
		catch (const std::length_error& e) {
			printf("std::length_error\n");
		}
		catch (...) {
			printf("Allocation failed. Reason: unknown.\n");
		}
	}

	size_t getWidth()const { return mWidth; }
	size_t getHeight()const { return mHeight; }

	T& operator()(size_t x, size_t y) {
		return mData[y * mWidth + x];
	}

	T operator()(size_t x, size_t y) const {
		return mData[y * mWidth + x];
	}

private:
	size_t mWidth;
	size_t mHeight;
	std::vector<T> mData;
};

struct Position {
	uint16_t x, y;
};

using Positions = std::vector<Position>;

struct SearchInfo {
	Position start;
	Position search;
	size_t distance;
};

struct SearchingData {
	SearchInfo& info;
	const Matrix<uint8_t>& data;
};

static uint16_t SHIFT = 1u;