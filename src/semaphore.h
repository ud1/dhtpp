#ifndef DHT_SEMAPHORE_H
#define DHT_SEMAPHORE_H

#include "timer.h"

namespace dhtpp {
	class CVirtualSemaphore {
	public:
		CVirtualSemaphore(int count_) {
			count = count_;
		}

		bool Wait() {
			// this semaphore cannot wait really
			//assert(count > 0);
			--count;
			return true;
		}

		bool Wait(int milliseconds) {
			// move virtual time forward
			GetTimerInstance()->AddTimeInterval(milliseconds);
			return Wait();
		}

		void Post() {
			++count;
		}

	protected:
		int count;
	};

	typedef CVirtualSemaphore CSemaphore;
}

#endif // DHT_SEMAPHORE_H
