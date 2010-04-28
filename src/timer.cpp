#include "timer.h"

using namespace dhtpp;

Ctimer *GetTimerInstance() {
	static Ctimer timer;
	return &timer;
}
