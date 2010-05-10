#ifndef DHT_STORE_H
#define DHT_STORE_H

#include "node_id.h"
#include "job_scheduler.h"
#include "kad_node.h"

#include <map>
#include <string>

namespace dhtpp {

	class CStore {
	public:
		CStore(CKadNode *node, CJobScheduler *scheduler);
		~CStore();
		void StoreItem(const NodeID &key, const std::string &value, uint64 time_to_live);
		void GetItems(const NodeID &key, std::vector<std::string> &out_values);
		void OnNewContact(const NodeInfo &contact, bool is_close_to_holder);
		void OnRemoveContact(const NodeID &contact, bool is_close_to_holder);

	private:
		struct Item {
			Item() {
				max_distance_setted = false;
			}

			uint64 expiration_time;
			std::string value;
			bool max_distance_setted;
			NodeID max_distance;
		};
		typedef std::multimap<NodeID, Item *> Store;
		Store store;
		CKadNode *node;
		CJobScheduler *scheduler;
		int removed_contacts;

		void RepublishItem(NodeID key, Item *item);
		void DeleteItem(NodeID key, Item *item);
		uint64 GetRandomRepublishTime();
		uint64 GetRandomRepublishTimeDelta();
		void StoreCallback(Item *item, CKadNode::ErrorCode code, rpc_id id, const NodeID *max_distance);
	};
}

#endif // DHT_STORE_H
