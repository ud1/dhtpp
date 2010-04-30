#include "simulator.h"

#include <stdlib.h>

#include <boost/bind.hpp>

#define IMPLEMENT_RPC_METHOD(cl, name)														\
	void cl::Send##name(const name &r) {													\
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

}