#pragma once
#include "../service/web_reader.h"
#include "../config.h"
#include <Regexp.h>

namespace Local::Api {
	class BaseAPI {

	protected:
		Local::Service::WebReader* web_reader;
		Local::Config* cfg;

	public:
		BaseAPI(Local::Config& cfg, Local::Service::WebReader& web_reader): cfg(&cfg), web_reader(&web_reader) {
		}
	};
}
