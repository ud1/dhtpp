#ifndef DHT_TIMER_H
#define DHT_TIMER_H

#include "types.h"

namespace dhtpp {
	class CVirtualTimer {
	public:
		CVirtualTimer() {
			cur_time = 0;
		}

		uint64 GetCurrentTime() const {
			return cur_time;
		}

		void AddTimeInterval(uint64 delta) {
			cur_time += delta;
		}

	private:
		uint64 cur_time;
	};

	typedef CVirtualTimer Ctimer;

	Ctimer *GetTimerInstance();
}

#endif // DHT_TIMER_H
