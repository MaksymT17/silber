#include"DfsSearcher.h"

namespace silber {

	// 0 - empty cell, otherwise acceptable value for algorithm
	void dfs(silber::SearchInfo& info, const silber::Matrix<uint8_t>& data, silber::Position position, size_t distance = 0) {
		using namespace silber;
		Matrix<uint8_t> newData(data);
		newData(position.x, position.y) = 0;

		if (distance > info.distance && info.distance != 0) {
			return;
		}
		else if (position.x == info.search.x && position.y == info.search.y) {
			info.distance = distance;
		}

		if (position.y + 1 < newData.getHeight() && newData(position.x, position.y + SHIFT))
			dfs(info, newData, { position.x, position.y + SHIFT }, distance + SHIFT);

		if (position.y > 0 && newData(position.x, position.y - SHIFT))
			dfs(info, newData, { position.x, position.y - SHIFT }, distance + SHIFT);

		if (position.x + 1 < newData.getWidth() && newData(position.x + SHIFT, position.y))
			dfs(info, newData, { position.x + SHIFT, position.y }, distance + SHIFT);

		if (position.x > 0 && newData(position.x - SHIFT, position.y))
			dfs(info, newData, { position.x - SHIFT, position.y }, distance + SHIFT);

	}
}

namespace silber {
	void DfsSearcher::search(SearchInfo& info, const Matrix<uint8_t>& data) {
		dfs(info, data, { info.start });
	}
}