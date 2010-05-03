#ifndef DHT_SIMULATOR_H
#define DHT_SIMULATOR_H

#include "transport.h"
#include "job_scheduler.h"
#include "kad_node.h"

#include <map>
#include <vector>

class StochasticLib2;

#define DECLARE_RPC_METHOD(name) \
	public: \
	void Send##name(const name &r); \
	private: \
	void Do##name(name *r);

namespace dhtpp {

	const uint64 network_delay = 50;
	const uint64 network_delay_delta = 50;
	const float packet_loss = 0.1f;

	class CTransport : public ITransport {
	public:
		CTransport(CJobScheduler *scheduler);
		bool AddNode(CKadNode *node);
		bool RemoveNode(CKadNode *node);

		DECLARE_RPC_METHOD(PingRequest)
		DECLARE_RPC_METHOD(StoreRequest)
		DECLARE_RPC_METHOD(FindNodeRequest)
		DECLARE_RPC_METHOD(FindValueRequest)

		DECLARE_RPC_METHOD(PingResponse)
		DECLARE_RPC_METHOD(StoreResponse)
		DECLARE_RPC_METHOD(FindNodeResponse)
		DECLARE_RPC_METHOD(FindValueResponse)

	public:
		CKadNode *GetRandomNode();
		CKadNode *GetNode(const NodeAddress &addr);

	private:
		typedef std::map<NodeAddress, CKadNode *> Nodes;
		Nodes nodes;

		CJobScheduler *scheduler;

	};

	class CSimulator {
	public:
		CSimulator(int nodesN);
		void Run(uint64 period);

	protected:
		CTransport *transport;
		CJobScheduler scheduler;
		CKadNode *supernode; // is always running
		StochasticLib2 *random_lib;
		double avg_on_time, avg_off_time;
		double avg_on_time_delta, avg_off_time_delta;

		struct InactiveNode {
			NodeInfo info;
			std::vector<NodeAddress> bootstrap_contacts;
		};

		void ActivateNode(InactiveNode *nd);
		void DeactivateNode(CKadNode *node);
		void StartNodeLoop(CKadNode *node, CKadNode::ErrorCode code);
		uint64 GenerateRandomOnTime();
		uint64 GenerateRandomOffTime();

		void PrintTime();
		void CheckRandomNode();
	};
}

#endif // DHT_SIMULATOR_H
