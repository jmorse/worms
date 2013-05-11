void group_leader()
{
	return;
}

void group_worker()
{
	return;
}

__kernel void start_trampoline(__global uint8_t *match_configs)
{
	if (get_local_id() == 0)
		group_leader();
	else
		group_worker();

	return;
}
