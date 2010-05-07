#ifndef DHT_ROUTING_TABLE_H
#define DHT_ROUTING_TABLE_H

#include "kbucket.h"

#include <boost/intrusive/set.hpp>

#include <vector>

namespace dhtpp {

	class CRoutingTable {
	public:
		CRoutingTable(const NodeID &id);
		bool AddContact(const Contact &contact);
		bool RemoveContact(const NodeID &node_id);
		bool GetContact(const NodeID &id, NodeInfo &cont) const;
		void GetClosestContacts(const NodeID &id, std::vector<NodeInfo> &out_contacts);
		void SaveBootstrapContacts(std::vector<NodeAddress> &out) const;

	protected:
		class CKbucketEntry : 
			public CKbucket, 
			public boost::intrusive::set_base_hook<> 
		{
		public:
			CKbucketEntry(const BigInt &low_bound, const BigInt &high_bound) :
				CKbucket(low_bound, high_bound) {}

			bool operator <(const CKbucketEntry &o) const {
				return GetHighBound() < o.GetHighBound();
			}
		};

		struct Comp {
			bool operator()(const CKbucketEntry &b1, const BigInt &bi2) const {
				return b1.GetHighBound() < bi2;
			}
			bool operator()(const BigInt &bi1, const CKbucketEntry &b2) const {
				return bi1 < b2.GetHighBound();
			}
		};

		typedef boost::intrusive::set<CKbucketEntry> Buckets;

		Buckets buckets;
		CKbucketEntry *holder_bucket, *holder_brother_bucket;
		NodeID holder_id;
	};

}

#endif // DHT_ROUTING_TABLE_H
