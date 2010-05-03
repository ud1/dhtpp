#ifndef DHT_KAD_RPC_H
#define DHT_KAD_RPC_H

#include "contact.h"
#include "types.h"

#include <vector>
#include <string>

namespace dhtpp {

	struct RPCMessage {
		NodeAddress from, to;
		rpc_id id;
		RPCMessage &operator = (const RPCMessage &o) {
			from = o.from;
			to = o.to;
			id = o.id;
			return *this;
		}
	};

	struct RPCRequest : public RPCMessage {
		NodeID sender_id;
		void Init(const NodeAddress &from_, const NodeAddress &to_, const NodeID &sender_id_, rpc_id id_) {
			from = from_;
			to = to_;
			sender_id = sender_id_;
			id = id_;
		}

		RPCRequest(){}
		RPCRequest(const RPCRequest &o) {
			*this = o;
		}

		RPCRequest &operator = (const RPCRequest &o) {
			*(RPCMessage *)this = o;
			sender_id = o.sender_id;
			return *this;
		}
	};

	struct RPCResponse : public RPCMessage {
		NodeID responder_id;
		void Init(const NodeAddress &from_, const NodeAddress &to_, const NodeID &responder_id_, rpc_id rpc_id_) {
			from = from_;
			to = to_;
			responder_id = responder_id_;
			id = rpc_id_;
		}

		RPCResponse(){}
		RPCResponse(const RPCResponse &o) {
			*this = o;
		}

		RPCResponse &operator = (const RPCResponse &o) {
			*(RPCMessage *)this = o;
			responder_id = o.responder_id;
			return *this;
		}
	};

	typedef RPCRequest PingRequest;
	typedef RPCResponse PingResponse;

	struct StoreRequest : public RPCRequest {
		uint64 time_to_live;
		NodeID key;
		std::string value;

		StoreRequest(){}
		StoreRequest(const StoreRequest &o) {
			*this = o;
		}

		StoreRequest &operator = (const StoreRequest &o) {
			*(RPCRequest *)this = o;
			key = o.key;
			value = o.value;
			time_to_live = o.time_to_live;
			return *this;
		}
	};

	typedef RPCResponse StoreResponse;

	struct FindNodeRequest : public RPCRequest {
		NodeID target;

		FindNodeRequest(){}
		FindNodeRequest(const FindNodeRequest &o) {
			*this = o;
		}

		FindNodeRequest &operator = (const FindNodeRequest &o) {
			*(RPCRequest *)this = o;
			target = o.target;
			return *this;
		}
	};
	struct FindNodeResponse : public RPCResponse {
		std::vector<NodeInfo> nodes;

		FindNodeResponse(){}
		FindNodeResponse(const FindNodeResponse &o) {
			*this = o;
		}

		FindNodeResponse &operator = (const FindNodeResponse &o) {
			*(RPCResponse *)this = o;
			nodes = o.nodes;
			return *this;
		}
	};

	struct FindValueRequest : public RPCRequest {
		NodeID key;

		FindValueRequest(){}
		FindValueRequest(const FindValueRequest &o) {
			*this = o;
		}

		FindValueRequest &operator = (const FindValueRequest &o) {
			*(RPCRequest *)this = o;
			key = o.key;
			return *this;
		}
	};

	struct FindValueResponse : public RPCResponse {
		std::vector<NodeInfo> nodes;
		std::vector<std::string> values; // may be more than one value

		FindValueResponse(){}
		FindValueResponse(const FindValueResponse &o) {
			*this = o;
		}

		FindValueResponse &operator = (const FindValueResponse &o) {
			*(RPCResponse *)this = o;
			nodes = o.nodes;
			values = o.values;
			return *this;
		}
	};

}

#endif // DHT_KAD_RPC_H
