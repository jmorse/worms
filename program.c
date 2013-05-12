/* Preprocessor defines added by opencl compiler
 * #define CONFIGS_PER_PROC
 */

__kernel void start_trampoline(__global char *match_configs,
				__global char *output)
{
	__private unsigned int i, startloc;
	// Per worker match configs.
	__private char local_match_configs[CONFIGS_PER_PROC * sizeof(char) * 4];

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
