#include "kad_node.h"
#include "timer.h"
#include "store.h"

#include <boost/bind.hpp>
#include <boost/lambda/lambda.hpp>

namespace dhtpp {

	CKadNode::CKadNode(const NodeInfo &info, CJobScheduler *sched, ITransport *tr) : routing_table(info.id) {
		scheduler = sched;
		transport = tr;
		my_info = info;
		ping_id_counter = store_id_counter = find_id_counter = downlist_id_counter = 0;
		join_pinging_nodesN = 0;
		join_succeedN = 0;
		join_state = NOT_JOINED;
		store = new CStore(this, sched);
	}

	CKadNode::~CKadNode() {
		Terminate();
		delete store;
	}

	void CKadNode::OnPingRequest(const PingRequest &req) {
		PingResponse resp;
		resp.Init(my_info, req.from, my_info.GetId(), req.id);
		transport->SendPingResponse(resp);

		UpdateRoutingTable(req);
	}

	void CKadNode::OnStoreRequest(const StoreRequest &req) {
		store->StoreItem(req.key, req.value, req.time_to_live);

		StoreResponse resp;
		resp.Init(my_info, req.from, my_info.GetId(), req.id);
		transport->SendStoreResponse(resp);

		UpdateRoutingTable(req);
	}

	void CKadNode::OnFindNodeRequest(const FindNodeRequest &req) {
		FindNodeResponse resp;
		resp.Init(my_info, req.from, my_info.GetId(), req.id);
		routing_table.GetClosestContacts(req.target, resp.nodes);
		transport->SendFindNodeResponse(resp);

		UpdateRoutingTable(req);
	}

	void CKadNode::OnFindValueRequest(const FindValueRequest &req) {
		FindValueResponse resp;
		resp.Init(my_info, req.from, my_info.GetId(), req.id);
		store->GetItems(req.key, resp.values);
		if (!resp.values.size()) {
			routing_table.GetClosestContacts(req.key, resp.nodes);
		}
		transport->SendFindValueResponse(resp);

		UpdateRoutingTable(req);
	}

	void CKadNode::OnDownlistRequest(const DownlistRequest &req) {
		std::vector<NodeID>::const_iterator it;
		for (it = req.down_nodes.begin(); it != req.down_nodes.end(); ++it) {
			const NodeID &id = *it;
			Contact info;
			if (routing_table.GetContact(id, info)) {
				Ping(info, boost::bind(&CKadNode::DoRemoveContact, this,
					id, boost::lambda::_1, boost::lambda::_2));
			}
		}

		DownlistResponse resp;
		resp.Init(my_info, req.from, my_info.GetId(), req.id);
		transport->SendDownlistResponse(resp);

		UpdateRoutingTable(req);
	}

	void CKadNode::UpdateRoutingTable(const RPCRequest &req) {
		NodeInfo *contact = new NodeInfo;
		contact->id = req.sender_id;
		*(NodeAddress *) contact = req.from;
		UpdateRoutingTable(contact);
	}

	void CKadNode::UpdateRoutingTable(const RPCResponse &resp) {
		NodeInfo *contact = new NodeInfo;
		contact->id = resp.responder_id;
		*(NodeAddress *) contact = resp.from;
		UpdateRoutingTable(contact);
	}

	void CKadNode::UpdateRoutingTable(NodeInfo *contact) {
		bool is_close_to_holder;
		RoutingTableErrorCode err = routing_table.AddContact(*contact, is_close_to_holder);
		if (err == SUCCEED) {
			store->OnNewContact(*contact, is_close_to_holder);
		} else if (err == FULL) {
			// Check last seen contact
			Contact *last_seen_contact = new Contact;
			if (routing_table.LastSeenContact(contact->id, *last_seen_contact) 
				&& (last_seen_contact->last_seen + min_rt_check_time_interval < GetTimerInstance()->GetCurrentTime()) 
				&& !last_seen_contacts.count(last_seen_contact->id)) 
			{
				last_seen_contacts.insert(last_seen_contact->id);
				Ping(*last_seen_contact, boost::bind(&CKadNode::DoAddContact, this,
					contact, last_seen_contact, 
					boost::lambda::_1, boost::lambda::_2));
				return;
			}
			delete last_seen_contact;
		}
		delete contact;
	}

	void CKadNode::DoAddContact(NodeInfo *new_contact, Contact *last_seen_contact, ErrorCode code, rpc_id id) {
		if (code == FAILED) {
			// last_seen_contact is down
			bool is_close_to_holder;
			if (routing_table.RemoveContact(last_seen_contact->id, is_close_to_holder)) {
				store->OnRemoveContact(last_seen_contact->id, is_close_to_holder);
			}
			if (routing_table.AddContact(*new_contact, is_close_to_holder) == SUCCEED) {
				store->OnNewContact(*new_contact, is_close_to_holder);
			}
		}
		last_seen_contacts.erase(last_seen_contact->id);
		delete new_contact;
		delete last_seen_contact;
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

	void CKadNode::OnDownlistResponse(const DownlistResponse &resp) {
		UpdateRoutingTable(resp);
		DownlistRequestData temp, *data;
		temp.id = resp.id;
		DownlistRequests::iterator it = downlist_requests.find(&temp);
		if (it == downlist_requests.end())
			return;
		data = *it;

		std::set<DownlistRequestData::RequestedNode *>::iterator rit = data->req_nodes.begin();
		for (;rit != data->req_nodes.end(); ++rit) {
			DownlistRequestData::RequestedNode *node = *rit;
			if ((NodeAddress &)*node == resp.from) {
				scheduler->CancelJobsByOwner(node);
				data->req_nodes.erase(rit);
				delete node;
				break;
			}
		}
		if (!data->req_nodes.size())
			FinishDownlistRequests(data);
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

		if (cand->type == FindRequestData::Candidate::PENDING)
			data->pending_nodes--;

		cand->type = FindRequestData::Candidate::UP;
		
		// update contacts
		data->Update(resp.nodes);

		while (data->pending_nodes < alpha && SendFindRequestToOneNode(data));

		if (!data->pending_nodes) {
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

		if (cand->type == FindRequestData::Candidate::PENDING)
			data->pending_nodes--;

		cand->type = FindRequestData::Candidate::UP;

		if (resp.values.size()) {
			data->find_value_callback_(SUCCEED, &resp);
			FinishSearch(data);
			return;
		}

		// update contacts
		data->Update(resp.nodes);

		while (data->pending_nodes < alpha && SendFindRequestToOneNode(data));

		if (!data->pending_nodes) {
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
		// Check our store
		FindValueResponse resp;
		store->GetItems(key, resp.values);
		if (resp.values.size()) {
			resp.id = find_id_counter++;
			callback(SUCCEED, &resp);
			return resp.id;
		}

		// Start searching process
		// Create request data
		FindRequestData *data = CreateFindData(key, FindRequestData::FIND_VALUE);
		data->find_value_callback_ = callback;

		// Send requests to closest alpha nodes
		for (int i = 0; i < alpha; ++i) {
			SendFindRequestToOneNode(data);
		}
		return data->id;
	}

	void CKadNode::GetLocalCloseNodes(const NodeID &id, std::vector<NodeInfo> &out) {
		routing_table.GetClosestContacts(id, out);
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
			// this node can be requested again
			cand->type = FindRequestData::Candidate::UNKNOWN;
		} else {
			cand->type = FindRequestData::Candidate::DOWN;
		}

		data->pending_nodes--;

		while (data->pending_nodes < alpha && SendFindRequestToOneNode(data));

		if (!data->pending_nodes) {
			if (data->type == FindRequestData::FIND_NODE)
				CallFindNodeCallback(data);
			else data->find_value_callback_(FAILED, NULL);
			FinishSearch(data);
		}
	}

	bool CKadNode::SendFindRequestToOneNode(FindRequestData *data) {
		FindRequestData::Candidates::iterator it;
		int up_count = 0;
		for (it = data->candidates.begin(); it != data->candidates.end() && up_count < K; ++it) {
			FindRequestData::Candidate *cand = &*it;
			if (cand->type == FindRequestData::Candidate::UP)
				++up_count;
			if (cand->type != FindRequestData::Candidate::UNKNOWN)
				continue;
			if (*cand == my_info)
				continue;
			SendFindRequestToOneNode(data, cand);
			data->pending_nodes++;
			return true;
		}
		return false;
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
		data->requests_total++;
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
		DownlistRequestData *ddata = new DownlistRequestData;
		// Clean up
		FindRequestData::Candidates::iterator it;
		for (it = data->candidates.begin(); it != data->candidates.end(); ) {
			FindRequestData::Candidate *cand = &*it;
			it = data->candidates.erase(it);

			switch (cand->type) {
				case FindRequestData::Candidate::PENDING:
					{
						scheduler->CancelJobsByOwner(cand);
						break;
					}
				case FindRequestData::Candidate::UP:
					{
						DownlistRequestData::RequestedNode *nd = new DownlistRequestData::RequestedNode;
						*(NodeInfo *)nd = *(NodeInfo *)cand;
						ddata->req_nodes.insert(nd);
						break;
					}
				case FindRequestData::Candidate::DOWN:
					{
						ddata->down_nodes.push_back(cand->id);
						break;
					}
			}

			delete cand;
		}

		if (data->type == FindRequestData::FIND_NODE) {
			find_node_reqs_count[data->requests_total]++;
		} else {
			find_value_reqs_count[data->requests_total]++;
		}

		find_requests.erase(data);
		delete data;

		DoDownlistRequests(ddata);
	}

	rpc_id CKadNode::Store(const NodeID &key, const std::string &value, uint64 time_to_live, const store_callback &callback) {
		StoreRequestData *data = new StoreRequestData;
		data->key = key;
		data->value = value;
		data->callback = callback;
		data->time_to_live = time_to_live;
		data->id = store_id_counter++;
		FindCloseNodes(key, boost::bind(&CKadNode::DoStore, this, data, false, boost::lambda::_1, boost::lambda::_2));
		return data->id;
	}

	rpc_id CKadNode::StoreToNode(const NodeInfo &to_node, const NodeID &key, const std::string &value, uint64 time_to_live, const store_callback &callback) {
		StoreRequestData *data = new StoreRequestData;
		data->key = key;
		data->value = value;
		data->callback = callback;
		data->time_to_live = time_to_live;
		data->id = store_id_counter++;
		FindNodeResponse resp;
		resp.nodes.push_back(to_node);
		DoStore(data, true, SUCCEED, &resp);
		return data->id;
	}

	void CKadNode::DoStore(StoreRequestData *data, bool single, ErrorCode code, const FindNodeResponse *resp) {
		if (code != SUCCEED) {
			data->callback(code, data->id, NULL);
			delete data;
			return;
		}

		store_requests.insert(data);

		// Send Store requests
		StoreRequest req;
		req.key = data->key;
		req.value = data->value;
		req.time_to_live = data->time_to_live;

		std::vector<NodeInfo>::const_iterator it;
		for (it = resp->nodes.begin(); it != resp->nodes.end(); ++it) {
			if (*it == my_info) // do not send store to yourself
				continue;
			StoreRequestData::StoreNode *node = new StoreRequestData::StoreNode;
			*(NodeInfo *)node = *it;
			data->store_nodes.insert(node);
			req.Init(my_info, *(NodeAddress *)node, my_info.GetId(), data->id);
			transport->SendStoreRequest(req);
			scheduler->AddJob_(timeout_period, boost::bind(&CKadNode::StoreRequestTimeout, this, data, node), node);
		}

		if (!single && resp->nodes.size() < K) {
			data->max_distance = (BigInt) MaxNodeID();
		} else {
			data->max_distance = (BigInt) resp->nodes[resp->nodes.size()-1].id ^ (BigInt) data->key;
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
			data->callback(SUCCEED, data->id, &data->max_distance);
		else data->callback(FAILED, data->id, NULL);

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
				data->succeded++;
				delete node;
				break;
			}
		}
		if (!data->store_nodes.size())
			FinishStore(data);
	}

	void CKadNode::SaveBootstrapContacts(std::vector<NodeAddress> &out) const {
		routing_table.SaveBootstrapContacts(out);
	}

	void CKadNode::JoinNetwork(const std::vector<NodeAddress> &bootstrap_contacts, const join_callback &callback) {
		std::copy(bootstrap_contacts.begin(), bootstrap_contacts.end(), std::back_inserter(join_bootstrap_contacts));
		join_callback_ = callback;
		for ( ;join_pinging_nodesN < alpha && join_bootstrap_contacts.size(); ++join_pinging_nodesN) {
			Ping(bootstrap_contacts[join_bootstrap_contacts.size()-1], 
				boost::bind(&CKadNode::Join_PingCallback, this, boost::lambda::_1, boost::lambda::_2));
			join_bootstrap_contacts.pop_back();
		}
	}

	void CKadNode::Join_PingCallback(ErrorCode code, rpc_id id) {
		--join_pinging_nodesN;
		if (code == SUCCEED)
			++join_succeedN;

		if (join_state == NOT_JOINED && (join_succeedN >= K || !join_bootstrap_contacts.size())) {
			if (!join_succeedN) {
				join_callback_(FAILED);
				return;
			}
			join_state = FIND_NODES_STARTED;
			FindCloseNodes(my_info.GetId(), 
				boost::bind(&CKadNode::Join_FindNodeCallback, this, 
				join_bootstrap_contacts.size() > 0, boost::lambda::_1, boost::lambda::_2));
		}

		for ( ;join_pinging_nodesN < alpha && join_bootstrap_contacts.size(); ++join_pinging_nodesN) {
			Ping(join_bootstrap_contacts[join_bootstrap_contacts.size()-1], 
				boost::bind(&CKadNode::Join_PingCallback, this, boost::lambda::_1, boost::lambda::_2));
			join_bootstrap_contacts.pop_back();
		}
	}

	void CKadNode::Join_FindNodeCallback(bool try_again, ErrorCode code, const FindNodeResponse *resp) {
		if (code == SUCCEED) {
			join_state = JOINED;
			join_callback_(SUCCEED);
		} else {
			join_state = NOT_JOINED;
			if (try_again) {
				join_state = FIND_NODES_STARTED;
				FindCloseNodes(my_info.GetId(), 
					boost::bind(&CKadNode::Join_FindNodeCallback, this, 
					join_bootstrap_contacts.size() > 0, boost::lambda::_1, boost::lambda::_2));
			} else {
				join_callback_(FAILED);
			}
		}
	}

	void CKadNode::DoDownlistRequests(DownlistRequestData *data) {
		// Remove down nodes from our routing table
		for (std::vector<NodeID>::size_type i = 0; i < data->down_nodes.size(); ++i) {
			bool is_close_to_holder;
			if (routing_table.RemoveContact(data->down_nodes[i], is_close_to_holder)) {
				store->OnRemoveContact(data->down_nodes[i], is_close_to_holder);
			}
		}
		if (!data->down_nodes.size() || !data->req_nodes.size()) {
			FinishDownlistRequests(data);
			return;
		}
		std::set<DownlistRequestData::RequestedNode *>::iterator it = data->req_nodes.begin();
		DownlistRequest req;
		std::copy(data->down_nodes.begin(), data->down_nodes.end(), std::back_inserter(req.down_nodes));
		data->id = downlist_id_counter++;
		downlist_requests.insert(data);
		for (; it != data->req_nodes.end(); ++it) {
			DownlistRequestData::RequestedNode *node = *it;
			req.Init(my_info, *node, my_info.GetId(), data->id);
			transport->SendDownlistRequest(req);
			scheduler->AddJob_(timeout_period, boost::bind(&CKadNode::DownlistRequestTimeout, this, data, node), node);
		}
	}

	void CKadNode::DoRemoveContact(NodeID node_id, ErrorCode code, rpc_id id) {
		if (code == FAILED) {
			// Node is not responding on pings
			bool is_close_to_holder;
			if (routing_table.RemoveContact(node_id, is_close_to_holder)) {
				store->OnRemoveContact(node_id, is_close_to_holder);
			}
		}
	}

	void CKadNode::DownlistRequestTimeout(DownlistRequestData *data, DownlistRequestData::RequestedNode *node) {
		if (node->attempts++ < attempts_number) {
			DownlistRequest req;
			std::copy(data->down_nodes.begin(), data->down_nodes.end(), std::back_inserter(req.down_nodes));
			req.Init(my_info, *node, my_info.GetId(), data->id);
			transport->SendDownlistRequest(req);
			scheduler->AddJob_(timeout_period, boost::bind(&CKadNode::DownlistRequestTimeout, this, data, node), node);
		} else {
			data->req_nodes.erase(node);
			delete node;
			if (!data->req_nodes.size())
				FinishDownlistRequests(data);
		}
	}

	void CKadNode::FinishDownlistRequests(DownlistRequestData *data) {
		downlist_requests.erase(data);
		std::set<DownlistRequestData::RequestedNode *>::iterator it = data->req_nodes.begin();
		for (; it != data->req_nodes.end();) {
			DownlistRequestData::RequestedNode *node = *it;
			scheduler->CancelJobsByOwner(node);
			delete node;
			it = data->req_nodes.erase(it);
		}
		delete data;
	}

	void CKadNode::Terminate() {
		TerminatePingRequests();
		TerminateFindRequests();
		TerminateStoreRequests();
		TerminateDownlistRequests();
	}

	void CKadNode::TerminatePingRequests() {
		PingRequests::iterator it;
		for (it = ping_requests.begin(); it != ping_requests.end(); ) {
			PingRequestData *data = *it;
			scheduler->CancelJobsByOwner(data);
			data->callback(TERMINATED, data->GetId());
			it = ping_requests.erase(it);
			delete data;
		}
	}

	void CKadNode::TerminateFindRequests() {
		FindRequests::iterator it;
		for (it = find_requests.begin(); it != find_requests.end(); ) {
			FindRequestData *data = *it;
			FindRequestData::Candidates::iterator cit;
			for (cit = data->candidates.begin(); cit != data->candidates.end();) {
				FindRequestData::Candidate *cand = &*cit;
				if (cand->type == FindRequestData::Candidate::PENDING) {
					scheduler->CancelJobsByOwner(cand);
				}
				cit = data->candidates.erase(cit);
				delete cand;
			}
			it = find_requests.erase(it);
			if (data->type == FindRequestData::FIND_NODE) {
				data->find_node_callback_(TERMINATED, NULL);
			} else {
				data->find_value_callback_(TERMINATED, NULL);
			}
			delete data;
		}
	}

	void CKadNode::TerminateStoreRequests() {
		StoreRequests::iterator it;
		for (it = store_requests.begin(); it != store_requests.end(); ) {
			StoreRequestData *data = *it;
			std::set<StoreRequestData::StoreNode *>::iterator sit;
			for (sit = data->store_nodes.begin(); sit != data->store_nodes.end(); ) {
				StoreRequestData::StoreNode *node = *sit;
				scheduler->CancelJobsByOwner(node);
				sit = data->store_nodes.erase(sit);
				delete node;
			}
			it = store_requests.erase(it);
			data->callback(TERMINATED, data->id, NULL);
			delete data;
		}
	}

	void CKadNode::TerminateDownlistRequests() {
		while (downlist_requests.size()) {
			FinishDownlistRequests(*downlist_requests.begin());
		}
	}

}
