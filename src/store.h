#ifndef DHT_STORE_H
#define DHT_STORE_H

#include "node_id.h"
#include "job_scheduler.h"
#include "kad_node.h"

#include <fstream>
#include <map>
#include <string>
#include <boost/shared_ptr.hpp>

namespace dhtpp {

	class CStore {
	public:
		CStore(CKadNode *node, CJobScheduler *scheduler);
		~CStore();
		void StoreItem(const NodeID &key, const std::string &value, uint64 time_to_live);
		void GetItems(const NodeID &key, std::vector<std::string> &out_values);
		void OnNewContact(const NodeInfo &contact, bool is_close_to_holder);
		void OnRemoveContact(const NodeID &contact, bool is_close_to_holder);
		void SaveStoreTo(std::ofstream &f) const;

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
		typedef boost::shared_ptr<Item> PItem;
		typedef std::multimap<NodeID, PItem> Store;
		Store store;
		CKadNode *node;
		CJobScheduler *scheduler;
		int removed_contacts;

		void RepublishItem(NodeID key, PItem item);
		void DeleteItem(NodeID key, PItem item);
		uint64 GetRandomRepublishTime();
		uint64 GetRandomRepublishTimeDelta();
		void StoreCallback(PItem item, CKadNode::ErrorCode code, rpc_id id, const NodeID *max_distance);

		uint64 random_rep_time_delta_cached;
		uint64 random_rep_time_delta_time;
	};
}

#endif // DHT_STORE_H
