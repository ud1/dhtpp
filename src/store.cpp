#include "store.h"
#include "timer.h"
#include "kad_node.h"

#include <boost/bind.hpp>
#include <boost/math/special_functions/beta.hpp>

#include <algorithm>
#include <stdlib.h>

namespace dhtpp {

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

	void CStore::RepublishItem(NodeID key, Item *item) {
		uint64 cur_time = GetTimerInstance()->GetCurrentTime();
		if (cur_time >= item->expiration_time)
			return;
		node->Store(key, item->value, item->expiration_time - cur_time, CKadNode::store_callback());
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


