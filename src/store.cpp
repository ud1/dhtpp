#include "store.h"
#include "timer.h"
#include "kad_node.h"

#include <boost/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/math/special_functions/beta.hpp>

#include <algorithm>
#include <stdlib.h>

namespace dhtpp {

	CStore::CStore(CKadNode *node_, CJobScheduler *scheduler_) {
		node = node_;
		scheduler = scheduler_;
	}

	CStore::~CStore() {
		Store::iterator it = store.begin();
		while (it != store.end()) {
			Item *item = it->second;
			scheduler->CancelJobsByOwner(item);
			delete item;
			it = store.erase(it);
		}
	}

	void CStore::StoreItem(const NodeID &key, const std::string &value, uint64 time_to_live) {
		Store::iterator it1, it2;
		it1 = store.lower_bound(key);
		it2 = store.upper_bound(key);
		Item *item = NULL;
		uint64 cur_time = GetTimerInstance()->GetCurrentTime();
		for (; it1 != it2; ++it1) {
			if (it1->second->value == value) {
				// Already have this value, update timings
				item = it1->second;
				item->expiration_time = std::max(item->expiration_time, cur_time + time_to_live);
				scheduler->CancelJobsByOwner(item);
				break;
			}
		}
		if (!item) {
			item = new Item;
			item->value = value;
			item->expiration_time = cur_time + time_to_live;
			store.insert(std::make_pair(key, item));
		}
		item->max_distance_setted = false;
		scheduler->AddJob_(GetRandomRepublishTime(), 
			boost::bind(&CStore::RepublishItem, this, key, item), item);
		scheduler->AddJob_(item->expiration_time - cur_time, 
			boost::bind(&CStore::DeleteItem, this, key, item), item);
	}

	void CStore::GetItems(const NodeID &key, std::vector<std::string> &out_values) {
		Store::iterator it1, it2;
		it1 = store.lower_bound(key);
		it2 = store.upper_bound(key);
		for (; it1 != it2; ++it1) {
			out_values.push_back(it1->second->value);
		}
	}

	void CStore::OnNewContact(const NodeInfo &contact) {
		uint64 cur_time = GetTimerInstance()->GetCurrentTime();
		Store::iterator it;
		for (it = store.begin(); it != store.end(); ++it) {
			Item *item = it->second;
			BigInt distance = (BigInt)it->first ^ (BigInt) contact.id;
			if (item->max_distance_setted && item->max_distance > distance) {
				node->StoreToNode(contact, it->first, item->value, item->expiration_time - cur_time,
					boost::bind(&CStore::StoreCallback, this, item,
					boost::lambda::_1, boost::lambda::_2, boost::lambda::_3));
			}
		}
	}

	void CStore::RepublishItem(NodeID key, Item *item) {
		uint64 cur_time = GetTimerInstance()->GetCurrentTime();
		if (cur_time >= item->expiration_time)
			return;
		node->Store(key, item->value, item->expiration_time - cur_time, 
			boost::bind(&CStore::StoreCallback, this, item,
			boost::lambda::_1, boost::lambda::_2, boost::lambda::_3));
		scheduler->AddJob_(GetRandomRepublishTime(), 
			boost::bind(&CStore::RepublishItem, this, key, item), item);
	}

	void CStore::DeleteItem(NodeID key, Item *item) {
		scheduler->CancelJobsByOwner(item);
		Store::iterator it1, it2;
		it1 = store.lower_bound(key);
		it2 = store.upper_bound(key);
		for (; it1 != it2; ++it1) {
			Item *t = it1->second;
			if (t == item) {
				store.erase(it1);
				delete item;
				return;
			}
		}
	}

	void CStore::StoreCallback(Item *item, CKadNode::ErrorCode code, rpc_id id, const BigInt *max_distance) {
		if (code == CKadNode::SUCCEED) {
			item->max_distance_setted = true;
			item->max_distance = *max_distance;
		}
	}

	uint64 CStore::GetRandomRepublishTime() {
#if 0
		return republish_time;
#else
		// beta-republish optimization
		double rnd = (double) rand() / RAND_MAX;
		double Ibeta = boost::math::ibeta_inv(2, 0.5, rnd);
		return (republish_time - republish_time_delta) + (uint64)(2*republish_time_delta*Ibeta);
#endif
	}

}


