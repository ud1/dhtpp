#include "../src/kbucket.h"
#include "../src/simulator.h"
#include "../src/stats.h"
#include "../src/config.h"

#include <cassert>
#include <stdlib.h>
#include <string>
#include <stdlib.h>

#include <boost/lexical_cast.hpp>

#include <crtdbg.h>

using namespace dhtpp;

void testKBucket() {
	NullNodeID null_id;
	MaxNodeID max_id;
	CKbucket bucket(null_id, max_id);

	// bucket is empty
	Contact temp;
	assert(bucket.LastSeenContact(temp) == false);
	assert(bucket.RemoveContact(temp.id) == false);

	// insert one contact
	temp.last_seen = 10;
	memset(temp.id.id, 0, sizeof(temp.id.id));
	assert(bucket.AddContact(temp) == SUCCEED);
	Contact temp2;
	assert(bucket.LastSeenContact(temp2) == true);
	assert(temp.id == temp2.id);
	assert(bucket.RemoveContact(temp2.id) == true);
	assert(bucket.RemoveContact(temp2.id) == false);

	// insert several contacts
	for (int i = 0; i < K-1; ++i) {
		temp.id.id[0] = i;
		temp.last_seen = i + 10;
		assert(bucket.AddContact(temp) == SUCCEED);
	}

	// Already exist check
	for (int i = 0; i < K-1; ++i) {
		temp.id.id[0] = i;
		temp.last_seen = 0;
		assert(bucket.AddContact(temp) == EXISTED);
	}

	temp.id.id[0] = K-1;
	temp.last_seen = 1;
	assert(bucket.AddContact(temp) == SUCCEED);

	// Check FULL
	temp.id.id[0] = K;
	temp.last_seen = 1;
	assert(bucket.AddContact(temp) == FULL);

	temp.id.id[0] = 5;
	assert(bucket.RemoveContact(temp.id) == true);
}

void testNodeId() {
	unsigned int a, b, c;
	a = 0xabcdef;
	b = 0xffaabb;
	c = a + b;
	NodeID ida, idb, idc;
	ida = NullNodeID() + a;
	idb = NullNodeID() + b;
	idc = NullNodeID() + c;
	assert((ida + idb) == idc);
	c = b - a;
	idc = NullNodeID() + c;
	assert((idb - ida) == idc);
	c = a ^ b;
	idc = NullNodeID() + c;
	assert((ida ^ idb) == idc);
	c = b >> 1;
	idc = NullNodeID() + c;
	assert((idb >> 1) == idc);
	c = b >> 2;
	idc = NullNodeID() + c;
	assert((idb >> 2) == idc);
	c = b >> 10;
	idc = NullNodeID() + c;
	assert((idb >> 10) == idc);
	assert(ida < idb);
	idb = NullNodeID() + b;
	idb += a;
	c = a + b;
	idc = NullNodeID() + c;
	assert(idb == idc);
}

int main() {
	//testKBucket();
	//_CrtSetDbgFlag(
	//	_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) |
	//	_CRTDBG_LEAK_CHECK_DF |
	//	_CRTDBG_DELAY_FREE_MEM_DF |
	//	_CRTDBG_CHECK_ALWAYS_DF
	//	);

	//testNodeId();

	int nodesN = 20000;

	srand(time(0));

	CStats stats;
	stats.SetNodesN(nodesN);
	std::string filename;
	filename = boost::lexical_cast<std::string>(time(NULL));
	filename += ".txt";
	stats.Open(filename);

	CSimulator sim(nodesN, &stats);
	sim.Run(run_time);

	return 0;
}