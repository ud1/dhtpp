filename = "1273923776.txt"
rpc_count_time_delta = 60000
succeed_find_value_delta = 50
failed_find_value_delta = 200
nodes_number = 0

avg_on_time_val = 0
avg_off_time_val = 0

rpc_count = {}
closest_contacts_hist = {}
routing_table_hist = {}
succeed_find_value_duration_hist = {}
succeed_find_value_max_duration = 0
failed_find_value_duration_hist = {}
failed_find_value_max_duration = 0
findnode_hist = {}
findnode_max = 0;
findvalue_hist = {}
findvalue_max = 0;
min_time = 0;

function avg_on_time(str)
	avg_on_time_val = tonumber(str)
	print("avg_on_time", avg_on_time_val / 60000)
end

function avg_off_time(str)
	avg_off_time_val = tonumber(str)
	print("avg_off_time", avg_off_time_val / 60000)
	min_time = avg_on_time_val + avg_off_time_val
	print("min_time", min_time / 60000)
end

function nodes_number_func(str)
	nodes_number = tonumber(str)
	print("nodes_number", nodes_number)
end

function DOWNLIST_OPTIMIZATION_func(str)
	print("DOWNLIST_OPTIMIZATION", str) 
end

function count_rpc(t, cnt)
	t = math.floor(t / rpc_count_time_delta)
	rpc_count[t] = (rpc_count[t] or 0) + sum
end

prev_rpc = {
	ping_req = 0,
	store_req = 0,
	find_node_req = 0,
	find_value_req = 0,
	downlist_req = 0,
	ping_resp = 0,
	store_resp = 0,
	find_node_resp = 0,
	find_value_resp = 0,
	downlist_resp = 0
}
rpc_last_time = 0
rpc_count = {
	ping = 0,
	store = 0,
	find_node = 0,
	find_value = 0,
	downlist = 0
}
function rpcs(str)
	_, _, t,
	ping_req, store_req, find_node_req, find_value_req, downlist_req,
	ping_resp, store_resp, find_node_resp, find_value_resp, downlist_resp = 
	string.find(str, "(%d+);(%d+);(%d+);(%d+);(%d+);(%d+);(%d+);(%d+);(%d+);(%d+);(%d+)")
	
	t = tonumber(t)
	prev_rpc.ping_req,			ping_req		= tonumber(ping_req),			tonumber(ping_req)			- prev_rpc.ping_req
	prev_rpc.store_req,			store_req		= tonumber(store_req),			tonumber(store_req)			- prev_rpc.store_req
	prev_rpc.find_node_req,		find_node_req	= tonumber(find_node_req),		tonumber(find_node_req)		- prev_rpc.find_node_req
	prev_rpc.find_value_req,	find_value_req	= tonumber(find_value_req),		tonumber(find_value_req)	- prev_rpc.find_value_req
	prev_rpc.downlist_req,		downlist_req	= tonumber(downlist_req),		tonumber(downlist_req)		- prev_rpc.downlist_req
	prev_rpc.ping_resp,			ping_resp		= tonumber(ping_resp),			tonumber(ping_resp)			- prev_rpc.ping_resp
	prev_rpc.store_resp,		store_resp		= tonumber(store_resp),			tonumber(store_resp)		- prev_rpc.store_resp
	prev_rpc.find_node_resp,	find_node_resp	= tonumber(find_node_resp),		tonumber(find_node_resp)	- prev_rpc.find_node_resp
	prev_rpc.find_value_resp,	find_value_resp = tonumber(find_value_resp),	tonumber(find_value_resp)	- prev_rpc.find_value_resp
	prev_rpc.downlist_resp,		downlist_resp	= tonumber(downlist_resp),		tonumber(downlist_resp)		- prev_rpc.downlist_resp
	
	sum = ping_req + store_req + find_node_req + find_value_req + downlist_req +
		ping_resp + store_resp + find_node_resp + find_value_resp + downlist_resp
	count_rpc(t, sum)
	
	rpc_last_time = t
	if t < min_time then return end
	
	rpc_count.ping			= rpc_count.ping		+ ping_req			+ ping_resp
	rpc_count.store			= rpc_count.store		+ store_req			+ store_resp
	rpc_count.find_node		= rpc_count.find_node	+ find_node_req		+ find_node_resp
	rpc_count.find_value	= rpc_count.find_value	+ find_value_req	+ find_value_resp
	rpc_count.downlist		= rpc_count.downlist	+ downlist_req		+ downlist_resp
end;

avg_routing_table_contacts = 0
avg_routing_table_contacts_count = 0
function node_info(str)
	_, _,
	closest_contactsN, closest_contacts_activeN,
	routing_tableN, routing_table_activeN =
	string.find(str, "(%d+);(%d+);(%d+);(%d+)")
	
	closest_contactsN = tonumber(closest_contactsN)
	closest_contacts_activeN = tonumber(closest_contacts_activeN)
	routing_tableN = tonumber(routing_tableN)
	routing_table_activeN = tonumber(routing_table_activeN)
	
	closest_contacts_hist[closest_contacts_activeN] = 
	(closest_contacts_hist[closest_contacts_activeN] or 0) + 1
	
	pos = math.floor(10*routing_table_activeN / (routing_tableN+1))
	routing_table_hist[pos] = 
	(routing_table_hist[pos] or 0) + 1
	
	if routing_tableN > 0 then
		avg_routing_table_contacts = avg_routing_table_contacts + routing_table_activeN / routing_tableN
	end
	avg_routing_table_contacts_count = avg_routing_table_contacts_count + 1
end


avg_succeed_findvalue_duration = 0
avg_succeed_findvalue_count = 0
function succeed_find_value(str)
	_, _,
	t, duration = string.find(str, "(%d+);(%d+)")
	t = tonumber(t)
	if t < min_time then return end

	duration = tonumber(duration)
	avg_succeed_findvalue_duration = avg_succeed_findvalue_duration + duration
	avg_succeed_findvalue_count = avg_succeed_findvalue_count + 1
	duration = math.floor(duration / succeed_find_value_delta)
	
	succeed_find_value_duration_hist[duration] = 
	(succeed_find_value_duration_hist[duration] or 0) + 1
	
	if duration > succeed_find_value_max_duration then
		succeed_find_value_max_duration = duration
	end
end


avg_failed_findvalue_duration = 0
avg_failed_findvalue_count = 0
function failed_find_value(str)
	_, _,
	t, duration = string.find(str, "(%d+);(%d+)")
	t = tonumber(t)
	if t < min_time then return end

	duration = tonumber(duration)
	avg_failed_findvalue_duration = avg_failed_findvalue_duration + duration
	avg_failed_findvalue_count = avg_failed_findvalue_count + 1
	duration = math.floor(duration / failed_find_value_delta)
	
	failed_find_value_duration_hist[duration] = 
	(failed_find_value_duration_hist[duration] or 0) + 1
	
	if duration > failed_find_value_max_duration then
		failed_find_value_max_duration = duration
	end
end


function find_node_hist(str)
	_, _, t, h = string.find(str, "([%d]+);(.+)")
	t = tonumber(t)
	if t < min_time then return end	
	
	pos = 1
	while true do
		i, j = string.find(h, ";", pos)
		if not i then break end
		p = string.sub(h, pos, i-1)
		pos = i+1
		_, _, a, b = string.find(p, "([%d]+)|([%d]+)")
		a = tonumber(a)
		b = tonumber(b)
		findnode_hist[a] = 
			(findnode_hist[a] or 0) + b
		if a > findnode_max then
			findnode_max = a
		end
	end
end

function find_value_hist(str)
	_, _, t, h = string.find(str, "([%d]+);(.+)")
	t = tonumber(t)
	if t < min_time then return end	
	
	pos = 1
	while true do
		i, j = string.find(h, ";", pos)
		if not i then break end
		p = string.sub(h, pos, i-1)
		pos = i+1
		_, _, a, b = string.find(p, "([%d]+)|([%d]+)")
		a = tonumber(a)
		b = tonumber(b)
		findvalue_hist[a] = 
			(findvalue_hist[a] or 0) + b
		if a > findvalue_max then
			findvalue_max = a
		end
	end
end

functions = {}
functions["avg_on_time"] = avg_on_time
functions["avg_off_time"] = avg_off_time
functions["nodes_number"] = nodes_number_func
functions["DOWNLIST_OPTIMIZATION"] = DOWNLIST_OPTIMIZATION_func
functions["rpcs"] = rpcs
functions["node_info"] = node_info
functions["succeed_find_value"] = succeed_find_value
functions["failed_find_value"] = failed_find_value
functions["find_node_hist"] = find_node_hist
functions["find_value_hist"] = find_value_hist

io.input(filename)

while true do
	local str = io.read()
	if str == nil then break end
	_, _, cmd, params = string.find(str, "([%a_]+);(.+)")
	if functions[cmd] then
		functions[cmd](params)
	end
end


rpc_period = (rpc_last_time - min_time) / 1000
print("rpc count")
for i, k in ipairs(rpc_count) do
	print(i, k)
end
print()

print("ping", rpc_count.ping / (rpc_period*nodes_number))
print("store", rpc_count.store / (rpc_period*nodes_number))
print("find_node", rpc_count.find_node / (rpc_period*nodes_number))
print("find_value", rpc_count.find_value / (rpc_period*nodes_number))
print("downlist", rpc_count.downlist / (rpc_period*nodes_number))
print()

avg_closest_contacts = 0
avg_closest_contacts_count = 0
print("closest_contacts_hist")
for i = 0, 10 do
	local cnt = closest_contacts_hist[i] or 0
	avg_closest_contacts = avg_closest_contacts + i*cnt
	avg_closest_contacts_count = avg_closest_contacts_count + cnt
	print(i, cnt)
end
avg_closest_contacts = avg_closest_contacts / avg_closest_contacts_count
print("avg_closest_contacts", avg_closest_contacts)
print()


print("routing_table_hist")
for i = 0, 10 do
	print(i, (routing_table_hist[i] or 0))
end
avg_routing_table_contacts = avg_routing_table_contacts / avg_routing_table_contacts_count
print("avg_routing_table_contacts", avg_routing_table_contacts)
print()


print("succeed_find_value_duration_hist")
for i = 0, succeed_find_value_max_duration do
	print(i, (succeed_find_value_duration_hist[i] or 0))
end
avg_succeed_findvalue_duration = avg_succeed_findvalue_duration / avg_succeed_findvalue_count
print("avg_succeed_findvalue_duration", avg_succeed_findvalue_duration)
print()


print("failed_find_value_duration_hist")
for i = 0, failed_find_value_max_duration do
	print(i, (failed_find_value_duration_hist[i] or 0))
end
avg_failed_findvalue_duration = avg_failed_findvalue_duration / avg_failed_findvalue_count
print("avg_failed_findvalue_duration", avg_failed_findvalue_duration)

print("failed/succed", avg_failed_findvalue_count / avg_succeed_findvalue_count)
print()


avg_findnode = 0
findnode_count = 0
print("findnode_hist")
for i = 0, findnode_max do
	local cnt = findnode_hist[i] or 0
	avg_findnode = avg_findnode + i*cnt
	findnode_count = findnode_count + cnt
	print(i, cnt)
end
avg_findnode = avg_findnode / findnode_count
print("avg_findnode", avg_findnode)
print()


avg_findvalue = 0
findvalue_count = 0
print("findvalue_hist")
for i = 0, findvalue_max do
	local cnt = findvalue_hist[i] or 0
	avg_findvalue = avg_findvalue + i*cnt
	findvalue_count = findvalue_count + cnt
	print(i, cnt)
end
avg_findvalue = avg_findvalue / findvalue_count
print("avg_findvalue", avg_findvalue)
print()
