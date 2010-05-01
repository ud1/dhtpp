#ifndef DHT_KAD_NODE_H
#define DHT_KAD_NODE_H

#include "transport.h"
#include "routing_table.h"
#include "store.h"
#include "types.h"
#include "job_scheduler.h"

#include <set>
#include <vector>

#include <boost/function.hpp>
#include <boost/intrusive/set.hpp>

namespace dhtpp {

	class CKadNode : public INode {
	public:
		CKadNode(const NodeID &id, CJobScheduler *sched);

		const NodeInfo &GetNodeInfo() const {
			return my_info;
		}

		void Terminate();
		void SaveBootstrapContacts(std::vector<NodeAddress> &out) const;

		void OnPingRequest(const PingRequest &req);
		void OnStoreRequest(const StoreRequest &req);
		void OnFindNodeRequest(const FindNodeRequest &req);
		void OnFindValueRequest(const FindValueRequest &req);

		void OnPingResponse(const PingResponse &resp);
		void OnStoreResponse(const StoreResponse &resp);
		void OnFindNodeResponse(const FindNodeResponse &resp);
		void OnFindValueResponse(const FindValueResponse &resp);

		enum ErrorCode {
			SUCCEED,
			FAILED,
		};

		typedef boost::function<void (ErrorCode code, rpc_id id)> ping_callback;
		typedef boost::function<void (ErrorCode code, rpc_id id)> store_callback;
		typedef boost::function<void (ErrorCode code, const FindNodeResponse *resp)> find_node_callback;
		typedef boost::function<void (ErrorCode code, const FindValueResponse *resp)> find_value_callback;

		typedef boost::function<void (ErrorCode code)> join_callback;

		rpc_id Ping(const NodeAddress &to, const ping_callback &callback);
		rpc_id Store(const NodeID &key, const std::string &value, const store_callback &callback);
		rpc_id FindCloseNodes(const NodeID &id, const find_node_callback &callback);
		rpc_id FindValue(const NodeID &key, const find_value_callback &callback);

		void JoinNetwork(const std::vector<NodeAddress> &bootstrap_contacts, const join_callback &callback);

	protected:
		ITransport *transport;
		NodeInfo my_info;
		CRoutingTable routing_table;
		CStore store;
		CJobScheduler *scheduler;

		struct PingRequestData {
			PingRequestData() {
				attempts = 0;
			}
			PingRequest req;
			ping_callback callback;
			uint16 attempts;
			rpc_id GetId() const {
				return req.id;
			}
		};

		struct FindRequestData {
			rpc_id id;
			rpc_id GetId() const {
				return id;
			}

			NodeID target;

			enum FindType {
				FIND_NODE,
				FIND_VALUE,
			} type;

			find_node_callback find_node_callback_;
			find_value_callback find_value_callback_;

			struct CandidateLite {
				BigInt distance;
				bool operator < (const CandidateLite &o) const {
					return distance < o.distance;
				}
			};

			struct Candidate : 
				public CandidateLite,
				public NodeInfo,
				public boost::intrusive::set_base_hook<> 
			{
				Candidate(const NodeInfo &info, const NodeID &target) {
					*(NodeInfo *)this = info;
					distance = (const BigInt) id ^ (const BigInt) target;
					attempts = 0;
					type = UNKNOWN;
				}

				enum {
					UNKNOWN,
					PENDING,
					DOWN,
					UP,
				} type;

				uint16 attempts;

				using CandidateLite::operator <;
			};

			typedef boost::intrusive::set<Candidate> Candidates;
			Candidates candidates;

			Candidate *GetCandidate(const NodeID &id);
			void Update(const std::vector<NodeInfo> &nodes);
		};

		struct StoreRequestData {
			NodeID key;
			std::string value;
			rpc_id id;
			store_callback callback;
			uint16 succeded;

			StoreRequestData() {
				succeded = 0;
			}

			rpc_id GetId() const {
				return id;
			}

			struct StoreNode : public NodeInfo {
				StoreNode() {
					attempts = 0;
				}
				uint16 attempts;
			};

			std::set<StoreNode *> store_nodes;
		};

		template <typename ReqType>
		struct Comp {
			bool operator()(const ReqType *d1, const ReqType *d2) const {
				return d1->GetId() < d2->GetId();
			}
		};

		typedef std::set<PingRequestData *, Comp<PingRequestData> > PingRequests;
		typedef std::set<FindRequestData *, Comp<FindRequestData> > FindRequests;
		typedef std::set<StoreRequestData *, Comp<StoreRequestData> > StoreRequests;

		PingRequests ping_requests;
		FindRequests find_requests;
		StoreRequests store_requests;

		rpc_id ping_id_counter, store_id_counter, find_id_counter;

		void UpdateRoutingTable(const RPCRequest &req);
		void UpdateRoutingTable(const RPCResponse &resp);
		void PingRequestTimeout(rpc_id id);

		FindRequestData *CreateFindData(const NodeID &id, FindRequestData::FindType);
		void FindRequestTimeout(FindRequestData *data, FindRequestData::Candidate *cand);

		// return true if there is pending nodes
		bool SendFindRequestToOneNode(FindRequestData *data);

		void SendFindRequestToOneNode(FindRequestData *data, FindRequestData::Candidate *cand);
		void CallFindNodeCallback(FindRequestData *data);
		void FinishSearch(FindRequestData *data);
		FindRequestData *GetFindData(rpc_id id);

		void DoStore(StoreRequestData *data, ErrorCode code, const FindNodeResponse *resp);
		void StoreRequestTimeout(StoreRequestData *data, StoreRequestData::StoreNode *node);
		void FinishStore(StoreRequestData *data);

		// Bootstrap
		std::vector<NodeAddress> join_bootstrap_contacts;
		join_callback join_callback_;
		int join_pinging_nodesN, join_succeedN;
		void Join_PingCallback(ErrorCode code, rpc_id id);
		void Join_FindNodeCallback(bool try_again, ErrorCode code, const FindNodeResponse *resp);
		enum {
			NOT_JOINED,
			FIND_NODES_STARTED,
			JOINED,
		} join_state;
	};
}

#endif // DHT_KAD_NODE_H
