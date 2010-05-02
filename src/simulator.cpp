#include "simulator.h"

#include <stdlib.h>

#include <boost/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lexical_cast.hpp>

#include "../agner_random/stocc.h"

#include <sha.h>

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

	bool CTransport::AddNode(INode *node) {
		return nodes.insert(std::make_pair((NodeAddress)node->GetNodeInfo(), node)).second;
	}

	bool CTransport::RemoveNode(INode *node) {
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

	INode *CTransport::GetNode(const NodeAddress &addr) {
		Nodes::iterator it = nodes.find(addr);
		if (it == nodes.end())
			return NULL;
		return it->second;
	}


	CSimulator::CSimulator(int nodesN) {
		int seed = 0;
		random_lib = new StochasticLib2(seed);

		avg_on_time		= 10*60*1000;
		avg_off_time	= 10*60*1000;

		CryptoPP::SHA1 sha;

		NodeInfo info;
		info.ip = "supernode";
		info.port = 5555;
		sha.CalculateDigest(info.id.id, (const byte *)info.ip.c_str(), info.ip.size());
		supernode = new CKadNode(info, &scheduler);
		transport.AddNode(supernode);

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
		CKadNode *node = new CKadNode(nd->info, &scheduler);
		node->JoinNetwork(nd->bootstrap_contacts,
			boost::bind(&CSimulator::StartNodeLoop, this, node, boost::lambda::_1));
		scheduler.AddJob_(GenerateRandomOnTime(),
			boost::bind(&CSimulator::DeactivateNode, this, node), node);
		transport.AddNode(node);
		delete nd;
	}

	void CSimulator::DeactivateNode(CKadNode *node) {
		InactiveNode *nd = new InactiveNode;
		node->SaveBootstrapContacts(nd->bootstrap_contacts);
		nd->info = node->GetNodeInfo();
		scheduler.CancelJobsByOwner(node);
		scheduler.AddJob_(GenerateRandomOffTime(),
			boost::bind(&CSimulator::ActivateNode, this, nd), nd);
		transport.RemoveNode(node);
		delete node;
	}

	void CSimulator::StartNodeLoop(CKadNode *node, CKadNode::ErrorCode code) {}

	uint64 CSimulator::GenerateRandomOnTime() {
		return random_lib->Poisson(avg_on_time);
	}

	uint64 CSimulator::GenerateRandomOffTime() {
		return random_lib->Poisson(avg_off_time);
	}

}