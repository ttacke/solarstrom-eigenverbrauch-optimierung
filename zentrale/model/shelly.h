#pragma once

namespace Local::Model {
	class Shelly {
	public:
		int timestamp = 0;
		bool ison = false;
		int power = 0;
	};
}
