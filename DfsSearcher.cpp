#include"DfsSearcher.h"

namespace {

	// 0 - empty cell, otherwise acceptable value for algorithm
	void dfs(SearchInfo& info, const Matrix<uint8_t>& data, Position position, size_t distance = 0) {
		Matrix<uint8_t> newData(data);
		newData(position.x, position.y) = 0;

		if (distance > info.distance && info.distance != 0) {
			return;
		}
		else if (position.x == info.search.x && position.y == info.search.y) {
			info.distance = distance;
		}

		if (position.y + 1 < newData.getHeight() && newData(position.x, position.y + 1))
			dfs(info, newData, { position.x, uint16_t(position.y + SHIFT) }, distance + 1);

		if (position.y > 0 && newData(position.x, position.y - 1))
			dfs(info, newData, { position.x, uint16_t(position.y - SHIFT) }, distance + 1);

		if (position.x + 1 < newData.getWidth() && newData(position.x + 1, position.y))
			dfs(info, newData, { uint16_t(position.x + SHIFT), position.y }, distance + 1);

		if (position.x > 0 && newData(position.x - 1, position.y))
			dfs(info, newData, { uint16_t(position.x - SHIFT), position.y }, distance + 1);

	}
}

void DfsSearcher::search(SearchInfo& info, const Matrix<uint8_t>& data) {
	dfs(info, data, { info.start });
}