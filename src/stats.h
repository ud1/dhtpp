#ifndef DHT_STATS_H
#define DHT_STATS_H

#include "types.h"

#include <string>
#include <vector>
#include <map>
#include <fstream>

namespace dhtpp {

	class CStats {
	public:
		CStats();
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

		struct FindReqsCountHist {
			uint64 t;
			// histogram
			std::map<int, int> count;
		};

		bool Open(const std::string &filename);
		void SetNodesN(int nodes) {
			nodesN = nodes;
		}
		void Flush() {
			out.flush();
		}
		void InformAboutNode(const NodeStateInfo &info);
		void InformAboutRpcCounts(const RpcCounts &counts);

		void InformAboutFindNodeReqCountHist(const FindReqsCountHist &hist);
		void InformAboutFindValueReqCountHist(const FindReqsCountHist &hist);
		void InformAboutFailedFindValue(uint64 t, uint64 duration);
		void InformAboutSucceedFindValue(uint64 t, uint64 duration);
		void InformAboutStoreToFirstNodeCount(uint64 count);

	private:
		int nodesN;
		uint64 store_to_first_node_count;
		std::ofstream out;
	};
}

#endif // DHT_STATS_H
