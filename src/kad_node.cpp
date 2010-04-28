#include "kad_node.h"

#include <boost/bind.hpp>

namespace dhtpp {

	CKadNode::CKadNode(const NodeID &id, CJobScheduler *sched) : routing_table(id) {
		scheduler = sched;
		ping_id_counter = store_id_counter = find_id_counter = 0;
	}

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

	void CKadNode::OnPingResponse(const PingResponse &resp) {
		PingRequestData temp, *data;
		temp.req.id = resp.id;
		PingRequests::iterator it = ping_requests.find(&temp);
		if (it == ping_requests.end())
			return;
		data = *it;
		if (data->req.to != resp.from)
			return;
		scheduler->CancelJobsByOwner(data);
		data->callback(SUCCEED, resp);
		ping_requests.erase(it);
		delete data;
	}

	rpc_id CKadNode::Ping(const NodeAddress &to, const ping_callback &callback) {
		PingRequestData *data = new PingRequestData;
		data->req.Init(my_info, to, my_info.GetId(), ping_id_counter++);
		data->callback = callback;
		transport->SendPingRequest(data->req);
		ping_requests.insert(data);
		scheduler->AddJob_(timeout_period, boost::bind(&CKadNode::PingRequestTimeout, this, data->req.id), data);
		return data->req.id;
	}

	void CKadNode::PingRequestTimeout(rpc_id id) {
		PingRequestData temp, *data;
		temp.req.id = id;
		PingRequests::iterator it = ping_requests.find(&temp);
		if (it == ping_requests.end())
			return;
		data = *it;
		if (data->attempts++ < attempts_number) {
			transport->SendPingRequest(data->req);
			scheduler->AddJob_(timeout_period, boost::bind(&CKadNode::PingRequestTimeout, this, id), data);
		} else {
			ping_requests.erase(it);
			data->callback(TIMEOUT, PingResponse());
			delete data;
		}
	}

	void CKadNode::OnFindNodeResponse(const FindNodeResponse &resp) {
		// Get FindRequestData by rpc_id
		FindRequestData temp, *data;
		temp.id = resp.id;
		FindRequests::iterator it = find_requests.find(&temp);
		if (it == find_requests.end())
			return;
		data = *it;
		if (data->type != FindRequestData::FIND_NODE)
			return;

		// Get Candidate by NodeId
		FindRequestData::CandidateLite lite;
		lite.distance = (BigInt) resp.responder_id ^ (BigInt) data->target;
		FindRequestData::Candidates::iterator cit = data->candidates.find(lite, std::less<FindRequestData::CandidateLite>());
		if (cit == data->candidates.end())
			return;
		FindRequestData::Candidate *cand = &*cit;

		// Cancel timeout
		scheduler->CancelJobsByOwner(cand);

		cand->type = FindRequestData::Candidate::UP;
		
		// update contacts
		std::vector<NodeInfo>::const_iterator vit;
		for (vit = resp.nodes.begin(); vit != resp.nodes.end(); ++vit) {
			FindRequestData::Candidate *cand = new FindRequestData::Candidate(*vit, data->target);
			if (!data->candidates.insert(*cand).second) {
				// already exist
				delete cand;
			}
		}

		if (!SendFindRequestToOneNode(data))
			FinishSearch(data, &resp);
	}

	rpc_id CKadNode::FindCloseNodes(const NodeID &id, const find_node_callback &callback) {
		// Create request data
		FindRequestData *data = CreateFindData(id, FindRequestData::FIND_NODE);
		data->find_node_callback_ = callback;

		// Send requests to closest alpha nodes
		for (int i = 0; i < alpha; ++i) {
			SendFindRequestToOneNode(data);
		}
		return data->id;
	}

	CKadNode::FindRequestData *CKadNode::CreateFindData(const NodeID &id, FindRequestData::FindType type) {
		// Create request data
		FindRequestData *data = new FindRequestData;
		data->id = find_id_counter++;
		data->type = type;
		data->target = id;

		// Fill by our closest contacts
		std::vector<NodeInfo> closest_contacts;
		routing_table.GetClosestContacts(id, closest_contacts);
		assert(closest_contacts.size());
		std::vector<NodeInfo>::iterator it;
		for (it = closest_contacts.begin(); it != closest_contacts.end(); ++it) {
			FindRequestData::Candidate *cand = new FindRequestData::Candidate(*it, id);
			data->candidates.insert(*cand);
		}

		find_requests.insert(data);
		return data;
	}

	void CKadNode::FindRequestTimeout(FindRequestData *data, FindRequestData::Candidate *cand) {
		if (cand->attempts++ < attempts_number) {
			// try again
			SendFindRequestToOneNode(data, cand);
		} else {
			cand->type = FindRequestData::Candidate::DOWN;
		}
		if (!SendFindRequestToOneNode(data))
			FinishSearch(data, NULL);
	}

	bool CKadNode::SendFindRequestToOneNode(FindRequestData *data) {
		bool pending_nodes = false;
		FindRequestData::Candidates::iterator it;
		for (it = data->candidates.begin(); it != data->candidates.end(); ++it) {
			FindRequestData::Candidate *cand = &*it;
			if (cand->type == FindRequestData::Candidate::PENDING)
				pending_nodes = true;
			if (cand->type != FindRequestData::Candidate::UNKNOWN)
				continue;
			SendFindRequestToOneNode(data, cand);
			return true;
		}
		return pending_nodes;
	}

	void CKadNode::SendFindRequestToOneNode(FindRequestData *data, FindRequestData::Candidate *cand) {
		FindNodeRequest req;
		req.Init(my_info, *cand, my_info.GetId(), data->id);
		req.target = data->target;
		cand->type = FindRequestData::Candidate::PENDING;
		transport->SendFindNodeRequest(req);
		scheduler->AddJob_(timeout_period, boost::bind(&CKadNode::FindRequestTimeout, this, data, cand), cand);
	}

	void CKadNode::FinishSearch(FindRequestData *data, const FindNodeResponse *resp) {
		if (data->type == FindRequestData::FIND_NODE) {
			// do callback
			std::vector<NodeInfo> closest_contacts;
			int i = 0;
			FindRequestData::Candidates::iterator it;
			for (it = data->candidates.begin(); it != data->candidates.end() && i < K; ++it) {
				FindRequestData::Candidate *cand = &*it;
				if (cand->type == FindRequestData::Candidate::UP) {
					++i;
					closest_contacts.push_back(*(const NodeInfo *)cand);
				}
			}

			if (closest_contacts.size())
				data->find_node_callback_(SUCCEED, closest_contacts);
			else data->find_node_callback_(FAILED, closest_contacts);
		}

		// Clean up
		FindRequestData::Candidates::iterator it;
		for (it = data->candidates.begin(); it != data->candidates.end(); ) {
			FindRequestData::Candidate *cand = &*it;
			it = data->candidates.erase(it);
			delete cand;
		}

		find_requests.erase(data);
		delete data;
	}
}
