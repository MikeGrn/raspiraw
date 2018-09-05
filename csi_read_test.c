#include "stdio.h"

#include <bcm_host.h>
#include <interface/mmal/mmal.h>
#include <interface/mmal/util/mmal_default_components.h>
#include <interface/mmal/util/mmal_component_wrapper.h>

static MMAL_WRAPPER_T* comp;
static VCOS_SEMAPHORE_T sem;

static void mmalCallback(MMAL_WRAPPER_T* comp) {
    printf("Callback called\n");
    vcos_semaphore_post(&sem);
}

int main(int argc, char const* argv[]) {
    printf("test2\n");
    
    bcm_host_init();
    if (vcos_semaphore_create(&sem, "comp sem", 0) != VCOS_SUCCESS) {
        printf("sem error\n");
        exit(1);
    }

    if (mmal_wrapper_create(&comp, MMAL_COMPONENT_DEFAULT_CAMERA) != MMAL_SUCCESS) {
        printf("com error \n");
        exit(1);
    }
    
    return 0;
}
