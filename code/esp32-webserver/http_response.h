#pragma once

namespace Local {
	class HTTPResponse {
		public:
			int return_code;
			String content_type;
			String content;
			HTTPResponse(int return_code, String content_type, String content): return_code(return_code), content_type(content_type), content(content) {
		}
	};
}
