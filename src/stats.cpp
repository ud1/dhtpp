#include "stats.h"
#include "config.h"

#include <fstream>

namespace dhtpp {

	CStats::~CStats() {
		for (std::vector<NodeStateInfo *>::size_type i = 0; i < nodes_info.size(); ++i) {
			delete nodes_info[i];
		}
		for (std::vector<RpcCounts *>::size_type i = 0; i < rpc_counts.size(); ++i) {
			delete rpc_counts[i];
		}
	}

	bool CStats::Save(const std::string &filename) {
		std::ofstream out(filename.c_str());
		if (!out)
			return false;

		out << "nodes_number;" << nodesN << "\n";
		out << "NODE_ID_LENGTH_BYTES;" << NODE_ID_LENGTH_BYTES << "\n";
		out << "K;" << K << "\n";
		out << "alpha;" << alpha << "\n";
		out << "timeout_period;" << timeout_period << "\n";
		out << "attempts_number;" << attempts_number << "\n";
		out << "republish_time;" << republish_time << "\n";
		out << "republish_time_delta;" << republish_time_delta << "\n";
		out << "expiration_time;" << expiration_time << "\n";
		out << "avg_on_time;" << avg_on_time << "\n";
		out << "avg_on_time_delta;" << avg_on_time_delta << "\n";
		out << "avg_off_time;" << avg_off_time << "\n";
		out << "avg_off_time_delta;" << avg_off_time_delta << "\n";
		out << "begin_stats;" << begin_stats << "\n";
		out << "check_node_time_interval;" << check_node_time_interval << "\n";

		out << "network_delay;" << network_delay << "\n";
		out << "network_delay_delta;" << network_delay_delta << "\n";
		out << "packet_loss;" << packet_loss << "\n";
		out << "FORCE_K_OPTIMIZATION;" << FORCE_K_OPTIMIZATION << "\n";
		out << "DOWNLIST_OPTIMIZATION;" << DOWNLIST_OPTIMIZATION << "\n";

		for (std::vector<NodeStateInfo *>::size_type i = 0; i < nodes_info.size(); ++i) {
			NodeStateInfo &info = *nodes_info[i];
			out << "ni;" << info.closest_contactsN << ";"
				<< info.closest_contacts_activeN << ";"
				<< info.routing_tableN << ";"
				<< info.routing_table_activeN << "\n";
		}
		for (std::vector<RpcCounts *>::size_type i = 0; i < rpc_counts.size(); ++i) {
			RpcCounts &counts = *rpc_counts[i];
			out << "rpcs;" 
				<< counts.t << ";"
				<< counts.ping_reqs << ";"
				<< counts.store_req << ";"
				<< counts.find_node_req << ";"
				<< counts.find_value_req << ";"
				<< counts.downlist_req << ";"
				<< counts.ping_resp << ";"
				<< counts.store_resp << ";"
				<< counts.find_node_resp << ";"
				<< counts.find_value_resp << ";"
				<< counts.downlist_resp << ";"
				<< "\n";
		}

		return true;
	}

	void CStats::InformAboutNode(NodeStateInfo *info) {
		nodes_info.push_back(info);
	}

	void CStats::InformAboutRpcCounts(RpcCounts *counts) {
		rpc_counts.push_back(counts);
	}
}