#include "routing_table.h"

#include <cassert>
#include <iterator>

namespace dhtpp {

	CRoutingTable::CRoutingTable(const NodeID &id) {
		holder_id = id;
		memset(holder_brother_bucket, NULL, sizeof(holder_brother_bucket));

		holder_bucket = new CKbucketEntry(NullNodeID(), MaxNodeID());
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

	bool CRoutingTable::IdInHolderRange(const NodeID &id) const {
		if (holder_bucket->IdInRange(id))
			return true;
		return IsCloseToHolder(id);
	}

	RoutingTableErrorCode CRoutingTable::AddContact(const NodeInfo &info, bool &is_close_to_holder) {
		NodeID id = info.GetId();
		Buckets::iterator it = buckets.lower_bound(id, Comp());
		assert(it != buckets.end());

		is_close_to_holder = false;

		CKbucketEntry *ptr = &*it;
		RoutingTableErrorCode res = ptr->AddContact(info);

		if (res == SUCCEED) {
			if (ptr == holder_bucket)
				is_close_to_holder = true;
			return SUCCEED;
		} else if (res == FULL) {
			if (ptr == holder_bucket) {
				// Split the bucket
				NodeID wid = (ptr->GetHighBound() - ptr->GetLowBound());
				wid >>= rt_r;
				NodeID b_wid = wid >> (rt_b - rt_r);
				NodeID left_bound = ptr->GetLowBound();
				NodeID right_bound = left_bound + wid;
				int ind = 0;
				for (int i = 0; i < rt_pow2_r; ++i) {
					if ((left_bound <= holder_id) && (holder_id <= right_bound)) {
						holder_bucket = new CKbucketEntry(left_bound, right_bound);
						ptr->CopyContactsTo(*holder_bucket);
					} else {
						NodeID b_left_bound = left_bound;
						NodeID b_right_bound = b_left_bound + b_wid;
						for (int j = 0; j < rt_pow2_b_r; ++j) {
							holder_brother_bucket[ind] = new CKbucketEntry(b_left_bound, b_right_bound);
							ptr->CopyContactsTo(*holder_brother_bucket[ind]);
							b_left_bound = b_right_bound + 1;
							b_right_bound = b_left_bound + b_wid;
							++ind;
						}
					}
					left_bound = right_bound + 1;
					right_bound = left_bound + wid;
				}
				assert(ind == hld_br_buck_count);
	
				buckets.erase(it);
				delete ptr;

				buckets.insert(*holder_bucket);
				for (int i = 0; i < hld_br_buck_count; ++i) {
					buckets.insert(*holder_brother_bucket[i]);
				}

				return AddContact(info, is_close_to_holder);
			} else if (std::count(holder_brother_bucket, holder_brother_bucket + hld_br_buck_count, ptr)) {
				// ForceK optimization
				uint16 count = K - holder_bucket->GetContactsNumber();
				assert(count >= 0);
				if (count > 0) {
					return ptr->AddContactForceK(info, holder_id, count);
				}
			}

			// do nothing
			return FULL;
		}

		// already existed
		return EXISTED;
	}

	bool CRoutingTable::RemoveContact(const NodeID &node_id, bool &is_close_to_holder) {
		Buckets::iterator it = buckets.lower_bound(node_id, Comp());
		assert(it != buckets.end());

		if (&*it == holder_bucket) {
			is_close_to_holder = true;
		} else {
			is_close_to_holder = IsCloseToHolder(node_id);			
		}

		bool res = it->RemoveContact(node_id);
		return res;
	}

	bool CRoutingTable::GetContact(const NodeID &node_id, Contact &cont) const {
		Buckets::const_iterator it = buckets.lower_bound(node_id, Comp());
		assert(it != buckets.end());

		bool res = it->GetContact(node_id, cont);
		return res;
	}

	bool CRoutingTable::LastSeenContact(const NodeID &node_id, Contact &out) const {
		Buckets::const_iterator it = buckets.lower_bound(node_id, Comp());
		assert(it != buckets.end());

		bool res = it->LastSeenContact(out);
		return res;
	}

	void CRoutingTable::GetClosestContacts(const NodeID &id, std::vector<const Contact *> &out_contacts) const {
		Buckets::const_iterator it = buckets.lower_bound(id, Comp());
		assert(it != buckets.end());

		it->GetContacts(out_contacts);

		if (out_contacts.size() == K)
			return;

		assert(out_contacts.size() < K);

		// Less than K contacts, continue the searching
		uint16 contacts_needed = K - out_contacts.size();
		std::vector<const Contact *> additional_contacts;

		// sort buckets by the distance
		struct Buck {
			const CKbucketEntry *entry;
			Buck(const CKbucketEntry *e) {
				entry = e;
			}
			const NodeID &GetId() const {
				return entry->GetHighBound();
			}
		};

		std::vector<Buck> sorted_buckets;
		sorted_buckets.reserve(buckets.size());
		for (it = buckets.begin(); it != buckets.end(); ++it)
			sorted_buckets.push_back(Buck(&*it));

		std::sort(sorted_buckets.begin(), sorted_buckets.end(), distance_comp_le<Buck>(id));

		// extract additional contacts
		for (std::vector<Buck>::size_type i = 1; i < sorted_buckets.size(); ++i) {
			sorted_buckets[i].entry->GetContacts(additional_contacts);
			if (additional_contacts.size() >= contacts_needed)
				break;
		}

		if (additional_contacts.size() <= contacts_needed) {
			// return all what we have
			out_contacts.insert(out_contacts.end(), additional_contacts.begin(), additional_contacts.end());
			return;
		}

		// We have more than K contacts
		// sort them to choose _contacts_needed_ closest contacts
		std::nth_element(additional_contacts.begin(),
			additional_contacts.begin() + contacts_needed,
			additional_contacts.end(),
			distance_comp_le_ptr<const Contact *>(id));
		// copy contacts with smallest distance
		out_contacts.insert(out_contacts.end(), additional_contacts.begin(), additional_contacts.begin() + contacts_needed);
	}

	void CRoutingTable::GetClosestContacts(const NodeID &id, std::vector<NodeInfo> &out_contacts) const {
		std::vector<const Contact *> out_contacts_;
		GetClosestContacts(id, out_contacts_);
		std::vector<const Contact *>::iterator it = out_contacts_.begin();
		for (; it != out_contacts_.end(); ++it) {
			out_contacts.push_back(**it);
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

	bool CRoutingTable::IsCloseToHolder(const NodeID &id) const {
		std::vector<const Contact *> close_contacts;
		// Get contacts closest to us
		GetClosestContacts(holder_id, close_contacts);

		// Get min and max id
		NodeID min_id = MaxNodeID();
		NodeID max_id = NullNodeID();
		std::vector<const Contact *>::iterator it = close_contacts.begin();
		for (; it != close_contacts.end(); ++it) {
			if ((*it)->id < min_id)
				min_id = (*it)->id;
			if ((*it)->id > max_id)
				max_id = (*it)->id;
		}
		return (min_id <= id) && (id <= max_id);
	}

}