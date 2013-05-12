/* Preprocessor defines added by opencl compiler
 * #define CONFIGS_PER_PROC
 */

__kernel void start_trampoline(__global char *match_configs,
				__global char *output)
{

	__private unsigned int i;
	for (i = 0; i < 256; i++) {
		output[i] = CONFIGS_PER_PROC;
	}
	write_mem_fence(CLK_GLOBAL_MEM_FENCE);
	return;

}
