#ifndef DHT_STORE_H
#define DHT_STORE_H

#include "node_id.h"
#include "job_scheduler.h"

#include <map>
#include <string>

namespace dhtpp {

	class CKadNode;

	class CStore {
	public:
		void StoreItem(const NodeID &key, const std::string &value, uint64 time_to_live);
		void GetItems(const NodeID &key, std::vector<std::string> &out_values);

	private:
		struct Item {
			uint64 expiration_time;
			std::string value;
		};
		typedef std::multimap<NodeID, Item *> Store;
		Store store;
		CKadNode *node;
		CJobScheduler *scheduler;

		void RepublishItem(NodeID key, Item *item);
		void DeleteItem(NodeID key, Item *item);
		uint64 GetRandomRepublishTime();
	};
}

#endif // DHT_STORE_H
