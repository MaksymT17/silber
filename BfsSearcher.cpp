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

	void bfs(SearchInfo& info, const Matrix<uint16_t>& data, Positions& collected, Positions toCheck, uint16_t distance = 0) {
		Positions nextCheck{};
		for (auto& position : toCheck) {
			if (data(position.x, position.y)) {

				if (position.x == info.search.x && position.y == info.search.y) {
					info.distance = distance;

					return;
				}

				collected.push_back(position);

				if (position.y - SHIFT >= 0 && data(position.x, position.y - SHIFT))
					pushCheckIfNew(collected, nextCheck, { position.x, static_cast<uint16_t>(position.y - SHIFT)});
				if (position.y + SHIFT < data.getHeight() && data(position.x, static_cast<uint16_t>(position.y + SHIFT)))
					pushCheckIfNew(collected, nextCheck, { position.x, static_cast<uint16_t>(position.y + SHIFT) });
				if (position.x - SHIFT >= 0 && data(static_cast<uint16_t>(position.x - SHIFT), position.y))
					pushCheckIfNew(collected, nextCheck, { static_cast<uint16_t>(position.x - SHIFT), position.y });
				if (position.x + SHIFT < data.getWidth() && data(static_cast<uint16_t>(position.x + SHIFT), position.y))
					pushCheckIfNew(collected, nextCheck, { static_cast<uint16_t>(position.x + SHIFT), position.y });

			}
		}
		bfs(info, data, collected, nextCheck, ++distance);
	}

	void BfsSearcher::search(SearchInfo& info, const Matrix<uint16_t>& data) {

		Positions empty;
		bfs(info, data, empty, { info.start });
	}
}