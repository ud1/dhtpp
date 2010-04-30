#ifndef DHT_TRANSPORT_H
#define DHT_TRANSPORT_H

#include "kad_rpc.h"

namespace dhtpp {

	class INode {
	public:
		virtual const NodeInfo &GetNodeInfo() const = 0;
		virtual void OnPingRequest(const PingRequest &req) = 0;
		virtual void OnStoreRequest(const StoreRequest &req) = 0;
		virtual void OnFindNodeRequest(const FindNodeRequest &req) = 0;
		virtual void OnFindValueRequest(const FindValueRequest &req) = 0;

		virtual void OnPingResponse(const PingResponse &resp) = 0;
		virtual void OnStoreResponse(const StoreResponse &resp) = 0;
		virtual void OnFindNodeResponse(const FindNodeResponse &resp) = 0;
		virtual void OnFindValueResponse(const FindValueResponse &resp) = 0;
	};

	class ITransport {
	public:
		virtual void SendPingRequest(const PingRequest &req) = 0;
		virtual void SendStoreRequest(const StoreRequest &req) = 0;
		virtual void SendFindNodeRequest(const FindNodeRequest &req) = 0;
		virtual void SendFindValueRequest(const FindValueRequest &req) = 0;

		virtual void SendPingResponse(const PingResponse &resp) = 0;
		virtual void SendStoreResponse(const StoreResponse &resp) = 0;
		virtual void SendFindNodeResponse(const FindNodeResponse &resp) = 0;
		virtual void SendFindValueResponse(const FindValueResponse &resp) = 0;
	};

}

#endif // DHT_TRANSPORT_H
