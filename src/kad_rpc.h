#ifndef DHT_KAD_RPC_H
#define DHT_KAD_RPC_H

#include "contact.h"

#include <vector>
#include <string>

namespace dhtpp {

	struct RPCMessage {
		NodeAddress from, to;
	};

	struct RPCRequest : public RPCMessage {
		NodeID sender_id;
	};

	struct RPCResponse : public RPCMessage {
		NodeID responder_id;
		void Init(const NodeAddress &from_, const NodeAddress &to_, const NodeID &responder_id_) {
			from = from_;
			to = to_;
			responder_id = responder_id_;
		}
	};

	struct PingRequest : public RPCRequest {};
	struct PingResponse : public RPCResponse {};

	struct StoreRequest : public RPCRequest {
		NodeID key;
		std::string value;
	};

	struct StoreResponse : public RPCResponse {};

	struct FindNodeRequest : public RPCRequest {
		NodeID target;
	};
	struct FindNodeResponse : public RPCResponse {
		std::vector<NodeInfo> nodes;
	};

	struct FindValueRequest : public RPCRequest {
		NodeID key;
	};
	struct FindValueResponse : public RPCResponse {
		std::vector<NodeInfo> nodes;
		std::vector<std::string> values; // may be more than one value
	};

}

#endif // DHT_KAD_RPC_H
