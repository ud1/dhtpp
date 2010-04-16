#ifndef DHT_KAD_NODE_H
#define DHT_KAD_NODE_H

#include "transport.h"
#include "routing_table.h"
#include "store.h"

namespace dhtpp {

	class CKadNode : public INode {
	public:
		void OnPingRequest(const PingRequest &req);
		void OnStoreRequest(const StoreRequest &req);
		void OnFindNodeRequest(const FindNodeRequest &req);
		void OnFindValueRequest(const FindValueRequest &req);

		void OnPingResponse(const PingResponse &resp);
		void OnStoreResponse(const StoreResponse &resp);
		void OnFindNodeResponse(const FindNodeResponse &resp);
		void OnFindValueResponse(const FindValueResponse &resp);

	protected:
		void UpdateRoutingTable(const RPCRequest &req);
		ITransport *transport;
		NodeInfo my_info;
		CRoutingTable routing_table;
		CStore store;
	};
}

#endif // DHT_KAD_NODE_H
