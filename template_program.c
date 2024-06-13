#include <CL/cl.h>
#include <stdio.h>
#include <string.h>

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

#define CHECK_ERR(x, tag)                                                      \
  do {                                                                         \
    cl_int err = (x);                                                          \
    if (err != CL_SUCCESS) {                                                   \
      fprintf(stderr, "CL error %d in" __FILE__ ":%d %s\n", err, __LINE__,     \
              STR(tag));                                                       \
      return err;                                                              \
    }                                                                          \
  } while (0)

typedef struct CLResources_t {
  cl_event upload_events[2];
  cl_event kernel_event;
  cl_mem A;
  cl_mem B;
  cl_mem C;
  cl_kernel kernel;
  cl_command_queue queue;
  cl_context ctx;
  cl_device_id device;
} CLResources;

typedef struct HostData {
  int size;
  size_t datasize;
  int *A;
  int *B;
  int *C;
} HostData;

static const char *PROGRAM_SRC_STR = STR(

    void kernel vecadd(global int *A, global int *B, global int *C) {
      int idx = get_global_id(0);
      C[idx] = A[idx] + B[idx];
    });

cl_int init_host_data(HostData *data) {
  printf("Initializing host data...\n");
  data->datasize = data->size * sizeof(int);

  int *A = malloc(data->datasize);
  int *B = malloc(data->datasize);
  int *C = malloc(data->datasize);

  for (int i = 0; i < data->size; i++) {
    A[i] = i;
    B[i] = i;
  }

  data->A = A;
  data->B = B;
  data->C = C;

  return CL_SUCCESS;
}

cl_int free_host_data(HostData *data) {
  printf("Freeing host data...\n");

  free(data->A);
  free(data->B);
  free(data->C);

  return CL_SUCCESS;
}

cl_int get_device(cl_device_id *device) {

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
  cl_device_id dev = devices[0]; 
  *device = dev;

  free(devices);
  free(platform);
  free(platform_name);

  return CL_SUCCESS;
}

cl_int init_cl_resources(CLResources *cl, HostData *data) {

  printf("Initializing OpenCL resources...\n");
  cl_int error;

  cl->ctx = clCreateContext(NULL, 1, &cl->device, NULL, NULL, &error);
  CHECK_ERR(error, clCreateContext);

  cl->A =
      clCreateBuffer(cl->ctx, CL_MEM_READ_ONLY, data->datasize, NULL, &error);
  CHECK_ERR(error, clCreateBuffer);
  cl->B =
      clCreateBuffer(cl->ctx, CL_MEM_READ_ONLY, data->datasize, NULL, &error);
  CHECK_ERR(error, clCreateBuffer);
  cl->C =
      clCreateBuffer(cl->ctx, CL_MEM_WRITE_ONLY, data->datasize, NULL, &error);
  CHECK_ERR(error, clCreateBuffer);

  cl->queue =
      clCreateCommandQueueWithProperties(cl->ctx, cl->device, NULL, &error);
  CHECK_ERR(error, clCreateCommandQueueWithProperties);

  size_t program_len = strlen(PROGRAM_SRC_STR);
  cl_program program = clCreateProgramWithSource(cl->ctx, 1, &PROGRAM_SRC_STR,
                                                 &program_len, &error);
  CHECK_ERR(error, clCreateProgramWithSource);

  error = clBuildProgram(program, 1, &cl->device, NULL, NULL, NULL);
  if (error != CL_SUCCESS) {
    size_t log_len;
    CHECK(clGetProgramBuildInfo(program, cl->device, CL_PROGRAM_BUILD_LOG, 0,
                                NULL, &log_len));
    char *log = malloc(log_len);
    CHECK(clGetProgramBuildInfo(program, cl->device, CL_PROGRAM_BUILD_LOG,
                                log_len, log, NULL));
    printf("Program build failed. Program Source:\n%s\nError:\n%s\n",
           PROGRAM_SRC_STR, log);
    free(log);
    CHECK_ERR(error, clBuildProgram);
  }

  cl->kernel = clCreateKernel(program, "vecadd", &error);
  CHECK_ERR(error, clCreateKernel);
  CHECK(clReleaseProgram(program));

  return CL_SUCCESS;
}

cl_int free_cl_resources(CLResources *cl) {

  printf("Releasing OpenCL resources...\n");
  CHECK(clReleaseMemObject(cl->A));
  CHECK(clReleaseMemObject(cl->B));
  CHECK(clReleaseMemObject(cl->C));
  CHECK(clReleaseCommandQueue(cl->queue));
  CHECK(clReleaseKernel(cl->kernel));
  CHECK(clReleaseContext(cl->ctx));

  return CL_SUCCESS;
}

cl_int upload_to_device(CLResources *cl, HostData *data) {

  printf("Uploading buffers from host to device...\n");
  CHECK(clEnqueueWriteBuffer(cl->queue, cl->A, CL_FALSE, 0, data->datasize,
                             data->A, 0, NULL, &cl->upload_events[0]));
  CHECK(clEnqueueWriteBuffer(cl->queue, cl->B, CL_FALSE, 0, data->datasize,
                             data->B, 0, NULL, &cl->upload_events[1]));

  return CL_SUCCESS;
}

cl_int execute_kernel(CLResources *cl, HostData *data) {
  printf("Executing kernel on device...\n");

  CHECK(clSetKernelArg(cl->kernel, 0, sizeof(cl_mem), &cl->A));
  CHECK(clSetKernelArg(cl->kernel, 1, sizeof(cl_mem), &cl->B));
  CHECK(clSetKernelArg(cl->kernel, 2, sizeof(cl_mem), &cl->C));

  size_t global_work_size[1];
  global_work_size[0] = data->size;

  CHECK(clEnqueueNDRangeKernel(cl->queue, cl->kernel, 1, NULL, global_work_size,
                               NULL, 2, cl->upload_events, &cl->kernel_event));

  CHECK(clReleaseEvent(cl->upload_events[0]));
  CHECK(clReleaseEvent(cl->upload_events[1]));
  cl->upload_events[0] = NULL;
  cl->upload_events[1] = NULL;

  return CL_SUCCESS;
}

cl_int download_from_device(CLResources *cl, HostData *data) {
  printf("Downloading buffer from device to host...\n");

  CHECK(clEnqueueReadBuffer(cl->queue, cl->C, CL_TRUE, 0, data->datasize,
                            data->C, 1, &cl->kernel_event, NULL));
  CHECK(clReleaseEvent(cl->kernel_event));
  cl->kernel_event = NULL;
  CHECK(clFinish(cl->queue));

  return CL_SUCCESS;
}

cl_bool verify_results(HostData *data) {
  for (int i = 0; i < data->size; i++) {
    if (data->C[i] != i + i) {
      return CL_FALSE;
    }
  }
  return CL_TRUE;
}

int main() {

  CLResources cl;
  HostData data;
  data.size = 1024;

  CHECK(init_host_data(&data));
  CHECK(get_device(&cl.device));
  CHECK(init_cl_resources(&cl, &data));
  CHECK(upload_to_device(&cl, &data));
  CHECK(execute_kernel(&cl, &data));
  CHECK(download_from_device(&cl, &data));

  if (verify_results(&data) == CL_TRUE)
    printf("Output is correct.\n");
  else
    printf("Output is incorrect.\n");

  CHECK(free_cl_resources(&cl));
  CHECK(free_host_data(&data));

  return 0;
}