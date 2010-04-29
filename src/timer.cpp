#include "timer.h"

namespace dhtpp {
	Ctimer *GetTimerInstance() {
		static Ctimer timer;
		return &timer;
	}
}

