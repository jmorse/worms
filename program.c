/* Preprocessor defines added by opencl compiler
 * #define NUM_MATCH_CONFIGS
 * #define CONFIGS_PER_PROC
 * #define NUM_MATCHES
 * #define NUM_ROUNDS
 * #define SPACED_DISTANCE
 * #define NUM_STREAM_PROCS
 */

#define roundbit(arr, round, idx) \
			(1 << arr[(round * NUM_MATCHES * 4) + idx])

#define scratch_idx(round, match, config) \
			((round * NUM_MATCHES * NUM_MATCH_CONFIGS) + \
			 (match * NUM_MATCH_CONFIGS) + \
			 config)

#define CONFIG_VALID 1
#define CONFIG_EXPLORED 2

int
verify_spaced_constraint(__local char *current_schedule,
			__private unsigned int schedule_depth)
{
	__private unsigned int round_boundries = schedule_depth % NUM_MATCHES;
	__private unsigned int cur_boundry, i, failure;

	failure = 0;
	for (cur_boundry = 0; cur_boundry < round_boundries; cur_boundry++) {
		long bitmask = 0;

		// Accumulate a bitmask of all the round bits from the prefix
		// of the earlier round
		for (i = 0; i < (SPACED_DISTANCE - 1); i++) {
			bitmask |=
			roundbit(current_schedule, cur_boundry, (i*4)+0);
			bitmask |=
			roundbit(current_schedule, cur_boundry, (i*4)+1);
			bitmask |=
			roundbit(current_schedule, cur_boundry, (i*4)+2);
			bitmask |=
			roundbit(current_schedule, cur_boundry, (i*4)+3);
		}

		for (; i < NUM_MATCHES + SPACED_DISTANCE; i++) {
			// If this match matches one of the previous bits, then
			// we have a failure
			failure |= (bitmask &
			roundbit(current_schedule, cur_boundry, (i*4)+0));

			failure |= (bitmask &
			roundbit(current_schedule, cur_boundry, (i*4)+1));

			failure |= (bitmask &
			roundbit(current_schedule, cur_boundry, (i*4)+2));

			failure |= (bitmask &
			roundbit(current_schedule, cur_boundry, (i*4)+3));

			// Accumulate the bits in the current round.
			bitmask |=
			roundbit(current_schedule, cur_boundry, (i*4)+0);
			bitmask |=
			roundbit(current_schedule, cur_boundry, (i*4)+1);
			bitmask |=
			roundbit(current_schedule, cur_boundry, (i*4)+2);
			bitmask |=
			roundbit(current_schedule, cur_boundry, (i*4)+3);

			// xor out
			bitmask ^=
			roundbit(current_schedule, cur_boundry,
					((i+1-SPACED_DISTANCE)*4)+0);
			bitmask ^=
			roundbit(current_schedule, cur_boundry,
					((i+1-SPACED_DISTANCE)*4)+1);
			bitmask ^=
			roundbit(current_schedule, cur_boundry,
					((i+1-SPACED_DISTANCE)*4)+2);
			bitmask ^=
			roundbit(current_schedule, cur_boundry,
					((i+1-SPACED_DISTANCE)*4)+3);
		}

	}

	return failure;
}

void
calcumalate_schedule_validity(unsigned int schedule_depth,
				unsigned int startloc,
				__private char *local_match_configs,
				__local char *current_schedule,
				__global char *scratch_buffer)
{
	unsigned int cur_config;
	// Enumerate through our set of configs to test
	for (cur_config = 0; cur_config < CONFIGS_PER_PROC; cur_config++) {
		// Copy this config into schedule.
		current_schedule[(schedule_depth * 4) + 0] =
			local_match_configs[(cur_config * 4) + 0];
		current_schedule[(schedule_depth * 4) + 1] =
			local_match_configs[(cur_config * 4) + 1];
		current_schedule[(schedule_depth * 4) + 2] =
			local_match_configs[(cur_config * 4) + 2];
		current_schedule[(schedule_depth * 4) + 3] =
			local_match_configs[(cur_config * 4) + 3];

		char invalid = verify_spaced_constraint(current_schedule,
							schedule_depth);

		scratch_buffer[scratch_idx(schedule_depth / NUM_MATCHES,
					schedule_depth % NUM_MATCHES,
					startloc + cur_config)] =
			(!invalid) ? CONFIG_VALID : 0;
	}

	return;
}

unsigned int
find_a_valid_config(unsigned int schedule_depth, unsigned int startloc,
			__global char *scratch_buffer)
{
	unsigned int cur_config, min_config, scratchidx;

	min_config = 0xFFFFFFFF;
	for (cur_config = 0; cur_config < CONFIGS_PER_PROC;
			cur_config++) {
		scratchidx = scratch_idx(schedule_depth / NUM_MATCHES,
					schedule_depth % NUM_MATCHES,
					startloc + cur_config);
		min_config = (scratch_buffer[scratchidx] == CONFIG_VALID)
			? (startloc + cur_config) : min_config;
	}

	return min_config;
}

unsigned int
work_out_next_config(__local unsigned short *best_config_per_proc)
{
	unsigned int i;
	unsigned short min_config = 0xFFFF;

	// Everyone check what the best config is.
	for (i = 0; i < NUM_STREAM_PROCS; i++)
		min_config = (min_config < best_config_per_proc[i])
			? min_config : best_config_per_proc[i];

	return min_config;
}

void
mark_config_as_explored(unsigned int schedule_depth, unsigned short min_config,
			__global char *scratch_buffer)
{
	unsigned int scratchidx;

	scratchidx = scratch_idx(schedule_depth / NUM_MATCHES,
				schedule_depth % NUM_MATCHES,
				min_config);
	scratch_buffer[scratchidx] |= CONFIG_EXPLORED;
}

unsigned int
install_match(unsigned int schedule_depth, unsigned int min_config,
		__local char *current_schedule, __global char *match_configs)
{
  unsigned int match_idx = min_config * 4; // 4 bytes per config
  unsigned int schedule_idx = schedule_depth * 4; // 4 bytes per schedule match

  current_schedule[schedule_idx] = match_configs[match_idx];
  current_schedule[schedule_idx + 1] = match_configs[match_idx + 1];
  current_schedule[schedule_idx + 2] = match_configs[match_idx + 2];
  current_schedule[schedule_idx + 3] = match_configs[match_idx + 3];
  return schedule_depth + 1;
}

unsigned int
pop_match(unsigned int schedule_depth, __local char *current_schedule)
{
	unsigned int idx = schedule_depth * 4;
	current_schedule[idx] = 0;
	current_schedule[idx + 1] = 0;
	current_schedule[idx + 2] = 0;
	current_schedule[idx + 3] = 0;
	return schedule_depth - 1;;
}

__kernel void start_trampoline(__global char *match_configs,
				__global unsigned int *output,
				__global char *scratch_buffer)
{
	__private unsigned int i, startloc, cur_config;
	// Per worker match configs.
	__private char local_match_configs[CONFIGS_PER_PROC * sizeof(char) * 4];
	// Current depth into schedule.
	__private unsigned int schedule_depth = 0;

	// An array of what each proc's best config number is.
	__local unsigned short best_config_per_proc[NUM_STREAM_PROCS];
	// Current exploration schedule config
	__local char
		current_schedule[sizeof(char) * 4 * NUM_MATCHES * NUM_ROUNDS];

	// Read in per worker match configs
	startloc = get_local_id(0) * CONFIGS_PER_PROC * 4;
	for (i = 0; i < CONFIGS_PER_PROC * 4; i++)
		local_match_configs[i] = match_configs[startloc + i];

	for (i = 0; i < 256; i++) {
		output[i] = 0;
	}

	// Primary algorithm goes here...
	__private unsigned int min_config;
	for (i = 0; i < 2; i++) {
		min_config = 0xFFFFFFFF;
		// Determine minimum good config from our run.
		min_config = 0xFFFF;
		calcumalate_schedule_validity(schedule_depth, startloc,
				local_match_configs, current_schedule,
				scratch_buffer);

		best_config_per_proc[get_local_id(0)] =
		find_a_valid_config(schedule_depth, startloc, scratch_buffer);

		// Lettuce sync
		barrier(CLK_LOCAL_MEM_FENCE);

		min_config = work_out_next_config(best_config_per_proc);

		mark_config_as_explored(schedule_depth, min_config,
					scratch_buffer);

		schedule_depth = install_match(schedule_depth, min_config,
						current_schedule,
						match_configs);

		output[i] = min_config;
	}

	write_mem_fence(CLK_GLOBAL_MEM_FENCE);
	return;

}
