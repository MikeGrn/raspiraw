/*
Copyright (c) 2012, Broadcom Europe Ltd
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// Camera demo using OpenMAX IL though the ilcient helper library
// modified from hello_video.c by HJ Imbens

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "bcm_host.h"
#include "libs/ilclient/ilclient.h"

#define kDecoderInputPort 130         
#define kDecoderOutputPort 131          

#define kSchedulerInputPort 10          
#define kSchedulerOutputPort 11         
#define kSchedulerClockPort 12          

#define kRendererInputPort 90         

#define kEGLRendererInputPort 220         
#define kEGLRendererImagePort 221

#define kClockOutputPort0 80          
#define kClockOutputPort1 81          
#define kClockOutputPort2 82          
#define kClockOutputPort3 83          
#define kClockOutputPort4 84          
#define kClockOutputPort5 85

#define kAudioDecoderInputPort 120          
#define kAudioDecoderOutputPort 121         

#define kAudioRendererInputPort 100         
#define kAudioRendererClockPort 101         

#define kAudioMixerClockPort 230          
#define kAudioMixerOutputPort 231         
#define kAudioMixerInputPort0 232         
#define kAudioMixerInputPort1 233         
#define kAudioMixerInputPort2 234         
#define kAudioMixerInputPort3 235         

#define kCameraPreviewPort 70         
#define kCameraCapturePort 71         
#define kCameraStillImagePort 72          
#define kCameraClockPort 73         

#define kResizeInputPort 60
#define kResizeOutputPort 61

static int camera_test()
{
   OMX_TIME_CONFIG_CLOCKSTATETYPE cstate;
   OMX_CONFIG_PORTBOOLEANTYPE cameraport;
   OMX_CONFIG_DISPLAYREGIONTYPE displayconfig;
   COMPONENT_T *camera = NULL, *video_render = NULL, *clock = NULL;
   COMPONENT_T *list[4];
   TUNNEL_T tunnel[3];
   ILCLIENT_T *client;
   int status = 0;
   int height = 600;
   int w = 4*height/3;
   int h = height;
   int x = (1280 - 4*height/3)/2;
   int y = (720 - height)/2;
   int layer = 0;

   memset(list, 0, sizeof(list));
   memset(tunnel, 0, sizeof(tunnel));

   if((client = ilclient_init()) == NULL)
   {
      return -3;
   }

   if(OMX_Init() != OMX_ErrorNone)
   {
      ilclient_destroy(client);
      return -4;
   }

   // create video_decode
   int res = ilclient_create_component(client, &camera, "vc.ril.rawcam", ILCLIENT_DISABLE_ALL_PORTS);
   if(res != 0)
      status = -14;
   list[0] = camera;

   // create video_render
   if(status == 0 && ilclient_create_component(client, &video_render, "video_render", ILCLIENT_DISABLE_ALL_PORTS) != 0)
      status = -14;
   list[1] = video_render;

   // create clock
   if(status == 0 && ilclient_create_component(client, &clock, "clock", ILCLIENT_DISABLE_ALL_PORTS) != 0)
      status = -14;
   list[2] = clock;

   // enable the capture port of the camera
   memset(&cameraport, 0, sizeof(cameraport));
   cameraport.nSize = sizeof(cameraport);
   cameraport.nVersion.nVersion = OMX_VERSION;
   cameraport.nPortIndex = kCameraCapturePort;
   cameraport.bEnabled = OMX_TRUE;
   if(camera != NULL && OMX_SetParameter(ILC_GET_HANDLE(camera), OMX_IndexConfigPortCapturing, &cameraport) != OMX_ErrorNone)
      status = -13;


   // configure the renderer to display the content in a 4:3 rectangle in the middle of a 1280x720 screen
   memset(&displayconfig, 0, sizeof(displayconfig));
   displayconfig.nSize = sizeof(displayconfig);
   displayconfig.nVersion.nVersion = OMX_VERSION;
   displayconfig.set = (OMX_DISPLAYSETTYPE)(OMX_DISPLAY_SET_FULLSCREEN | OMX_DISPLAY_SET_DEST_RECT | OMX_DISPLAY_SET_LAYER);
   displayconfig.nPortIndex = kRendererInputPort;
   displayconfig.fullscreen = (w > 0 && h > 0) ? OMX_FALSE : OMX_TRUE; 
   displayconfig.dest_rect.x_offset = x;
   displayconfig.dest_rect.y_offset = y;
   displayconfig.dest_rect.width = w;
   displayconfig.dest_rect.height = h;
   displayconfig.layer = layer;
   printf ("dest_rect: %d,%d,%d,%d\n", x, y, w, h);
   printf ("layer: %d\n", (int)displayconfig.layer);
   if (video_render != NULL && OMX_SetParameter(ILC_GET_HANDLE(video_render), OMX_IndexConfigDisplayRegion, &displayconfig) != OMX_ErrorNone) {
      status = -13;
      printf ("OMX_IndexConfigDisplayRegion failed\n");
   } else {
    printf("OMX_IndexConfigDisplayRegion\n");
   }

   // create a tunnel from the camera to the video_render component
   set_tunnel(tunnel+0, camera, kCameraCapturePort, video_render, kRendererInputPort);
   // create a tunnel from the clock to the camera
   set_tunnel(tunnel+1, clock, kClockOutputPort0, camera, kCameraClockPort);

   // setup both tunnels
   if(status == 0 && ilclient_setup_tunnel(tunnel+0, 0, 0) != 0) {
      status = -15;
   }
   if(status == 0 && ilclient_setup_tunnel(tunnel+1, 0, 0) != 0) {
      status = -15;
   }

   // change state of components to executing
   ilclient_change_component_state(camera, OMX_StateExecuting);
   ilclient_change_component_state(video_render, OMX_StateExecuting);
   ilclient_change_component_state(clock, OMX_StateExecuting);

   // start the camera by changing the clock state to running
   memset(&cstate, 0, sizeof(cstate));
   cstate.nSize = sizeof(displayconfig);
   cstate.nVersion.nVersion = OMX_VERSION;
   cstate.eState = OMX_TIME_ClockStateRunning;
   OMX_SetParameter (ILC_GET_HANDLE(clock), OMX_IndexConfigTimeClockState, &cstate);

   while (status == 0) {
      struct timespec theSleepTime;
      theSleepTime.tv_sec = 1000/1000;
      theSleepTime.tv_nsec = (1000%1000)*1000000;
      nanosleep(&theSleepTime, 0);
   }

   ilclient_disable_tunnel(tunnel);
   ilclient_disable_tunnel(tunnel+1);
   ilclient_teardown_tunnels(tunnel);

   ilclient_state_transition(list, OMX_StateIdle);
   ilclient_state_transition(list, OMX_StateLoaded);

   ilclient_cleanup_components(list);

   OMX_Deinit();

   ilclient_destroy(client);
   return status;
}

int main (int argc, char **argv)
{
   bcm_host_init();
   return camera_test();
}

