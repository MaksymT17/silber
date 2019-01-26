#include"Types.hpp"
#include"BfsSearcher.h"

namespace silber {
	bool isNew(Positions& object, Position& newPos) {
		for (auto pos : object)
			if (pos.x == newPos.x && pos.y == newPos.y)
				return false;

		return true;
	}

	void pushCheckIfNew(Positions& object, Positions& toCheck, Position newPos) {
		if (isNew(object, newPos))
			toCheck.push_back(newPos);
	}

	void bfs(SearchInfo& info, const Matrix<uint8_t>& data, Positions& collected, Positions toCheck, size_t distance = 0) {
		Positions nextCheck{};
		for (auto& position : toCheck) {
			if (data(position.x, position.y)) {
				if (position.x == info.search.x && position.y == info.search.y) {
					info.distance = distance;

					return;
				}

				collected.push_back(position);

				if (position.y - 1 >= 0 && data(position.x, position.y - 1))
					pushCheckIfNew(collected, nextCheck, { position.x, (position.y - 1u) });
				if (position.y + 1 < data.getHeight() && data(position.x, position.y + 1))
					pushCheckIfNew(collected, nextCheck, { position.x, position.y + 1u });
				if (position.x - 1 >= 0 && data(position.x - 1, position.y))
					pushCheckIfNew(collected, nextCheck, { position.x - 1u, position.y });
				if (position.x + 1 < data.getWidth() && data(position.x + 1, position.y))
					pushCheckIfNew(collected, nextCheck, { position.x + 1u, position.y });

			}
		}
		bfs(info, data, collected, nextCheck, ++distance);
	}

	void BfsSearcher::search(SearchInfo& info, const Matrix<uint8_t>& data) {

		Positions empty;
		bfs(info, data, empty, { info.start });
	}
}