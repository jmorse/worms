void group_leader(__global char *match_configs)
{
	return;
}

void group_worker(__global char *match_configs)
{
	return;
}

__kernel void start_trampoline(__global char *match_configs,
				__global char *output)
{
	__local char rts[760];
	__local char cts[760];

	if (get_local_id(0) == 0)
		group_leader(match_configs);
	else
		group_worker(match_configs);

	return;
}
