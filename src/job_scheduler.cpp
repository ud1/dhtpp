#include <boost/bind.hpp>

#include "job_scheduler.h"
#include "timer.h"

using namespace dhtpp;

CJobScheduler::CJobScheduler() : semaphore(0 /* initial count */) {
	isRunning = true;
}

CJobScheduler::~CJobScheduler() {
	isRunning = false;
}

void CJobScheduler::AddJob_(uint64 t, const Job &f, const void *owner) {
	mutex.Lock();
	std::pair<Jobs::index<time_tag>::type::iterator, bool> it = jobs.get<time_tag>().insert(JobEntry(t, f, owner));
	if (it.first == jobs.get<time_tag>().begin()) {
		semaphore.Post();
	}
	mutex.Unlock();
}

void CJobScheduler::AddJob_(int milliseconds, const Job &f, const void *owner) {
	if (milliseconds > 0) {
		AddJob_(GetTimerInstance()->GetCurrentTime() + milliseconds, f, owner);
	} else {
		AddJob_(0, f, owner);
	}
}

void CJobScheduler::CancelJobsByOwner(const void *owner) {
	mutex.Lock();
	jobs.get<owner_tag>().erase(owner);
	mutex.Unlock();
}

void CJobScheduler::Run() {
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
				if (job)
					job(); // do job
			} else {
				semaphore.Wait(t - GetTimerInstance()->GetCurrentTime());
			}
		}
	}
}
