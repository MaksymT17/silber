
#include"Types.hpp"
#include"BfsSearcher.h"
#include"DfsSearcher.h"

// 0 0 0 1 1
// 1[1]0 1 1
// 1 1 0[1]1
// 1 1 0 1 1
// 1 1 1 1 1

int main()
{
	using namespace silber;

	
	Matrix<uint16_t> data(5, 5);
	data(0, 0) = 0; data(1, 0) = 0; data(2, 0) = 0; data(3, 0) = 1; data(4, 0) = 1;
	data(0, 1) = 0; data(1, 1) = 1; data(2, 1) = 0; data(3, 1) = 1; data(4, 1) = 1;
	data(0, 2) = 0; data(1, 2) = 1; data(2, 2) = 0; data(3, 2) = 1; data(4, 2) = 1;
	data(0, 3) = 1; data(1, 3) = 1; data(2, 3) = 0; data(3, 3) = 1; data(4, 3) = 1;
	data(0, 4) = 1; data(1, 4) = 1; data(2, 4) = 1; data(3, 4) = 1; data(4, 4) = 1;

	SearchInfo info;
	info.start = Position{ 1,1 };
	info.search = Position{ 3,1 };
	info.distance = 0;

	SearchInfo infoBfs(info);

	DfsSearcher sd;
	sd.search(info, data);

	BfsSearcher sr;
	sr.search(infoBfs, data);
	return 0;
}
