#include "store.h"

using namespace dhtpp;

void CStore::StoreItem(const NodeID &key, const std::string &value) {
	store.insert(std::make_pair(key, value));
}

void CStore::GetItems(const NodeID &key, std::vector<std::string> &out_values) {
	Store::iterator it1, it2;
	it1 = store.lower_bound(key);
	it2 = store.upper_bound(key);
	for (; it1 != it2; ++it1) {
		out_values.push_back(it1->second);
	}
}