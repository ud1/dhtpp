#ifndef DHT_JOB_SCHEDULER_H
#define DHT_JOB_SCHEDULER_H

#include <boost/function.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>

#include "semaphore.h"
#include "mutex.h"

namespace dhtpp {
	class CJobScheduler {
	public:
		typedef boost::function<void (void)> Job;
		CJobScheduler();
		~CJobScheduler();

		void AddJobAt(uint64 t, const Job &f, const void *owner);
		void AddJob_(uint64 milliseconds, const Job &f, const void *owner);
		void CancelJobsByOwner(const void *owner);
		void Run();
		void Stop();
		uint64 GetJobsCount() const {
			return jobs.get<time_tag>().size();
		}

		uint64 JobsDone() const {
			return jobs_done;
		}

	protected:
		CSemaphore semaphore;
		CMutex mutex;

		volatile bool isRunning;
		struct JobEntry {
			JobEntry(uint64 t_, const Job &f_, const void *owner_) 
				: t(t_), job(f_), owner (owner_) {}
			JobEntry(const JobEntry &o) {
				t = o.t;
				job = o.job;
				owner = o.owner;
			}
			JobEntry &operator = (const JobEntry &o) {
				t = o.t;
				job = o.job;
				owner = o.owner;
				return *this;
			}
			uint64 t;
			Job job;
			const void *owner;
		};
		struct time_tag{};
		struct owner_tag{};
		typedef boost::multi_index::multi_index_container<
			JobEntry,
			boost::multi_index::indexed_by<
				// sort by time
				boost::multi_index::ordered_non_unique<boost::multi_index::tag<time_tag>,
					boost::multi_index::member<JobEntry, uint64, &JobEntry::t> >,
				// sort by owner
				boost::multi_index::ordered_non_unique<boost::multi_index::tag<owner_tag>,
					boost::multi_index::member<JobEntry, const void *, &JobEntry::owner> >
			>
		> Jobs;
		Jobs jobs; // guarded by mutex

		uint64 jobs_done;
	};
}



#endif // DHT_JOB_SCHEDULER_H
