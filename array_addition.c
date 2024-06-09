#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>

#define STR(x) #x

#define CHECK(x)                                                  \
    do                                                            \
    {                                                             \
        cl_int err = (x);                                         \
        if (err != CL_SUCCESS)                                    \
        {                                                         \
            fprintf(stderr, "CL error %d in" __FILE__ ":%d %s\n", \
                    err, __LINE__, STR(x));                       \
            return err;                                           \
        }                                                         \
    } while (0)

int main()
{
    cl_platform_id *platform;
    cl_uint platforms;
    CHECK(clGetPlatformIDs(0, NULL, &platforms));
    platform = (cl_platform_id *)malloc(platforms * sizeof(cl_platform_id));
    CHECK(clGetPlatformIDs(platforms, platform, NULL));

    return 0;
}