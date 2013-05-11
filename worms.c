#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#include <CL/opencl.h>
#include <CL/cl_ext.h>

#define NUMTEAMS 32
#define NUMSLOTS 4
#define NUMMATCHES 8
#define NUMROUNDS 6
#define NUMMATCHCONFIGS 35960

uint8_t *match_configs;
cl_context opencl_ctx;
cl_mem match_config_buf;
cl_device_id device_id;
cl_kernel kernel;
cl_command_queue cmd_queue;

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
	unsigned int matches[NUMTEAMS], i, j;

	match_configs = malloc(sizeof(uint8_t) * NUMTEAMS * NUMMATCHCONFIGS);

	/* And read everything in from input file. */
	FILE *f = fopen(filename, "r");
	j = 0;
	while (fscanf(f, "(%u, %u, %u, %u)\n", &matches[0], &matches[1],
				&matches[2], &matches[3]) == 4) {
		for (i = 0; i < NUMTEAMS; i++) {
			match_configs[(j * NUMTEAMS) + i] = matches[i];
		}
		j++;
	}

	fprintf(stderr, "Read in %d match configs\n", j);
}

void
prepare_job_scenario()
{
	cl_int error;
	const char *progtextptr = "#include \"program.c\"";
	cl_program prog = clCreateProgramWithSource(opencl_ctx, 1, &progtextptr,
							NULL, &error);
	check_error("creating program", error);

	error = clBuildProgram(prog, 1, &device_id, "", NULL, NULL);

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
				sizeof(uint8_t) * NUMTEAMS * NUMMATCHCONFIGS,
				match_configs, &error);
	check_error("creating buffer", error);

	error = clSetKernelArg(kernel, 0, sizeof(cl_mem), &match_config_buf);
	check_error("setting kernel arg", error);

	cmd_queue = clCreateCommandQueue(opencl_ctx, device_id, 0, &error);
	check_error("create cmd queue", error);
}

int
main(int argc, char **argv)
{

	if (argc != 2) {
		fprintf(stderr, "Usage: worms matchlist\n");
		exit(EXIT_FAILURE);
	}

	init_opencl();
	load_round_configs(argv[1]);
	prepare_job_scenario();
	exit(EXIT_SUCCESS);
}
