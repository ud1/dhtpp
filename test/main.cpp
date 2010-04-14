#include "../src/kbucket.h"

#include <cassert>

using namespace dhtpp;

void testKBucket() {
	BigInt high_bound;
	high_bound.pow2(8*NODE_ID_LENGTH_BYTES);
	high_bound--;
	CKbucket bucket(0, high_bound);

	// bucket is empty
	Contact temp;
	assert(bucket.LastSeenContact(temp) == false);
	assert(bucket.RemoveContact(temp.node_id) == false);

	// insert one contact
	temp.last_seen = 10;
	memset(temp.node_id.id, 0, sizeof(temp.node_id.id));
	assert(bucket.AddContact(temp) == CKbucket::SUCCEED);
	Contact temp2;
	assert(bucket.LastSeenContact(temp2) == true);
	assert(temp.node_id == temp2.node_id);
	assert(bucket.RemoveContact(temp2.node_id) == true);
	assert(bucket.RemoveContact(temp2.node_id) == false);

	// insert several contacts
	for (int i = 0; i < K-1; ++i) {
		temp.node_id.id[0] = i;
		temp.last_seen = i + 10;
		assert(bucket.AddContact(temp) == CKbucket::SUCCEED);
	}

	// Already exist check
	for (int i = 0; i < K-1; ++i) {
		temp.node_id.id[0] = i;
		temp.last_seen = 0;
		assert(bucket.AddContact(temp) == CKbucket::FAILED);
	}

	temp.node_id.id[0] = K-1;
	temp.last_seen = 1;
	assert(bucket.AddContact(temp) == CKbucket::SUCCEED);

	// Check FULL
	temp.node_id.id[0] = K;
	temp.last_seen = 1;
	assert(bucket.AddContact(temp) == CKbucket::FULL);

	temp.node_id.id[0] = 5;
	assert(bucket.RemoveContact(temp.node_id) == true);
}

int main() {
	testKBucket();

	return 0;
}