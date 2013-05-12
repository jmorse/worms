/* Preprocessor defines added by opencl compiler
 * #define CONFIGS_PER_PROC
 * #define NUM_MATCHES
 * #define NUM_ROUNDS
 */

void
verify_spaced_constraint(__private char *current_schedule,
			__private unsigned int schedule_depth)
{
	// If scheduling the first round, no spaced constraint.
	if (schedule_depth < NUM_MATCHES)
		return;

	return;
}

__kernel void start_trampoline(__global char *match_configs,
				__global char *output)
{
	__private unsigned int i, startloc, cur_config;
	// Per worker match configs.
	__private char local_match_configs[CONFIGS_PER_PROC * sizeof(char) * 4];
	// Current exploration schedule config
	__private char
		current_schedule[sizeof(char) * 4 * NUM_MATCHES * NUM_ROUNDS];
	// Current depth into schedule.
	__private unsigned int schedule_depth = 0;

	// Read in per worker match configs
	startloc = get_local_id(0) * CONFIGS_PER_PROC * 4;
	for (i = 0; i < CONFIGS_PER_PROC * 4; i++)
		local_match_configs[i] = match_configs[startloc + i];

	// Primary algorithm goes here...
	while (1) {
		// Enumerate through our set of configs to test
		for (cur_config = 0; cur_config < CONFIGS_PER_PROC;
				cur_config++) {
			// Copy this config into schedule.
			current_schedule[(schedule_depth * 4) + 0] =
				local_match_configs[(cur_config * 4) + 0];
			current_schedule[(schedule_depth * 4) + 1] =
				local_match_configs[(cur_config * 4) + 1];
			current_schedule[(schedule_depth * 4) + 2] =
				local_match_configs[(cur_config * 4) + 2];
			current_schedule[(schedule_depth * 4) + 3] =
				local_match_configs[(cur_config * 4) + 3];

			verify_spaced_constraint(current_schedule,
					schedule_depth);
		}

		// Break out on account of not being implemented right now.
		break;
	}

	for (i = 0; i < 256; i++) {
		output[i] = CONFIGS_PER_PROC;
	}
	write_mem_fence(CLK_GLOBAL_MEM_FENCE);
	return;

}
