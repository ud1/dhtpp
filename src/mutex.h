#ifndef DHT_MUTEX_H
#define DHT_MUTEX_H

namespace dhtpp {

	class CVirtualMutex {
	public:
		CVirtualMutex() {
			times_unlock = 1;
		}

		void Lock() {
			// cannot lock really
			assert(times_unlock > 0);
			--times_unlock;
		}

		void Unlock() {
			++times_unlock;
		}

	protected:
		int times_unlock;
	};

	typedef CVirtualMutex CMutex;
}

#endif // DHT_MUTEX_H
