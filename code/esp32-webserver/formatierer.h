#pragma once

namespace Local {
	class Formatierer {
	public:
		static String wert_als_k(bool force_prefix, int value) {
		  char str[10];
		  if(value > 999 || value < -999) {
			sprintf(str, (force_prefix ? "%+.3f" : "%.3f"), (float) value / 1000);
		  } else {
			sprintf(str, (force_prefix ? "%+d" : "%d"), value);
		  }
		  return (String) str;
		}

		static String wert_als_k_mit_1stellen(int value) {
		  char str[10];
		  sprintf(str, ("%.1f"), (float) value / 1000);
		  return (String) str;
		}
	};
}
