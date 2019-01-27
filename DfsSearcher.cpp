#include"DfsSearcher.h"

namespace silber {

	// 0 - empty cell, otherwise acceptable value for algorithm
	void dfs(silber::SearchInfo& info, const Matrix<uint16_t>& data, Position position, uint16_t distance = 0) {
		using namespace silber;
		Matrix<uint16_t> newData(data);
		newData(position.x, position.y) = 0;

		if (distance > info.distance && info.distance != 0) {
			return;
		}
		else if (position.x == info.search.x && position.y == info.search.y) {
			info.distance = distance;
		}
		
		if (position.y + SHIFT < newData.getHeight() && newData(position.x, position.y + SHIFT))
			dfs(info, newData, { position.x, static_cast<uint16_t>(position.y + SHIFT) }, distance + SHIFT);

		if (position.y  && newData(position.x, position.y - SHIFT))
			dfs(info, newData, { position.x, static_cast<uint16_t>(position.y - SHIFT) }, distance + SHIFT);

		if (position.x + SHIFT < newData.getWidth() && newData(position.x + SHIFT, position.y))
			dfs(info, newData, { static_cast<uint16_t>(position.x + SHIFT), position.y }, distance + SHIFT);

		if (position.x  && newData(position.x - SHIFT, position.y))
			dfs(info, newData, { static_cast<uint16_t>(position.x - SHIFT), position.y }, distance + SHIFT);

	}
}

namespace silber {
	void DfsSearcher::search(SearchInfo& info, const Matrix<uint16_t>& data) {
		dfs(info, data, { info.start });
	}
}