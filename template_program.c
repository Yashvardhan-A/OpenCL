#include <CL/cl.h>
#include <stdio.h>

#define STR(x) #x

#define CHECK(x)                                                               \
  do {                                                                         \
    cl_int err = (x);                                                          \
    if (err != CL_SUCCESS) {                                                   \
      fprintf(stderr, "CL error %d in" __FILE__ ":%d %s\n", err, __LINE__,     \
              STR(x));                                                         \
      return err;                                                              \
    }                                                                          \
  } while (0)

typedef struct CLResources_t {
  cl_event upload_event;
  cl_event kernel_event;
  cl_mem input_a;
  cl_mem input_b;
  cl_mem output;
  cl_kernel kernel;
  cl_command_queue queue;
  cl_context ctx;
  cl_device_id device;
} CLResources;

static const char *PROGRAM_SRC_STR = STR(

);

cl_int get_device(cl_device_id device) {

  cl_platform_id *platform;
  cl_uint platforms;
  size_t platform_name_size = 0;

  CHECK(clGetPlatformIDs(0, NULL, &platforms));
  platform = (cl_platform_id *)malloc(platforms * sizeof(cl_platform_id));
  CHECK(clGetPlatformIDs(platforms, platform, NULL));

  CHECK(clGetPlatformInfo(platform[0], CL_PLATFORM_NAME, 0, NULL,
                          &platform_name_size));
  char *platform_name = malloc(platform_name_size);
  CHECK(clGetPlatformInfo(platform[0], CL_PLATFORM_NAME, platform_name_size,
                          platform_name, NULL));
  printf("Platform name: %s\n", platform_name);

  cl_device_id *devices;
  cl_uint no_of_devices;
  size_t device_name_size;

  CHECK(
      clGetDeviceIDs(platform[0], CL_DEVICE_TYPE_ALL, 0, NULL, &no_of_devices));
  devices = (cl_device_id *)malloc(no_of_devices * sizeof(cl_device_id));
  CHECK(clGetDeviceIDs(platform[0], CL_DEVICE_TYPE_ALL, no_of_devices, devices,
                       NULL));

  for (int32_t i = 0; i < no_of_devices; i++) {
    CHECK(clGetDeviceInfo(devices[i], CL_DEVICE_NAME, 0, NULL,
                          &device_name_size));
    char *device_name = malloc(device_name_size);
    CHECK(clGetDeviceInfo(devices[i], CL_DEVICE_NAME, device_name_size,
                          device_name, NULL));
    printf("Device name: %s\n", device_name);

    free(device_name);
  }
  device = devices[0];

  free(devices);
  free(platform);
  free(platform_name);

  return CL_SUCCESS;
}

cl_int init_cl_resources(CLResources *cl) {
  
}

int main() {

  CLResources cl;
  get_device(cl.device);


  return 0;
}