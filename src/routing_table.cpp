#include "routing_table.h"

#include <cassert>
#include <iterator>

namespace dhtpp {

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

		// Check if we have brother
		if (&*it == holder_bucket) {
			holder_brother_bucket->GetContacts(additional_contacts);
		} else if (&*it == holder_brother_bucket) {
			holder_bucket->GetContacts(additional_contacts);
		}

		if (additional_contacts.size() < contacts_needed) {
			// We still have no enough contacts
			// go up through the tree

			CKbucketEntry *buck = it->parent_brother;
			while (buck) {
				buck->GetContacts(additional_contacts);
				if (additional_contacts.size() >= contacts_needed)
					break;

				buck = buck->parent_brother;
			}
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