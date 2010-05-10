#ifndef DHT_SIMULATOR_H
#define DHT_SIMULATOR_H

#include "transport.h"
#include "job_scheduler.h"
#include "kad_node.h"
#include "stats.h"

#include <map>
#include <set>
#include <vector>

class StochasticLib2;

#if 0

#define DECLARE_RPC_METHOD(name) \
	public: \
	void Send##name(const name &r); \
	RPC_Counter name##_counter; \
	private: \
	void Do##name(name r);

#else

#define DECLARE_RPC_METHOD(name) \
	public: \
	void Send##name(const name &r); \
	RPC_Counter name##_counter; \
	private: \
	void Do##name(name *r);

#endif

namespace dhtpp {

	class CTransport : public ITransport {
	public:
		CTransport(CJobScheduler *scheduler);
		bool AddNode(CKadNode *node);
		bool RemoveNode(CKadNode *node);

		struct RPC_Counter {
			uint64 count;
			RPC_Counter() {
				count = 0;
			}
			operator uint64&() {
				return count;
			}
		};

		DECLARE_RPC_METHOD(PingRequest)
		DECLARE_RPC_METHOD(StoreRequest)
		DECLARE_RPC_METHOD(FindNodeRequest)
		DECLARE_RPC_METHOD(FindValueRequest)
		DECLARE_RPC_METHOD(DownlistRequest)

		DECLARE_RPC_METHOD(PingResponse)
		DECLARE_RPC_METHOD(StoreResponse)
		DECLARE_RPC_METHOD(FindNodeResponse)
		DECLARE_RPC_METHOD(FindValueResponse)
		DECLARE_RPC_METHOD(DownlistResponse)

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
		CSimulator(int nodesN, CStats *stats);
		~CSimulator();
		void Run(uint64 period);

	protected:
		CTransport *transport;
		CJobScheduler scheduler;
		CKadNode *supernode; // is always running
		StochasticLib2 *random_lib;
		CStats *stats;
		int values_counter, values_total;

		struct InactiveNode {
			NodeInfo info;
			std::vector<NodeAddress> bootstrap_contacts;
		};

		std::set<CKadNode *> active_nodes;
		std::set<InactiveNode *> inactive_nodes;

		void ActivateNode(InactiveNode *nd);
		void DeactivateNode(CKadNode *node);
		void StartNodeLoop(CKadNode *node, CKadNode::ErrorCode code);
		uint64 GenerateRandomOnTime();
		uint64 GenerateRandomOffTime();

		void PrintTime();
		void CheckRandomNode();
		void SaveRpcCounts();
		void CheckRandomValue(CKadNode *node);
		void FindValueCallback(NodeID *key, CKadNode::ErrorCode code, const FindValueResponse *resp);
		void StoreCallback(CKadNode::ErrorCode code, rpc_id id, const NodeID *max_distance);
	};
}

#endif // DHT_SIMULATOR_H
