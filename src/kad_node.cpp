#include "kad_node.h"
#include "timer.h"

#include <boost/bind.hpp>
#include <boost/lambda/lambda.hpp>

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
		Contact contact;
		contact.id = req.sender_id;
		(NodeAddress) contact = req.from;
		contact.last_seen = GetTimerInstance()->GetCurrentTime();
		routing_table.AddContact(contact);
	}

	void CKadNode::UpdateRoutingTable(const RPCResponse &resp) {
		Contact contact;
		contact.id = resp.responder_id;
		(NodeAddress) contact = resp.from;
		contact.last_seen = GetTimerInstance()->GetCurrentTime();
		routing_table.AddContact(contact);
	}

	void CKadNode::OnPingResponse(const PingResponse &resp) {
		UpdateRoutingTable(resp);
		PingRequestData temp, *data;
		temp.req.id = resp.id;
		PingRequests::iterator it = ping_requests.find(&temp);
		if (it == ping_requests.end())
			return;
		data = *it;
		if (data->req.to != resp.from)
			return;
		scheduler->CancelJobsByOwner(data);
		data->callback(SUCCEED, resp.id);
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
			data->callback(FAILED, id);
			delete data;
		}
	}

	CKadNode::FindRequestData *CKadNode::GetFindData(rpc_id id) {
		// Get FindRequestData by rpc_id
		FindRequestData temp;
		temp.id = id;
		FindRequests::iterator it = find_requests.find(&temp);
		if (it == find_requests.end())
			return NULL;
		return *it;
	}

	CKadNode::FindRequestData::Candidate *CKadNode::FindRequestData::GetCandidate(const NodeID &id) {
		// Get Candidate by NodeId
		FindRequestData::CandidateLite lite;
		lite.distance = (BigInt) id ^ (BigInt) target;
		FindRequestData::Candidates::iterator cit = candidates.find(lite, std::less<FindRequestData::CandidateLite>());
		if (cit == candidates.end())
			return NULL;
		return &*cit;
	}

	void CKadNode::FindRequestData::Update(const std::vector<NodeInfo> &nodes) {
		// update contacts
		std::vector<NodeInfo>::const_iterator vit;
		for (vit = nodes.begin(); vit != nodes.end(); ++vit) {
			Candidate *cand = new Candidate(*vit, target);
			if (!candidates.insert(*cand).second) {
				// already exist
				delete cand;
			}
		}
	}

	void CKadNode::OnFindNodeResponse(const FindNodeResponse &resp) {
		UpdateRoutingTable(resp);
		// Get FindRequestData by rpc_id
		FindRequestData *data = GetFindData(resp.id);
		if (!data || data->type != FindRequestData::FIND_NODE)
			return;

		// Get Candidate by NodeId
		FindRequestData::Candidate *cand = data->GetCandidate(resp.responder_id);
		if (!cand)
			return;

		// Cancel timeout
		scheduler->CancelJobsByOwner(cand);

		cand->type = FindRequestData::Candidate::UP;
		
		// update contacts
		data->Update(resp.nodes);

		if (!SendFindRequestToOneNode(data)) {
			CallFindNodeCallback(data);
			FinishSearch(data);
		}
	}

	void CKadNode::OnFindValueResponse(const FindValueResponse &resp) {
		UpdateRoutingTable(resp);
		// Get FindRequestData by rpc_id
		FindRequestData *data = GetFindData(resp.id);
		if (!data || data->type != FindRequestData::FIND_VALUE)
			return;

		// Get Candidate by NodeId
		FindRequestData::Candidate *cand = data->GetCandidate(resp.responder_id);
		if (!cand)
			return;

		// Cancel timeout
		scheduler->CancelJobsByOwner(cand);

		cand->type = FindRequestData::Candidate::UP;

		if (resp.values.size()) {
			data->find_value_callback_(SUCCEED, &resp);
			FinishSearch(data);
			return;
		}

		// update contacts
		data->Update(resp.nodes);

		if (!SendFindRequestToOneNode(data)) {
			data->find_value_callback_(FAILED, NULL);
			FinishSearch(data);
		}
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

	rpc_id CKadNode::FindValue(const NodeID &key, const find_value_callback &callback) {
		// Create request data
		FindRequestData *data = CreateFindData(key, FindRequestData::FIND_VALUE);
		data->find_value_callback_ = callback;

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
		if (!SendFindRequestToOneNode(data)) {
			if (data->type == FindRequestData::FIND_NODE)
				CallFindNodeCallback(data);
			else data->find_value_callback_(FAILED, NULL);
			FinishSearch(data);
		}
	}

	bool CKadNode::SendFindRequestToOneNode(FindRequestData *data) {
		bool pending_nodes = false;
		FindRequestData::Candidates::iterator it;
		int up_count = 0;
		for (it = data->candidates.begin(); it != data->candidates.end() && up_count < K; ++it) {
			FindRequestData::Candidate *cand = &*it;
			if (cand->type == FindRequestData::Candidate::PENDING)
				pending_nodes = true;
			if (cand->type == FindRequestData::Candidate::UP)
				++up_count;
			if (cand->type != FindRequestData::Candidate::UNKNOWN)
				continue;
			SendFindRequestToOneNode(data, cand);
			return true;
		}
		return pending_nodes;
	}

	void CKadNode::SendFindRequestToOneNode(FindRequestData *data, FindRequestData::Candidate *cand) {
		if (data->type == FindRequestData::FIND_NODE) {
			FindNodeRequest req;
			req.Init(my_info, *cand, my_info.GetId(), data->id);
			req.target = data->target;
			cand->type = FindRequestData::Candidate::PENDING;
			transport->SendFindNodeRequest(req);
			scheduler->AddJob_(timeout_period, boost::bind(&CKadNode::FindRequestTimeout, this, data, cand), cand);
		} else {
			FindValueRequest req;
			req.Init(my_info, *cand, my_info.GetId(), data->id);
			req.key = data->target;
			cand->type = FindRequestData::Candidate::PENDING;
			transport->SendFindValueRequest(req);
			scheduler->AddJob_(timeout_period, boost::bind(&CKadNode::FindRequestTimeout, this, data, cand), cand);
		}
	}

	void CKadNode::CallFindNodeCallback(FindRequestData *data) {
		assert(data->type == FindRequestData::FIND_NODE);
		// do callback
		FindNodeResponse closest_contacts;
		closest_contacts.id = data->id;
		int i = 0;
		FindRequestData::Candidates::iterator it;
		for (it = data->candidates.begin(); it != data->candidates.end() && i < K; ++it) {
			FindRequestData::Candidate *cand = &*it;
			if (cand->type == FindRequestData::Candidate::UP) {
				++i;
				closest_contacts.nodes.push_back(*(const NodeInfo *)cand);
			}
		}

		if (closest_contacts.nodes.size())
			data->find_node_callback_(SUCCEED, &closest_contacts);
		else data->find_node_callback_(FAILED, NULL);		
	}

	void CKadNode::FinishSearch(FindRequestData *data) {
		// Clean up
		FindRequestData::Candidates::iterator it;
		for (it = data->candidates.begin(); it != data->candidates.end(); ) {
			FindRequestData::Candidate *cand = &*it;
			it = data->candidates.erase(it);

			if (cand->type == FindRequestData::Candidate::PENDING) {
				scheduler->CancelJobsByOwner(cand);
			}

			delete cand;
		}

		find_requests.erase(data);
		delete data;
	}

	rpc_id CKadNode::Store(const NodeID &key, const std::string &value, const store_callback &callback) {
		StoreRequestData *data = new StoreRequestData;
		data->key = key;
		data->value = value;
		data->callback = callback;
		data->id = FindCloseNodes(key, boost::bind(&CKadNode::DoStore, this, data, boost::lambda::_1, boost::lambda::_2));
		return data->id;
	}

	void CKadNode::DoStore(StoreRequestData *data, ErrorCode code, const FindNodeResponse *resp) {
		if (code == FAILED) {
			data->callback(FAILED, data->id);
			delete data;
			return;
		}

		store_requests.insert(data);

		// Send Store requests
		StoreRequest req;
		req.key = data->key;
		req.value = data->value;

		std::vector<NodeInfo>::const_iterator it;
		for (it = resp->nodes.begin(); it != resp->nodes.end(); ++it) {
			StoreRequestData::StoreNode *node = new StoreRequestData::StoreNode;
			*(NodeInfo *)node = *it;
			data->store_nodes.insert(node);
			req.Init(my_info, *(NodeAddress *)node, my_info.GetId(), data->id);
			transport->SendStoreRequest(req);
			scheduler->AddJob_(timeout_period, boost::bind(&CKadNode::StoreRequestTimeout, this, data, node), node);
		}
	}

	void CKadNode::StoreRequestTimeout(StoreRequestData *data, StoreRequestData::StoreNode *node) {
		if (node->attempts++ < attempts_number) {
			// Repeat request
			StoreRequest req;
			req.key = data->key;
			req.value = data->value;
			req.Init(my_info, *(NodeAddress *)node, my_info.GetId(), data->id);
			transport->SendStoreRequest(req);
			scheduler->AddJob_(timeout_period, boost::bind(&CKadNode::StoreRequestTimeout, this, data, node), node);
		} else {
			data->store_nodes.erase(node);
			delete node;
			if (!data->store_nodes.size())
				FinishStore(data);
		}
	}

	void CKadNode::FinishStore(StoreRequestData *data) {
		if (data->succeded > 0)
			data->callback(SUCCEED, data->id);
		else data->callback(FAILED, data->id);

		store_requests.erase(data);

		delete data;
	}

	void CKadNode::OnStoreResponse(const StoreResponse &resp) {
		StoreRequestData temp, *data;
		temp.id = resp.id;
		StoreRequests::iterator it = store_requests.find(&temp);
		if (it == store_requests.end())
			return;
		data = *it;

		std::set<StoreRequestData::StoreNode *>::iterator sit;
		for (sit = data->store_nodes.begin(); sit != data->store_nodes.end(); ++sit) {
			StoreRequestData::StoreNode *node = *sit;
			if (node->id == resp.responder_id) {
				scheduler->CancelJobsByOwner(node);
				data->store_nodes.erase(sit);
				delete node;
				break;
			}
		}
		if (!data->store_nodes.size())
			FinishStore(data);
	}

}
