#include "simulator.h"
#include "types.h"

#include <stdlib.h>

#include <boost/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lexical_cast.hpp>

#include "../agner_random/stocc.h"

#include <sha.h>

#include <iterator>

#define IMPLEMENT_RPC_METHOD(cl, name)														\
	void cl::Send##name(const name &r) {													\
		if (r.to == r.from) {																\
			Do##name(new name(r)); /* loopback	*/											\
		}																					\
		bool is_not_lost = (float) rand() / RAND_MAX > packet_loss;							\
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


namespace dhtpp {
	const uint64 print_time_interval = 5000;

	CTransport::CTransport(CJobScheduler *sched) {
		scheduler = sched;
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
		stats->SetNodesN(nodesN);

		int seed = 0;
		random_lib = new StochasticLib2(seed);

		CryptoPP::SHA1 sha;

		NodeInfo info;
		info.ip = "supernode";
		info.port = 5555;
		sha.CalculateDigest(info.id.id, (const byte *)info.ip.c_str(), info.ip.size());
		supernode = new CKadNode(info, &scheduler, transport);
		transport->AddNode(supernode);

		int nodes_to_run = (int) (nodesN * avg_on_time / (avg_on_time + avg_off_time));
		for (int i = 0; i < nodes_to_run; ++i) {
			InactiveNode *nd = new InactiveNode;
			nd->bootstrap_contacts.push_back(supernode->GetNodeInfo());
			nd->info.ip = "node" + boost::lexical_cast<std::string>(i);
			nd->info.port = 5555;
			sha.CalculateDigest(nd->info.id.id, (const byte *)nd->info.ip.c_str(), nd->info.ip.size());
			ActivateNode(nd);
		}

		for (int i = nodes_to_run; i < nodesN; ++i) {
			InactiveNode *nd = new InactiveNode;
			nd->bootstrap_contacts.push_back(supernode->GetNodeInfo());
			nd->info.ip = "node" + boost::lexical_cast<std::string>(i);
			nd->info.port = 5555;
			sha.CalculateDigest(nd->info.id.id, (const byte *)nd->info.ip.c_str(), nd->info.ip.size());
			scheduler.AddJob_(GenerateRandomOffTime(),
				boost::bind(&CSimulator::ActivateNode, this, nd), nd);
		}
	}

	void CSimulator::ActivateNode(InactiveNode *nd) {
		CKadNode *node = new CKadNode(nd->info, &scheduler, transport);
		node->JoinNetwork(nd->bootstrap_contacts,
			boost::bind(&CSimulator::StartNodeLoop, this, node, boost::lambda::_1));
		scheduler.AddJob_(GenerateRandomOnTime(),
			boost::bind(&CSimulator::DeactivateNode, this, node), node);
		transport->AddNode(node);
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
		delete node;
	}

	void CSimulator::StartNodeLoop(CKadNode *node, CKadNode::ErrorCode code) {
		if (code == CKadNode::FAILED) {
			printf("node not joined\n");
			return;
		}
	}

	uint64 CSimulator::GenerateRandomOnTime() {
		return avg_on_time - avg_on_time_delta + 2*((double)rand()/RAND_MAX*avg_on_time_delta);
	}

	uint64 CSimulator::GenerateRandomOffTime() {
		return avg_off_time - avg_off_time_delta + 2*((double)rand()/RAND_MAX*avg_off_time_delta);
	}

	void CSimulator::Run(uint64 period) {
		scheduler.AddJob_(period, boost::bind(&CJobScheduler::Stop, &scheduler), &scheduler);
		scheduler.AddJob_(print_time_interval, boost::bind(&CSimulator::PrintTime, this), this);
		scheduler.AddJob_(begin_stats, boost::bind(&CSimulator::CheckRandomNode, this), this);
		scheduler.Run();
	}

	void CSimulator::PrintTime() {
		scheduler.AddJob_(print_time_interval, boost::bind(&CSimulator::PrintTime, this), this);
		printf("time = %lld\n", GetTimerInstance()->GetCurrentTime());
	}

	void CSimulator::CheckRandomNode() {
		scheduler.AddJob_(check_node_time_interval, boost::bind(&CSimulator::CheckRandomNode, this), this);
		CKadNode *node = transport->GetRandomNode();
		if (!node)
			return;

		std::vector<NodeAddress> addrs;
		node->SaveBootstrapContacts(addrs);
		int active = 0;
		for (int i = 0; i < addrs.size(); ++i) {
			if (transport->GetNode(addrs[i])) {
				++active;
			}
		}

		std::vector<NodeInfo> closest;
		node->GetLocalCloseNodes(node->GetNodeInfo().id, closest);
		int closest_active = 0;
		for (int i = 0; i < closest.size(); ++i) {
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
}