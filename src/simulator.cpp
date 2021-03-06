#include "simulator.h"
#include "types.h"

#include <stdlib.h>

#include <boost/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/variate_generator.hpp>

#include "../agner_random/stocc.h"

#include "../sha1/SHA1.h"

#include <iterator>

#if 0
#define IMPLEMENT_RPC_METHOD(cl, name)														\
	void cl::Send##name(const name &r) {													\
		name##_counter++;																	\
		if (r.to == r.from) {																\
			Do##name(r); /* loopback	*/											\
		}																					\
		bool is_not_lost = (float) rand() / RAND_MAX > packet_loss;							\
		if (is_not_lost) {																	\
			uint64 delay = network_delay + network_delay_delta * rand() / RAND_MAX;			\
			scheduler->AddJob_(delay, boost::bind(&cl::Do##name, this, r), this); \
		}																					\
	}																						\
	void cl::Do##name(name r) {															\
		INode *node = GetNode(r.to);														\
		if (node) {																			\
			node->On##name(r);																\
		}																					\
	}
#else

#define IMPLEMENT_RPC_METHOD(cl, name)														\
	void cl::Send##name(const name &r) {													\
		name##_counter++;																	\
		if (r.to == r.from) {																\
			Do##name(new name(r)); /* loopback	*/											\
		}																					\
		bool is_not_lost = (float) rand() / RAND_MAX >= packet_loss;						\
		if (is_not_lost) {																	\
			uint64 delay = network_delay + network_delay_delta * rand() / RAND_MAX;			\
			scheduler->AddJob_(delay, boost::bind(&cl::Do##name, this, new name(r)), this); \
		}																					\
	}																						\
	void cl::Do##name(name *r) {															\
		INode *node = GetNode(r->to);														\
		if (node) {																			\
			node->On##name(*r);																\
		}																					\
		delete r;																			\
	}

#endif

namespace dhtpp {
	const uint64 print_time_interval = 5000;
	const uint64 print_rpc_counts_interval = 1000;
	const uint64 flush_stats_interval = 60*1000;
	static boost::mt19937 gen;

	void CalculateDigest(uint8 *out, const uint8 *in, uint32 len) {
		CSHA1 sha;
		sha.Update(in, len);
		sha.Final();
		sha.GetHash(out);
	}

	CTransport::CTransport(CJobScheduler *sched) {
		scheduler = sched;
	}

	CTransport::~CTransport() {
		std::string filename = boost::lexical_cast<std::string>(time(NULL));
		filename += "store_dmp.txt";
		std::ofstream out(filename.c_str());
		if (!out)
			return;

		Nodes::iterator it = nodes.begin();
		for (; it != nodes.end(); ++it) {
			it->second->SaveStoreTo(out);
		}
	}

	bool CTransport::AddNode(CKadNode *node) {
		return nodes.insert(std::make_pair((NodeAddress)node->GetNodeInfo(), node)).second;
	}

	bool CTransport::RemoveNode(CKadNode *node) {
		return nodes.erase(node->GetNodeInfo()) > 0;
	}

	IMPLEMENT_RPC_METHOD(CTransport, PingRequest)
	IMPLEMENT_RPC_METHOD(CTransport, StoreRequest)
	IMPLEMENT_RPC_METHOD(CTransport, FindNodeRequest)
	IMPLEMENT_RPC_METHOD(CTransport, FindValueRequest)

	IMPLEMENT_RPC_METHOD(CTransport, PingResponse)
	IMPLEMENT_RPC_METHOD(CTransport, StoreResponse)
	IMPLEMENT_RPC_METHOD(CTransport, FindNodeResponse)
	IMPLEMENT_RPC_METHOD(CTransport, FindValueResponse)

#if DOWNLIST_OPTIMIZATION
	IMPLEMENT_RPC_METHOD(CTransport, DownlistRequest)
	IMPLEMENT_RPC_METHOD(CTransport, DownlistResponse)
#else
	// Empty
	void CTransport::SendDownlistRequest(const DownlistRequest &req) {}
	void CTransport::SendDownlistResponse(const DownlistResponse &resp) {}
#endif

	CKadNode *CTransport::GetNode(const NodeAddress &addr) {
		Nodes::iterator it = nodes.find(addr);
		if (it == nodes.end())
			return NULL;
		return it->second;
	}

	CKadNode *CTransport::GetRandomNode() {
		Nodes::size_type count = nodes.size();
		if (!count)
			return NULL;
		Nodes::size_type pos = rand() * (count - 1) / RAND_MAX;
		Nodes::iterator it = nodes.begin();
		std::advance(it, pos);
		return it->second;
	}

	CSimulator::CSimulator(int nodesN, CStats *st) {
		transport = new CTransport(&scheduler);
		stats = st;
		values_counter = values_total = nodesN * values_per_node;

		int seed = 0;
		random_lib = new StochasticLib2(seed);

		NodeInfo info;
		info.ip = -1;
		//info.port = 5555;
		std::string ip_str = std::string("node") + boost::lexical_cast<std::string>(info.ip);
		CalculateDigest(info.id.id, (const uint8 *)ip_str.c_str(), ip_str.size());
		supernode = new CKadNode(info, &scheduler, transport);
		active_nodes.insert(supernode);
		transport->AddNode(supernode);

#if 0
		int nodes_to_run = (int) (nodesN * avg_on_time / (avg_on_time + avg_off_time));
		for (int i = 0; i < nodes_to_run; ++i) {
			InactiveNode *nd = new InactiveNode;
			inactive_nodes.insert(nd);
			nd->bootstrap_contacts.push_back(supernode->GetNodeInfo());
			nd->info.ip = "node" + boost::lexical_cast<std::string>(i);
			nd->info.port = 5555;
			sha.CalculateDigest(nd->info.id.id, (const byte *)nd->info.ip.c_str(), nd->info.ip.size());
			ActivateNode(nd);
		}

		for (int i = nodes_to_run; i < nodesN; ++i) {
			InactiveNode *nd = new InactiveNode;
			inactive_nodes.insert(nd);
			nd->bootstrap_contacts.push_back(supernode->GetNodeInfo());
			nd->info.ip = "node" + boost::lexical_cast<std::string>(i);
			nd->info.port = 5555;
			sha.CalculateDigest(nd->info.id.id, (const byte *)nd->info.ip.c_str(), nd->info.ip.size());
			scheduler.AddJob_(GenerateRandomOffTime(),
				boost::bind(&CSimulator::ActivateNode, this, nd), nd);
		}

#else
		for (int i = 0; i < nodesN; ++i) {
			uint64 t = (avg_on_time + avg_off_time) * i / nodesN;
			InactiveNode *nd = new InactiveNode;
			//inactive_nodes.insert(nd);
			nd->bootstrap_contacts.push_back(supernode->GetNodeInfo());
			nd->info.ip = i;
			//nd->info.port = 5555;
			std::string ip_str = std::string("node") + boost::lexical_cast<std::string>(nd->info.ip);
			CalculateDigest(nd->info.id.id, (const uint8 *)ip_str.c_str(), ip_str.size());
			scheduler.AddJob_(t, boost::bind(&CSimulator::ActivateNode, this, nd), nd);
		}
#endif
	}

	CSimulator::~CSimulator() {
		delete transport;
		delete random_lib;

		std::set<CKadNode *>::iterator ait = active_nodes.begin();
		for (; ait != active_nodes.end(); ++ait) {
			delete *ait;
		}

		std::set<InactiveNode *>::iterator iit = inactive_nodes.begin();
		for (; iit != inactive_nodes.end(); ++iit) {
			delete *iit;
		}
	}

	void CSimulator::ActivateNode(InactiveNode *nd) {
		CKadNode *node = new CKadNode(nd->info, &scheduler, transport);
		if (!nd->bootstrap_contacts.size()) {
			nd->bootstrap_contacts.push_back(supernode->GetNodeInfo());
		}
		node->JoinNetwork(nd->bootstrap_contacts,
			boost::bind(&CSimulator::StartNodeLoop, this, node, boost::lambda::_1));
		scheduler.AddJob_(GenerateRandomOnTime(),
			boost::bind(&CSimulator::DeactivateNode, this, node), node);
		if (!transport->AddNode(node)) {
			printf("Error\n");
		}
		//active_nodes.insert(node);
		//inactive_nodes.erase(nd);
		delete nd;
	}

	void CSimulator::DeactivateNode(CKadNode *node) {
		InactiveNode *nd = new InactiveNode;
		node->SaveBootstrapContacts(nd->bootstrap_contacts);
		nd->info = node->GetNodeInfo();
		scheduler.CancelJobsByOwner(node);
		scheduler.AddJob_(GenerateRandomOffTime(),
			boost::bind(&CSimulator::ActivateNode, this, nd), nd);
		transport->RemoveNode(node);

		if (node->IsJoined()) {
			CStats::FindReqsCountHist find_node_hist;
			find_node_hist.t = GetTimerInstance()->GetCurrentTime();
			find_node_hist.count = node->GetFindNodeStats();
			if (find_node_hist.count.size()) {
				stats->InformAboutFindNodeReqCountHist(find_node_hist);
			};

			CStats::FindReqsCountHist find_value_hist;
			find_value_hist.t = GetTimerInstance()->GetCurrentTime();
			find_value_hist.count = node->GetFindValueStats();
			if (find_value_hist.count.size()) {
				stats->InformAboutFindValueReqCountHist(find_value_hist);
			};
		}

		stats->InformAboutStoreToFirstNodeCount(node->GetStoreToFirstNodeCount());

		//active_nodes.erase(node);
		//inactive_nodes.insert(nd);
		delete node;
	}

	void CSimulator::StartNodeLoop(CKadNode *node, CKadNode::ErrorCode code) {
		if (code == CKadNode::FAILED) {
			printf("node not joined\n");
			return;
		}

		int i = 0;
		for (;values_counter > 0 && i < values_per_node; ++i, --values_counter) {
			std::string value = "v" + boost::lexical_cast<std::string>(values_counter);
			NodeID key;
			CalculateDigest(key.id, (const uint8 *) value.c_str(), value.size());
			node->Store(key, value, expiration_time, 
				boost::bind(&CSimulator::StoreCallback, this,
				boost::lambda::_1,
				boost::lambda::_2,
				boost::lambda::_3));
		}

		scheduler.AddJob_(check_value_time_interval, 
			boost::bind(&CSimulator::CheckRandomValue, this, node), node);
	}

	uint64 CSimulator::GenerateRandomOnTime() {
		return (uint64) (avg_on_time - avg_on_time_delta + 2*((double)rand()/RAND_MAX*avg_on_time_delta));
	}

	uint64 CSimulator::GenerateRandomOffTime() {
		return (uint64) (avg_off_time - avg_off_time_delta + 2*((double)rand()/RAND_MAX*avg_off_time_delta));
	}

	void CSimulator::Run(uint64 period) {
		scheduler.AddJob_(period, boost::bind(&CJobScheduler::Stop, &scheduler), &scheduler);
		scheduler.AddJob_(print_time_interval, boost::bind(&CSimulator::PrintTime, this), this);
		scheduler.AddJob_(begin_stats, boost::bind(&CSimulator::CheckRandomNode, this), this);
		scheduler.AddJob_(0, boost::bind(&CSimulator::SaveRpcCounts, this), this);
		scheduler.AddJob_(0, boost::bind(&CSimulator::FlushStats, this), this);
		scheduler.Run();
	}

	void CSimulator::PrintTime() {
		scheduler.AddJob_(print_time_interval, boost::bind(&CSimulator::PrintTime, this), this);
		printf("time = %lld, jobs = %lld, done = %lld\n", 
			GetTimerInstance()->GetCurrentTime(),
			scheduler.GetJobsCount(), 
			scheduler.JobsDone());
	}

	void CSimulator::CheckRandomNode() {
		scheduler.AddJob_(check_node_time_interval, boost::bind(&CSimulator::CheckRandomNode, this), this);
		CKadNode *node = transport->GetRandomNode();
		if (!node)
			return;

		std::vector<NodeAddress> addrs;
		node->SaveBootstrapContacts(addrs);
		int active = 0;
		for (std::vector<NodeAddress>::size_type i = 0; i < addrs.size(); ++i) {
			if (transport->GetNode(addrs[i])) {
				++active;
			}
		}

		std::vector<NodeInfo> closest;
		node->GetLocalCloseNodes(node->GetNodeInfo().id, closest);
		int closest_active = 0;
		for (std::vector<NodeInfo>::size_type i = 0; i < closest.size(); ++i) {
			if (transport->GetNode(closest[i])) {
				++closest_active;
			}
		}
		printf("CheckRandomNode: %d/%d %d/%d\n", active, addrs.size(), closest_active, closest.size());
		CStats::NodeStateInfo info;
		info.routing_table_activeN = active;
		info.routing_tableN = addrs.size();
		info.closest_contacts_activeN = closest_active;
		info.closest_contactsN = closest.size();
		stats->InformAboutNode(info);
	}

	void CSimulator::SaveRpcCounts() {
		scheduler.AddJob_(print_rpc_counts_interval, boost::bind(&CSimulator::SaveRpcCounts, this), this);
		CStats::RpcCounts counts;
		counts.t = GetTimerInstance()->GetCurrentTime();
		counts.ping_reqs = transport->PingRequest_counter;
		counts.store_req = transport->StoreRequest_counter;
		counts.find_node_req = transport->FindNodeRequest_counter;
		counts.find_value_req = transport->FindValueRequest_counter;
		counts.downlist_req = transport->DownlistRequest_counter;
		counts.ping_resp = transport->PingResponse_counter;
		counts.store_resp = transport->StoreResponse_counter;
		counts.find_node_resp = transport->FindNodeResponse_counter;
		counts.find_value_resp = transport->FindValueResponse_counter;
		counts.downlist_resp = transport->DownlistResponse_counter;
		stats->InformAboutRpcCounts(counts);
	}

	void CSimulator::CheckRandomValue(CKadNode *node) {
		scheduler.AddJob_(check_value_time_interval, 
			boost::bind(&CSimulator::CheckRandomValue, this, node), node);

		if (values_counter > 0)
			return;

		boost::uniform_int<> dist(0, values_total-1);
		boost::variate_generator<boost::mt19937&, boost::uniform_int<> > rnd(gen, dist);
		std::string value = "v" + boost::lexical_cast<std::string>(rnd());
		NodeID key;
		CalculateDigest(key.id, (const uint8 *) value.c_str(), value.size());
		node->FindValue(key, boost::bind(&CSimulator::FindValueCallback, this, 
			GetTimerInstance()->GetCurrentTime(),
			boost::lambda::_1, boost::lambda::_2));
	}

	void CSimulator::FindValueCallback(uint64 start_time, CKadNode::ErrorCode code, const FindValueResponse *resp) {
		uint64 finish_time = GetTimerInstance()->GetCurrentTime();
		if (code == CKadNode::FAILED) {
			stats->InformAboutFailedFindValue(finish_time, finish_time - start_time);
			//printf("Find value failed\n");
		} else if (code == CKadNode::SUCCEED) {
			stats->InformAboutSucceedFindValue(finish_time, finish_time - start_time);
		}
	}

	void CSimulator::StoreCallback(CKadNode::ErrorCode code, rpc_id id, const NodeID *max_distance) {
		if (code == CKadNode::FAILED) {
			printf("Store Error\n");
		}
	}

	void CSimulator::FlushStats() {
		scheduler.AddJob_(flush_stats_interval, boost::bind(&CSimulator::FlushStats, this), this);
		stats->Flush();
	}
}