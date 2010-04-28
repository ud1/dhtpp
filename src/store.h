#ifndef DHT_STORE_H
#define DHT_STORE_H

#include "node_id.h"

#include <map>
#include <string>

namespace dhtpp {

	class CStore {
	public:
		void StoreItem(const NodeID &key, const std::string &value);
		void GetItems(const NodeID &key, std::vector<std::string> &out_values);

	private:
		typedef std::multimap<NodeID, std::string> Store;
		Store store;
	};
}

#endif // DHT_STORE_H
