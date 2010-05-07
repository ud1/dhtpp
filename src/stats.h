#ifndef DHT_STATS_H
#define DHT_STATS_H

#include <string>
#include <vector>

namespace dhtpp {

	class CStats {
	public:
		struct NodeStateInfo {
			int routing_tableN;
			int routing_table_activeN;
			int closest_contactsN;
			int closest_contacts_activeN;

			NodeStateInfo(){}
			NodeStateInfo(const NodeStateInfo &o) {
				*this = o;
			}
			NodeStateInfo &operator = (const NodeStateInfo &o) {
				routing_tableN = o.routing_tableN;
				routing_table_activeN = o.routing_table_activeN;
				closest_contactsN = o.closest_contactsN;
				closest_contacts_activeN = o.closest_contacts_activeN;
				return *this;
			}
		};

		bool Save(const std::string &filename);
		void SetNodesN(int nodes) {
			nodesN = nodes;
		}
		void InformAboutNode(const NodeStateInfo &info);

	private:
		int nodesN;
		std::vector<NodeStateInfo> nodes_info;
	};
}

#endif // DHT_STATS_H
