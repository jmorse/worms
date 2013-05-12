#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <CL/opencl.h>
#include <CL/cl_ext.h>

#define NUMTEAMS 32
#define NUMSLOTS 4
#define NUMMATCHES 8
#define NUMROUNDS 6
#define NUMMATCHCONFIGS 35960
#define NUM_STREAM_PROCS 512
#define SPACED_DISTANCE 5

uint8_t *match_configs;
cl_context opencl_ctx;
cl_mem match_config_buf, output_buf;
cl_device_id device_id;
cl_kernel kernel;
cl_command_queue cmd_queue;
cl_event kernel_completion;
unsigned int configs_per_proc;

void
check_error(const char *msg, cl_uint error)
{
  if (!error)
    return;

  fprintf(stderr, "Error during %s: %d\n", msg, error);
  fflush(stderr);
  abort();
}

void
init_opencl()
{
	cl_int error;
	cl_uint platformIdCount = 0;
	clGetPlatformIDs(0, NULL, &platformIdCount);
	assert(platformIdCount == 1 && "More than one platform; bailing");

	cl_platform_id plat_ids[platformIdCount];
	clGetPlatformIDs(platformIdCount, plat_ids, NULL);

	cl_uint num_devices;
	clGetDeviceIDs(plat_ids[0], CL_DEVICE_TYPE_ALL, 0, NULL, &num_devices);
	assert(num_devices == 1 && "More than one device; bailing");

	cl_device_id dev_ids[num_devices];
	clGetDeviceIDs(plat_ids[0], CL_DEVICE_TYPE_ALL, 1, dev_ids, 0);

	cl_context_properties props[] = {
		CL_CONTEXT_PLATFORM, (cl_context_properties)plat_ids[0], 0, 0 };

	opencl_ctx = clCreateContext(props, num_devices, dev_ids,
					NULL, NULL, &error);
	check_error("creating context", error);
	device_id = dev_ids[0];
}

void
load_round_configs(const char *filename)
{
	unsigned int matches[NUMSLOTS], i, j, k, sz;

	sz = sizeof(uint8_t) * NUMSLOTS * NUMMATCHCONFIGS;
	match_configs = malloc(sz);

	/* And read everything in from input file. */
	FILE *f = fopen(filename, "r");
	j = 0;
	while (fscanf(f, "(%u, %u, %u, %u)\n", &matches[0], &matches[1],
				&matches[2], &matches[3]) == 4) {
		for (i = 0; i < NUMSLOTS; i++) {
			match_configs[(j * NUMSLOTS) + i] = matches[i]; }
		j++;
	}

	fprintf(stderr, "Read in %d match configs\n", j);

	// Calculate how many configs each processor will have. Fill in the
	// end with duplicates of the last entry, so that we don't have to
	// have any conditionals in the opencl proc. If needs be, all the way
	// up to a full processor being all duplicates.
	configs_per_proc = j / (NUM_STREAM_PROCS - 1);
	unsigned int fill = j % configs_per_proc;
	unsigned int remainder = configs_per_proc - fill;
	// Fill in the remainder.
	match_configs = realloc(match_configs, sz + (remainder * NUMSLOTS));
	for (k = 0; k < remainder; k++) {
		// Duplicate
		memcpy(&match_configs[(NUMMATCHCONFIGS + k) * NUMSLOTS],
				&match_configs[(j - 1) * NUMSLOTS],
				NUMSLOTS * sizeof(uint8_t));
	}
}

void
prepare_job_scenario()
{
	struct stat s;
	cl_int error;

	if (stat("program.c", &s) < 0) {
		perror("Couldn't stat program.c");
		exit(EXIT_FAILURE);
	}

	if (s.st_size >= 100000) {
		fprintf(stderr, "Unreasonably sized program.c to dump on the "
				"stack (%ld bytes)\n", (long)s.st_size);
		exit(EXIT_FAILURE);
	}

	char prog_buffer[s.st_size + 1];
	memset(prog_buffer, 0, s.st_size + 1);
	FILE *f = fopen("program.c", "r");
	if (fread(prog_buffer, s.st_size, 1, f) != 1) {
		perror("Couldn't read from program.c");
		exit(EXIT_FAILURE);
	}

	fclose(f);

	const char *progtextptr = prog_buffer;
	cl_program prog = clCreateProgramWithSource(opencl_ctx, 1, &progtextptr,
							NULL, &error);
	check_error("creating program", error);

	char preproc_defines[100000];
	sprintf(preproc_defines,
		"-DCONFIGS_PER_PROC=%d -DNUM_MATCHES=%d -DNUM_ROUNDS=%d "
		"-DSPACED_DISTANCE=%d -DNUM_MATCH_CONFIGS=%d",
		configs_per_proc, NUMMATCHES, NUMROUNDS, SPACED_DISTANCE,
		NUMMATCHCONFIGS);
	error = clBuildProgram(prog, 1, &device_id, preproc_defines,
				NULL, NULL);

	if (error != CL_SUCCESS) {
		char buffer[10000];
		error = clGetProgramBuildInfo(prog, device_id,
						CL_PROGRAM_BUILD_LOG,
						sizeof(buffer), buffer, NULL);
		check_error("checking error on program build", error);
		fprintf(stderr, "Error building OpenCL program, text is:\n%s",
				buffer);
		fflush(stderr);
		exit(EXIT_FAILURE);
	}

	check_error("building program", error);

	kernel = clCreateKernel(prog, "start_trampoline", &error);
	check_error("creating kernel", error);

	match_config_buf = clCreateBuffer(opencl_ctx,
				CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
				sizeof(uint8_t) * NUMSLOTS * configs_per_proc
							* NUM_STREAM_PROCS,
				match_configs, &error);
	check_error("creating buffer", error);

	output_buf = clCreateBuffer(opencl_ctx,
				CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,
				1024, NULL, &error);
	check_error("creating output buffer", error);

	error = clSetKernelArg(kernel, 0, sizeof(cl_mem), &match_config_buf);
	check_error("setting kernel arg 0", error);
	error = clSetKernelArg(kernel, 1, sizeof(cl_mem), &output_buf);
	check_error("setting kernel arg 1", error);

	cmd_queue = clCreateCommandQueue(opencl_ctx, device_id, 0, &error);
	check_error("create cmd queue", error);
}

int
main(int argc, char **argv)
{
	size_t g_work_size, l_work_size;
	cl_int error;

	if (argc != 2) {
		fprintf(stderr, "Usage: worms matchlist\n");
		exit(EXIT_FAILURE);
	}

	init_opencl();
	load_round_configs(argv[1]);
	prepare_job_scenario();

	/* Start this processing scenario. */
	g_work_size = NUM_STREAM_PROCS;
	l_work_size = NUM_STREAM_PROCS;
	error = clEnqueueNDRangeKernel(cmd_queue, kernel, 1, NULL, &g_work_size,
					&l_work_size, 0, NULL,
					&kernel_completion);
	check_error("enquing kernel", error);

	char output_data[1024];
	error = clEnqueueReadBuffer(cmd_queue, output_buf, true, 0, 1024,
					output_data, 1, &kernel_completion,
					NULL);
	unsigned int i;
	for (i = 0; i < 256; i++) {
		printf("%d,", output_data[i]);
		if ((i % 8) == 7)
			printf("\n");
	}

	exit(EXIT_SUCCESS);
}
