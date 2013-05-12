/* Preprocessor defines added by opencl compiler
 * #define NUM_MATCH_CONFIGS
 * #define CONFIGS_PER_PROC
 * #define NUM_MATCHES
 * #define NUM_ROUNDS
 * #define SPACED_DISTANCE
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
verify_spaced_constraint(__private char *current_schedule,
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

__kernel void start_trampoline(__global char *match_configs,
				__global char *output,
				__global char *scratch_buffer)
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

			char valid =
			verify_spaced_constraint(current_schedule,
					schedule_depth);

			scratch_buffer[scratch_idx(schedule_depth / NUM_MATCHES,
						schedule_depth % NUM_MATCHES,
						startloc + cur_config)] =
				(valid) ? CONFIG_VALID : 0;
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
