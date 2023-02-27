#pragma once
#include "web_client.h"
#include "config.h"
#include <Regexp.h>

namespace Local {
	class BaseAPI {

	protected:
		Local::WebClient* web_client;
		Local::Config* cfg;

	public:
		BaseAPI(Local::Config& cfg, Local::WebClient& web_client): cfg(&cfg), web_client(&web_client) {
		}
	};
}
