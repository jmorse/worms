/* Preprocessor defines added by opencl compiler
 * #define CONFIGS_PER_PROC
 * #define NUM_MATCHES
 * #define NUM_ROUNDS
 */

__kernel void start_trampoline(__global char *match_configs,
				__global char *output)
{
	__private unsigned int i, startloc;
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

	for (i = 0; i < 256; i++) {
		output[i] = CONFIGS_PER_PROC;
	}
	write_mem_fence(CLK_GLOBAL_MEM_FENCE);
	return;

}
