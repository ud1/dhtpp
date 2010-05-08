#include "routing_table.h"

#include <cassert>
#include <iterator>

namespace dhtpp {

	CRoutingTable::CRoutingTable(const NodeID &id) {
		holder_id = id;
		holder_brother_bucket = NULL;

		BigInt high_bound;
		high_bound.pow2(8*NODE_ID_LENGTH_BYTES);
		high_bound--;
		holder_bucket = new CKbucketEntry(0, high_bound);
		buckets.insert(*holder_bucket);
	}

	CRoutingTable::~CRoutingTable() {
		Buckets::iterator it = buckets.begin();
		while (it != buckets.end()) {
			CKbucketEntry *entry = &*it;
			it = buckets.erase(it);
			delete entry;
		}
	}

	RoutingTableErrorCode CRoutingTable::AddContact(const NodeInfo &info) {
		BigInt id = (BigInt) info.GetId();
		Buckets::iterator it = buckets.lower_bound(id, Comp());
		assert(it != buckets.end());

		RoutingTableErrorCode res = it->AddContact(info);
		if (res == SUCCEED) {
			return SUCCEED;
		} else if (res == FULL) {
			CKbucketEntry *ptr = &*it;
			if (ptr == holder_bucket) {
				// Split the bucket
				BigInt wid = (ptr->GetHighBound() - ptr->GetLowBound())/2;
				CKbucketEntry *left = new CKbucketEntry(
					ptr->GetLowBound(),
					ptr->GetLowBound() + wid);

				CKbucketEntry *right = new CKbucketEntry(
					ptr->GetLowBound() + wid + 1,
					ptr->GetHighBound());

				ptr->Split(*left, *right);
				buckets.erase(it);
				buckets.insert(*left);
				buckets.insert(*right);
				delete ptr;

				if (left->IdInRange(holder_id)) {
					holder_bucket = left;
					holder_brother_bucket = right;
				} else {
					holder_bucket = right;
					holder_brother_bucket = left;
				}

				return AddContact(info);

			} else if (ptr == holder_brother_bucket) {
				// ForceK optimization
				uint16 count = K - holder_bucket->GetContactsNumber();
				assert(count >= 0);
				if (count > 0) {
					return holder_brother_bucket->AddContactForceK(info, holder_id, count);
				}
			}

			// do nothing
			return FULL;
		}

		// already existed
		return EXISTED;
	}

	bool CRoutingTable::RemoveContact(const NodeID &node_id) {
		BigInt id = (BigInt) node_id;
		Buckets::iterator it = buckets.lower_bound(id, Comp());
		assert(it != buckets.end());

		bool res = it->RemoveContact(node_id);
		return res;
	}

	bool CRoutingTable::GetContact(const NodeID &node_id, Contact &cont) const {
		BigInt id = (BigInt) node_id;
		Buckets::const_iterator it = buckets.lower_bound(id, Comp());
		assert(it != buckets.end());

		bool res = it->GetContact(node_id, cont);
		return res;
	}

	bool CRoutingTable::LastSeenContact(const NodeID &node_id, Contact &out) const {
		BigInt id = (BigInt) node_id;
		Buckets::const_iterator it = buckets.lower_bound(id, Comp());
		assert(it != buckets.end());

		bool res = it->LastSeenContact(out);
		return res;
	}

	void CRoutingTable::GetClosestContacts(const NodeID &id_, std::vector<Contact> &out_contacts) {
		BigInt id = (BigInt) id_;
		Buckets::iterator it = buckets.lower_bound(id, Comp());
		assert(it != buckets.end());

		it->GetContacts(out_contacts);

		if (out_contacts.size() == K)
			return;

		assert(out_contacts.size() < K);

		// Less than K contacts, continue the searching
		uint16 contacts_needed = K - out_contacts.size();
		std::vector<Contact> additional_contacts;

		// sort buckets by the distance
		struct Buck {
			const CKbucketEntry *entry;
			Buck(const CKbucketEntry *e) {
				entry = e;
			}
			const BigInt &GetId() const {
				return entry->GetHighBound();
			}
		};

		std::vector<Buck> sorted_buckets;
		sorted_buckets.reserve(buckets.size());
		for (it = buckets.begin(); it != buckets.end(); ++it)
			sorted_buckets.push_back(Buck(&*it));

		std::sort(sorted_buckets.begin(), sorted_buckets.end(), distance_comp_le<Buck>(id));

		// extract additional contacts
		for (std::vector<Buck>::size_type i = 0; i < sorted_buckets.size(); ++i) {
			sorted_buckets[i].entry->GetContacts(additional_contacts);
			if (additional_contacts.size() >= contacts_needed)
				break;
		}

		if (additional_contacts.size() <= contacts_needed) {
			// return all what we have
			std::copy(additional_contacts.begin(), additional_contacts.end(), std::back_inserter(out_contacts));
			return;
		}

		// We have more than K contacts
		// sort them
		std::sort(additional_contacts.begin(), additional_contacts.end(), distance_comp_le<NodeInfo>(id));
		// copy contacts with smallest distance
		std::copy(additional_contacts.begin(), additional_contacts.begin() + contacts_needed, std::back_inserter(out_contacts));
	}

	void CRoutingTable::GetClosestContacts(const NodeID &id, std::vector<NodeInfo> &out_contacts) {
		std::vector<Contact> out_contacts_;
		GetClosestContacts(id, out_contacts_);
		std::vector<Contact>::iterator it = out_contacts_.begin();
		for (; it != out_contacts_.end(); ++it) {
			out_contacts.push_back(*it);
		}
	}

	void CRoutingTable::SaveBootstrapContacts(std::vector<NodeAddress> &out) const {
		Buckets::const_iterator it;
		for (it = buckets.begin(); it != buckets.end(); ++it) {
			std::vector<Contact> contacts;
			it->GetContacts(contacts);
			for (std::vector<NodeInfo>::size_type i = 0; i < contacts.size(); ++i) {
				out.push_back(contacts[i]);
			}
		}
	}

}