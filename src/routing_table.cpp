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

	bool CRoutingTable::AddContact(const Contact &contact) {
		BigInt id = (BigInt) contact.GetId();
		Buckets::iterator it = buckets.lower_bound(id, Comp());
		assert(it != buckets.end());

		CKbucket::ErrorCode res = it->AddContact(contact);
		if (res == CKbucket::SUCCEED) {
			return true;
		} else if (res == CKbucket::FULL) {
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

				return AddContact(contact);

			} else if (ptr == holder_brother_bucket) {
				// ForceK optimization
				uint16 count = K - holder_bucket->GetContactsNumber();
				assert(count >= 0);
				if (count > 0) {
					return holder_brother_bucket->AddContactForceK(contact, holder_id, count);
				}
			}

			// do nothing
			return false;
		}

		// internal error
		return false;
	}

	void CRoutingTable::GetClosestContacts(const NodeID &id_, std::vector<NodeInfo> &out_contacts) {
		BigInt id = (BigInt) id_;
		Buckets::iterator it = buckets.lower_bound(id, Comp());
		assert(it != buckets.end());

		it->GetContacts(out_contacts);

		if (out_contacts.size() == K)
			return;

		assert(out_contacts.size() < K);

		// Less than K contacts, continue the searching
		uint16 contacts_needed = K - out_contacts.size();
		std::vector<NodeInfo> additional_contacts;

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
		for (int i = 0; i < sorted_buckets.size(); ++i) {
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
}