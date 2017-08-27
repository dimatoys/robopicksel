/*
Chris Cummings
This is a heavily rewritten version of the core part of raspivid (copy right below)
and the work done by Pierre Raus at http://raufast.org/download/camcv_vid0.c to get
the camera feeding into opencv. It wraps up the camera system in a simple
StartCamera, StopCamera and callback to read data from the feed.
*/


/*
Copyright (c) 2013, Broadcom Europe Ltd
Copyright (c) 2013, James Hughes
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

#include "camera.h"
#include "mmalincludes.h"
#include "cameracontrol.h"
#include <stdio.h>
#include <unistd.h>

// Standard port setting for the camera component
#define MMAL_CAMERA_PREVIEW_PORT 0
#define MMAL_CAMERA_VIDEO_PORT 1
#define MMAL_CAMERA_CAPTURE_PORT 2

class CCamera
{
public:

	bool Init(int width, int height);
    bool Shoot(CameraCBFunction callback, void* data);
    void Disable();
	void Release();

	CCamera();
	~CCamera();
private:

	void OnCameraControlCallback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer);
	void OnStillBufferCallback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer);
	static void CameraControlCallback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer);
	static void StillBufferCallback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer);

	int							Width;
	int							Height;
	CameraCBFunction			Callback;
	RASPICAM_CAMERA_PARAMETERS	CameraParameters;
	MMAL_COMPONENT_T*			CameraComponent;
    MMAL_PORT_T*                Port;
	MMAL_POOL_T*				BufferPool;

    void*   ShootData;
};


static CCamera* GCamera = NULL;

void StartCamera(int width, int height)
{
	//can't create more than one camera
	if(GCamera != NULL)
	{
		printf("Can't create more than one camera\n");
		//return NULL;
	}

	//create and attempt to initialize the camera
	GCamera = new CCamera();
	if(!GCamera->Init(width,height))
	{
		//failed so clean up
		printf("Camera init failed\n");
		delete GCamera;
		GCamera = NULL;
	}
	//return GCamera;
}

bool Shoot(CameraCBFunction callback, void* data) {
    if (GCamera) {
        return GCamera->Shoot(callback, data);
    } else {
        printf("Camera is not started\n");
        return false;
    }
}

void StopCamera()
{
	if(GCamera)
	{
		delete GCamera;
		GCamera = NULL;
	}
}

CCamera::CCamera()
{
	Callback = NULL;
	CameraComponent = NULL;
}

CCamera::~CCamera()
{
	Release();
}

void CCamera::CameraControlCallback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
	GCamera->OnCameraControlCallback(port,buffer);
}
void CCamera::StillBufferCallback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
	GCamera->OnStillBufferCallback(port,buffer);
}

bool CCamera::Init(int width, int height)
{

	Release();
	//init broadcom host - QUESTION: can this be called more than once??
	bcm_host_init();

	//store basic parameters
	Width = width;       
	Height = height;

	// Set up the camera_parameters to default
	raspicamcontrol_set_defaults(&CameraParameters);

	MMAL_COMPONENT_T *camera = 0;
	MMAL_ES_FORMAT_T *format;
	MMAL_PORT_T *preview_port = NULL, *video_port = NULL, *still_port = NULL;
	MMAL_STATUS_T status;

	//create the camera component
	status = mmal_component_create(MMAL_COMPONENT_DEFAULT_CAMERA, &camera);
	if (status != MMAL_SUCCESS)
	{
		printf("Failed to create camera component\n");
		return false;
	}

	//check we have output ports
	if (!camera->output_num)
	{
		printf("Camera doesn't have output ports");
		mmal_component_destroy(camera);
		return false;
	}

	//get the 3 ports
	preview_port = camera->output[MMAL_CAMERA_PREVIEW_PORT];
	video_port = camera->output[MMAL_CAMERA_VIDEO_PORT];
	still_port = camera->output[MMAL_CAMERA_CAPTURE_PORT];

	// Enable the camera, and tell it its control callback function
	status = mmal_port_enable(camera->control, CameraControlCallback);
	if (status != MMAL_SUCCESS)
	{
		printf("Unable to enable control port : error %d", status);
		mmal_component_destroy(camera);
		return false;
	}

	//  set up the camera configuration
	{
		MMAL_PARAMETER_CAMERA_CONFIG_T cam_config;
		cam_config.hdr.id = MMAL_PARAMETER_CAMERA_CONFIG;
		cam_config.hdr.size = sizeof(cam_config);
		cam_config.max_stills_w = Width;
		cam_config.max_stills_h = Height;
		cam_config.stills_yuv422 = 0;
		cam_config.one_shot_stills = 0;
		cam_config.max_preview_video_w = Width;
		cam_config.max_preview_video_h = Height;
		cam_config.num_preview_video_frames = 3;
		cam_config.stills_capture_circular_buffer_height = 0;
		cam_config.fast_preview_resume = 0;
		cam_config.use_stc_timestamp = MMAL_PARAM_TIMESTAMP_MODE_RESET_STC;
		mmal_port_parameter_set(camera->control, &cam_config.hdr);
	}

	// setup preview port format - QUESTION: Needed if we aren't using preview?
	format = preview_port->format;
	format->encoding = MMAL_ENCODING_OPAQUE;
	format->encoding_variant = MMAL_ENCODING_I420;
	format->es->video.width = Width;
	format->es->video.height = Height;
	format->es->video.crop.x = 0;
	format->es->video.crop.y = 0;
	format->es->video.crop.width = Width;
	format->es->video.crop.height = Height;
	format->es->video.frame_rate.num = 30;
	format->es->video.frame_rate.den = 1;
	status = mmal_port_format_commit(preview_port);
	if (status != MMAL_SUCCESS)
	{
		printf("Couldn't set preview port format : error %d", status);
		mmal_component_destroy(camera);
		return false;
	}

	//setup video port format
	format = video_port->format;
	format->encoding = MMAL_ENCODING_I420;
	format->encoding_variant = MMAL_ENCODING_I420; //not opaque, as we want to read it!
	format->es->video.width = Width;
	format->es->video.height = Height;
	format->es->video.crop.x = 0;
	format->es->video.crop.y = 0;
	format->es->video.crop.width = Width;
	format->es->video.crop.height = Height;
	format->es->video.frame_rate.num = 30;
	format->es->video.frame_rate.den = 1;
	status = mmal_port_format_commit(video_port);
	if (status != MMAL_SUCCESS)
	{
		printf("Couldn't set video port format : error %d", status);
		mmal_component_destroy(camera);
		return false;
	}

	//setup still port format
	format = still_port->format;

    // Set our stills format on the stills  port
    if (true)
    {
        format->encoding = MMAL_ENCODING_BGR24;
        format->encoding_variant = MMAL_ENCODING_BGR24;
    }
    else
    {
        format->encoding = MMAL_ENCODING_I420;
        format->encoding_variant = MMAL_ENCODING_I420;
    }

    
    format->es->video.width = VCOS_ALIGN_UP(Width, 32); //Width;
	format->es->video.height = VCOS_ALIGN_UP(Height, 16); //Height;
	format->es->video.crop.x = 0;
	format->es->video.crop.y = 0;
	format->es->video.crop.width = Width;
	format->es->video.crop.height = Height;
	format->es->video.frame_rate.num = 3;
	format->es->video.frame_rate.den = 1;
	status = mmal_port_format_commit(still_port);
	if (status != MMAL_SUCCESS)
	{
		printf("Couldn't set still port format : error %d", status);
		mmal_component_destroy(camera);
		return false;
	}
    
    //setup still port buffer
    if (still_port->buffer_size < still_port->buffer_size_min)
    still_port->buffer_size = still_port->buffer_size_min;
    
    still_port->buffer_num = still_port->buffer_num_recommended;
    
    status = mmal_port_format_commit(still_port);
    
    if (status)
    {
        printf("Couldn't create still buffer\n");
        mmal_component_destroy(camera);
        return false;
        
    }
    
    
	//enable the camera
	status = mmal_component_enable(camera);
	if (status != MMAL_SUCCESS)
	{
		printf("Couldn't enable camera\n");
		mmal_component_destroy(camera);
		return false;	
	}

    
    /* Create pool of buffer headers for the output port to consume */
    MMAL_POOL_T* still_buffer_pool;
    still_buffer_pool = mmal_port_pool_create(still_port, still_port->buffer_num, still_port->buffer_size);
    
    if (!still_buffer_pool)
    {
        printf("Couldn't create still pool\n");
        mmal_component_destroy(camera);
        return false;
        
    }
    
	//apply all camera parameters
	raspicamcontrol_set_all_parameters(camera, &CameraParameters);
	//store created info
    CameraComponent = camera;
    Port = still_port;
    BufferPool = still_buffer_pool;
    
    //return success
    printf("Camera successfully created\n");

    return true;
}

bool CCamera::Shoot(CameraCBFunction callback, void* data) {

	Callback = callback;
	ShootData = data;

	if (Port->is_enabled) {
		printf("Disable: mmal_port_disable\n");
		mmal_port_disable(CameraComponent->output[MMAL_CAMERA_CAPTURE_PORT]);
	}


	//send all the buffers in our pool to the still port ready for use
	{
		if (mmal_port_enable(Port, StillBufferCallback) != MMAL_SUCCESS)
		{
			printf("Failed to set still buffer callback\n");
			mmal_port_pool_destroy(Port,BufferPool);
			mmal_component_destroy(CameraComponent);
			return false;
		}

		int num = mmal_queue_length(BufferPool->queue);
		printf("num buffers=%d\n", num);
		int q;
		for (q=0;q<num;q++)
		{
			MMAL_BUFFER_HEADER_T *buffer = mmal_queue_get(BufferPool->queue);
			if (!buffer)
				printf("Unable to get a required buffer %d from pool queue", q);
			if (mmal_port_send_buffer(Port, buffer)!= MMAL_SUCCESS)
				printf("Unable to send a buffer to encoder output port (%d)", q);
			printf("Sent buffer %d to still port\n");
		}
	}

	//begin capture
	if (mmal_port_parameter_set_boolean(Port, MMAL_PARAMETER_CAPTURE, 1) != MMAL_SUCCESS)
	{
		printf("Failed to start capture\n");
		return false;	
	}

	return true;
}

void CCamera::Release()
{
	if (CameraComponent != NULL) {

		printf("Disable: mmal_port_disable\n");
		mmal_port_disable(CameraComponent->output[MMAL_CAMERA_CAPTURE_PORT]);
		printf("Release: mmal_component_disable\n");
		mmal_component_disable(CameraComponent);
		sleep(2);
		printf("Release: mmal_port_pool_destroy\n");
		mmal_port_pool_destroy(CameraComponent->output[MMAL_CAMERA_CAPTURE_PORT],BufferPool);
		sleep(2);
		printf("Release: mal_component_destroy\n");
		mmal_component_destroy(CameraComponent);
		sleep(2);
		printf("Release: ok\n");
		CameraComponent = NULL;
	}
}

void CCamera::OnCameraControlCallback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
	printf("OnCameraControlCallback:\n");
}

void CCamera::OnStillBufferCallback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
	//check if buffer has data in
	if(Callback != NULL)
	{
		//got data so lock the buffer, call the callback so the application can use it, then unlock
		mmal_buffer_header_mem_lock(buffer);
        
		Callback(ShootData, buffer->data, buffer->length);
		
        mmal_buffer_header_mem_unlock(buffer);

        Callback = NULL;
	}
	
	// release buffer back to the pool
	mmal_buffer_header_release(buffer);

	// and send one back to the port (if still open)
	if (port->is_enabled)
	{
		MMAL_STATUS_T status;
		MMAL_BUFFER_HEADER_T *new_buffer;
		new_buffer = mmal_queue_get(BufferPool->queue);
		if (new_buffer) {
			status = mmal_port_send_buffer(port, new_buffer);
			if (status != MMAL_SUCCESS) {
				printf("OnStillBufferCallback: status = %d\n", (int)status);
			}
		} else {
			printf("OnStillBufferCallback: no buffer\n");
		}
	} else {
		printf("OnStillBufferCallback: port is not enabled\n");
	}
}

