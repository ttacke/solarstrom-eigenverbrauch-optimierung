#pragma once

namespace Local::Service {
	class Web {
	protected:
		Local::Webserver& webserver;

	public:
		Web(
			Local::Webserver& webserver
		): webserver(webserver) {
		}
	};
}
