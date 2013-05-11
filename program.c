void group_leader(__global uint8_t *match_configs)
{
	return;
}

void group_worker(__global uint8_t *match_configs)
{
	return;
}

__kernel void start_trampoline(__global uint8_t *match_configs)
{
	if (get_local_id() == 0)
		group_leader(match_configs);
	else
		group_worker(match_configs);

	return;
}
