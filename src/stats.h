#ifndef DHT_STATS_H
#define DHT_STATS_H

#include "types.h"

#include <string>
#include <vector>

namespace dhtpp {

	class CStats {
	public:
		~CStats();
		struct NodeStateInfo {
			int routing_tableN;
			int routing_table_activeN;
			int closest_contactsN;
			int closest_contacts_activeN;
		};

		struct RpcCounts {
			uint64 t;
			uint64 ping_reqs, store_req, find_node_req, find_value_req, downlist_req;
			uint64 ping_resp, store_resp, find_node_resp, find_value_resp, downlist_resp;
		};

		bool Save(const std::string &filename);
		void SetNodesN(int nodes) {
			nodesN = nodes;
		}
		void InformAboutNode(NodeStateInfo *info);
		void InformAboutRpcCounts(RpcCounts *counts);

	private:
		int nodesN;
		std::vector<NodeStateInfo *> nodes_info;
		std::vector<RpcCounts *> rpc_counts;
	};
}

#endif // DHT_STATS_H
