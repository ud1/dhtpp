#include <boost/bind.hpp>

#include "job_scheduler.h"
#include "timer.h"

namespace dhtpp {
	CJobScheduler::CJobScheduler() : semaphore(0 /* initial count */) {
		isRunning = true;
		jobs_done = 0;
	}

	CJobScheduler::~CJobScheduler() {
		isRunning = false;
	}

	void CJobScheduler::AddJobAt(uint64 t, const Job &f, const void *owner) {
		mutex.Lock();
		std::pair<Jobs::index<time_tag>::type::iterator, bool> it = jobs.get<time_tag>().insert(JobEntry(t, f, owner));
		if (it.first == jobs.get<time_tag>().begin()) {
			semaphore.Post();
		}
		mutex.Unlock();
	}

	void CJobScheduler::AddJob_(uint64 milliseconds, const Job &f, const void *owner) {
		AddJobAt(GetTimerInstance()->GetCurrentTime() + milliseconds, f, owner);
	}

	void CJobScheduler::CancelJobsByOwner(const void *owner) {
		_CrtCheckMemory();
		mutex.Lock();
		jobs.get<owner_tag>().erase(owner);
		mutex.Unlock();
	}

	void CJobScheduler::Run() {
		isRunning = true;
		while (isRunning) {
			mutex.Lock();
			bool isEmpty = jobs.empty();

			uint64 t;
			if (!isEmpty) {
				t = jobs.get<time_tag>().begin()->t;
			}
			mutex.Unlock();

			if (isEmpty) {
				semaphore.Wait();
			} else {
				if (t <= GetTimerInstance()->GetCurrentTime()) {
					Job job;
					{
						mutex.Lock();
						if (!jobs.empty()) {
							job = jobs.get<time_tag>().begin()->job;
							jobs.get<time_tag>().erase(jobs.get<time_tag>().begin());
						}
						mutex.Unlock();
					}
					if (job) {
						job(); // do job
						++jobs_done;
					}
				} else {
					semaphore.Wait(t - GetTimerInstance()->GetCurrentTime());
				}
			}
		}
	}

	void CJobScheduler::Stop() {
		isRunning = false;
	}
}


