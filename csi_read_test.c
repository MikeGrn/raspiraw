#include "stdio.h"

#include <bcm_host.h>
#include <interface/mmal/mmal.h>
#include <interface/mmal/util/mmal_component_wrapper.h>
#include <interface/mmal/util/mmal_default_components.h>

static MMAL_WRAPPER_T* rawcam;
static VCOS_SEMAPHORE_T sem;

static void mmalCallback(MMAL_WRAPPER_T* rawcam) {
    printf("Callback called\n");
    vcos_semaphore_post(&sem);
}

int main(int argc, char const* argv[]) {
    printf("test4\n");

    bcm_host_init();
    if (vcos_semaphore_create(&sem, "comp sem", 0) != VCOS_SUCCESS) {
        printf("sem error\n");
        exit(1);
    } else {
        printf("sem created\n");
    }

    if (mmal_wrapper_create(&rawcam, "vc.ril.rawcam") != MMAL_SUCCESS) {
        printf("rawcam error \n");
        exit(1);
    } else {
        printf("wrapper created\n");
    }
    rawcam->callback = mmalCallback;

    MMAL_PORT_T* output = NULL;
    MMAL_PARAMETER_CAMERA_RX_CONFIG_T rx_cfg;
    MMAL_STATUS_T status;
    MMAL_PARAMETER_CAMERA_RX_TIMING_T rx_timing;
    output = rawcam->output[0];

    rx_cfg.hdr.id = MMAL_PARAMETER_CAMERA_RX_CONFIG;
    rx_cfg.hdr.size = sizeof(rx_cfg);
    status = mmal_port_parameter_get(output, &rx_cfg.hdr);
    if (status != MMAL_SUCCESS) {
        printf("Failed to get cfg\n");
        goto component_destroy;
    } else {
        printf("port parameter get\n");
    }

    rx_cfg.unpack = MMAL_CAMERA_RX_CONFIG_UNPACK_NONE;
    rx_cfg.pack = MMAL_CAMERA_RX_CONFIG_PACK_NONE;

    rx_cfg.data_lanes = 2;
    rx_cfg.image_id = 0x2B;
    status = mmal_port_parameter_set(output, &rx_cfg.hdr);
    if (status != MMAL_SUCCESS) {
        printf("Failed to set cfg\n");
        goto component_destroy;
    } else {
        printf("port parameter set\n");
    }

    rx_timing.hdr.id = MMAL_PARAMETER_CAMERA_RX_TIMING;
    rx_timing.hdr.size = sizeof(rx_timing);
    status = mmal_port_parameter_get(output, &rx_timing.hdr);
    if (status != MMAL_SUCCESS)
    {
        printf("Failed to get timing\n");
        goto component_destroy;
    } else {
        printf("timing get\n");
    }
    status = mmal_port_parameter_set(output, &rx_timing.hdr);
    if (status != MMAL_SUCCESS)
    {
        printf("Failed to set timing\n");
        goto component_destroy;
    } else {
        printf("Timing is set \n");
    }

   if (output->is_enabled) {
      if (mmal_wrapper_port_disable(output) != MMAL_SUCCESS) {
         fprintf(stderr, "Failed to disable output port\n");
         exit(1);
      }
   }


   if (mmal_wrapper_port_enable(output, MMAL_WRAPPER_FLAG_PAYLOAD_ALLOCATE)
       != MMAL_SUCCESS) {
        fprintf(stderr, "Failed to enable output port\n");
        exit(1);
   } else {
        printf("Port enabled\n");
   }

   int eos = 0;
   MMAL_BUFFER_HEADER_T* out;
   while (!eos) {
    
      // Send output buffers to be filled with encoded image.
      while (mmal_wrapper_buffer_get_empty(output, &out, 0) == MMAL_SUCCESS) {
         if (mmal_port_send_buffer(output, out) != MMAL_SUCCESS) {
            fprintf(stderr, "Failed to send buffer\n");
            break;
         } else {
            printf("buffer sent\n");
         }
      }

      // Get filled output buffers.
      status = mmal_wrapper_buffer_get_full(output, &out, 0);
      if (status == MMAL_EAGAIN) {
        printf("egain\n");
         // No buffer available, wait for callback and loop.
         //vcos_semaphore_wait(&sem);
         continue;
      } else if (status != MMAL_SUCCESS) {
         fprintf(stderr, "Failed to get full buffer\n");
         exit(1);
      } else {
        printf("Output get\n");
      }

      printf("- received %i bytes\n", out->length);
      eos = out->flags & MMAL_BUFFER_HEADER_FLAG_EOS;

    
      mmal_buffer_header_release(out);
   }

   mmal_port_flush(output);

component_destroy:
    if (rawcam)
        mmal_wrapper_destroy(rawcam);

   vcos_semaphore_delete(&sem);
    return 0;
}
