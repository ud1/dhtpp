#include "kad_node.h"

namespace dhtpp {

	void CKadNode::OnPingRequest(const PingRequest &req) {
		PingResponse resp;
		resp.Init(my_info, req.from, my_info.GetId());
		transport->SendPingResponse(resp);

		UpdateRoutingTable(req);
	}

	void CKadNode::OnStoreRequest(const StoreRequest &req) {
		store.StoreItem(req.key, req.value);

		StoreResponse resp;
		resp.Init(my_info, req.from, my_info.GetId());
		transport->SendStoreResponse(resp);

		UpdateRoutingTable(req);
	}

	void CKadNode::OnFindNodeRequest(const FindNodeRequest &req) {
		FindNodeResponse resp;
		resp.Init(my_info, req.from, my_info.GetId());
		routing_table.GetClosestContacts(req.target, resp.nodes);
		transport->SendFindNodeResponse(resp);

		UpdateRoutingTable(req);
	}

	void CKadNode::OnFindValueRequest(const FindValueRequest &req) {
		FindValueResponse resp;
		resp.Init(my_info, req.from, my_info.GetId());
		store.GetItems(req.key, resp.values);
		if (!resp.values.size()) {
			routing_table.GetClosestContacts(req.key, resp.nodes);
		}
		transport->SendFindValueResponse(resp);

		UpdateRoutingTable(req);
	}

	void CKadNode::UpdateRoutingTable(const RPCRequest &req) {
		
	}
}