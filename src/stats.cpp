#include "stats.h"
#include "config.h"


namespace dhtpp {

	CStats::CStats() {
		store_to_first_node_count = 0;
	}

	CStats::~CStats() {
		out << "store_to_first_node_count;" << store_to_first_node_count << "\n";
	}

	bool CStats::Open(const std::string &filename) {
		out.open(filename.c_str());
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
		out << "check_value_time_interval;" << check_value_time_interval << "\n";
		out << "values_per_node;" << values_per_node << "\n";
		out << "min_rt_check_time_interval;" << min_rt_check_time_interval << "\n";
		out << "republish_treshhold;" << republish_treshhold << "\n";

		out << "network_delay;" << network_delay << "\n";
		out << "network_delay_delta;" << network_delay_delta << "\n";
		out << "packet_loss;" << packet_loss << "\n";
		out << "FORCE_K_OPTIMIZATION;" << FORCE_K_OPTIMIZATION << "\n";
		out << "DOWNLIST_OPTIMIZATION;" << DOWNLIST_OPTIMIZATION << "\n";

		out << "rt_b;" << rt_b << "\n";
		out << "rt_r;" << rt_r << "\n";

		return true;
	}

	void CStats::InformAboutNode(const NodeStateInfo &info) {
		out << "node_info;" << info.closest_contactsN << ";"
			<< info.closest_contacts_activeN << ";"
			<< info.routing_tableN << ";"
			<< info.routing_table_activeN << "\n";
	}

	void CStats::InformAboutRpcCounts(const RpcCounts &counts) {
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

	void CStats::InformAboutFindNodeReqCountHist(const FindReqsCountHist &counts) {
		out << "find_node_hist;"
			<< counts.t << ";";
		std::map<int, int>::const_iterator it = counts.count.begin();
		for (;it != counts.count.end(); ++it) {
			out << it->first << "|" << it->second << ";";
		}
		out << "\n";
	}

	void CStats::InformAboutFindValueReqCountHist(const FindReqsCountHist &counts) {
		out << "find_value_hist;"
			<< counts.t << ";";
		std::map<int, int>::const_iterator it = counts.count.begin();
		for (;it != counts.count.end(); ++it) {
			out << it->first << "|" << it->second << ";";
		}
		out << "\n";
	}

	void CStats::InformAboutFailedFindValue(uint64 t, uint64 duration) {
		out << "failed_find_value;" << t << ";" << duration << "\n";
	}

	void CStats::InformAboutSucceedFindValue(uint64 t, uint64 duration) {
		out << "succeed_find_value;" << t << ";" << duration << "\n";
	}

	void CStats::InformAboutStoreToFirstNodeCount(uint64 count) {
		store_to_first_node_count += count;
	}
}