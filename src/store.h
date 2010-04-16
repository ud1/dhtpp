#ifndef DHT_STORE_H
#define DHT_STORE_H

namespace dhtpp {

	class CStore {
	public:
		bool StoreItem(const NodeID &key, const std::string &value);
		void GetItems(const NodeID &key, std::vector<std::string> &out_values);
	};
}

#endif // DHT_STORE_H
