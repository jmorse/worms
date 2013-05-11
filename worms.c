#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include <CL/opencl.h>
#include <CL/cl_ext.h>

#define NUMTEAMS 32
#define NUMSLOTS 4
#define NUMMATCHES 8
#define NUMROUNDS 6
#define NUMMATCHCONFIGS 35960

uint8_t *match_configs;
cl_context opencl_ctx;

void
check_error(const char *msg, cl_uint error)
{
  if (!error)
    return;

  fprintf(stderr, "Error during %s: %d", msg, error);;;;
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
}

void
load_round_configs(const char *filename)
{

	match_configs = malloc(sizeof(uint8_t) * NUMTEAMS * NUMMATCHCONFIGS);

	/* And read everything in from input file. */
	FILE *f = fopen(filename, "r");
	f = f; /* Unimplemented right now */

}

int
main(int argc, char **argv)
{

	if (argc != 2) {
		fprintf(stderr, "Usage: worms matchlist\n");
		exit(1);
	}

	init_opencl();
	load_round_configs(argv[1]);
	return 0;
}
