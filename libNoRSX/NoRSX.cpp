/*
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 This program was created by Grazioli Giovanni Dante <wargio@libero.it>.

*/

#include <NoRSX.h>
#include <sys/process.h>

/* Allocate 1MB stack to avoid overflows */
SYS_PROCESS_PARAM(1001, 0x100000);

static int already_done=0;

NoRSX::NoRSX() : EventHandler(){
	currentBuffer = 0;
	buffer_type = screen_type = 0;
	host_addr = memalign(1024*1024, HOST_SIZE);
	getResolution(&width,&height);
	context = initScreen(host_addr, HOST_SIZE,0, width, height);
	for(int i=0;i<2;i++)
		makeBuffer(&buffers[i],width,height,i);
	buffer = makeMemBuffer(width,height,&buffer_size);
	flip(context, 0);
	setRenderTarget(context, &buffers[currentBuffer]);
	RegisterCallBack(EVENT_SLOT0);
}

NoRSX::NoRSX(int id_type) : EventHandler(){
	buffer_type = screen_type = id_type;
	currentBuffer = 0;
	host_addr = memalign(1024*1024, HOST_SIZE);
	
	switch(id_type){
		case RESOLUTION_1920x1080: {
			width=1920;height=1080;
			buffers[0].width=1920;buffers[0].height=1080;
			buffers[1].width=1920;buffers[1].height=1080;
		} break;
		case RESOLUTION_1280x720: {
			width=1280;height=720;
			buffers[0].width=1280;buffers[0].height=720;
			buffers[1].width=1280;buffers[1].height=720;
		} break;
		case RESOLUTION_720x576: {
			width=720;height=576;
			buffers[0].width=720;buffers[0].height=576;
			buffers[1].width=720;buffers[1].height=576;
		} break;
		case RESOLUTION_720x480: {
			width=720;height=480;
			buffers[0].width=720;buffers[0].height=480;
			buffers[1].width=720;buffers[1].height=480;
		} break;
		default:
			getResolution(&width,&height);
			buffers[0].width=width;buffers[0].height=height;
			buffers[1].width=width;buffers[1].height=height;
		  break;
	}
	context = initScreen(host_addr, HOST_SIZE, id_type, width, height);

	for(int i=0;i<2;i++)
		makeBuffer(&buffers[i],width,height,i);
		
	buffer = makeMemBuffer(width,height,&buffer_size);
	flip(context, 0);
	setRenderTarget(context, &buffers[0]);
	RegisterCallBack(EVENT_SLOT0);
}

NoRSX::NoRSX(int real_screen_type, int buffer_screen_type) : EventHandler(){
	screen_type = real_screen_type;
	buffer_type = buffer_screen_type;

	currentBuffer = 0;
	host_addr = memalign(1024*1024, HOST_SIZE);
	
	switch(real_screen_type){
		case RESOLUTION_1920x1080: {
			width=1920;height=1080;
			buffers[0].width=1920;buffers[0].height=1080;
			buffers[1].width=1920;buffers[1].height=1080;
		} break;
		case RESOLUTION_1280x720: {
			width=1280;height=720;
			buffers[0].width=1280;buffers[0].height=720;
			buffers[1].width=1280;buffers[1].height=720;
		} break;
		case RESOLUTION_720x576: {
			width=720;height=576;
			buffers[0].width=720;buffers[0].height=576;
			buffers[1].width=720;buffers[1].height=576;
		} break;
		case RESOLUTION_720x480: {
			width=720;height=480;
			buffers[0].width=720;buffers[0].height=480;
			buffers[1].width=720;buffers[1].height=480;
		} break;
		default:
			getResolution(&width,&height);
			buffers[0].width=width;buffers[0].height=height;
			buffers[1].width=width;buffers[1].height=height;
		  break;
	}
	context = initScreen(host_addr, HOST_SIZE, real_screen_type, width, height);

	for(int i=0;i<2;i++)
		makeBuffer(&buffers[i],width,height,i);
	
	switch(buffer_screen_type){
		case RESOLUTION_1920x1080:
			width=1920; height=1080;
			break;
		case RESOLUTION_1280x720:
			width=1280; height=720;
			break;
		case RESOLUTION_720x576:
			width=720; height=576;
			break;
		case RESOLUTION_720x480:
			width=720; height=480;
			break;
		default:
			getResolution(&width,&height);
			break;
	}	
	
	buffer = makeMemBuffer(width,height,&buffer_size);
	buffer_size = buffers[0].width * buffers[0].height * sizeof(u32);
	
	
	flip(context, 0);
	setRenderTarget(context, &buffers[0]);
	RegisterCallBack(EVENT_SLOT0);
}

NoRSX::~NoRSX(){
	if(already_done!=0)
		NoRSX_Exit();
}

void NoRSX::RescaleFlip(){
	waitFlip();
	ResizeBuffer();
	flip(context, currentBuffer);
	currentBuffer = !currentBuffer;
	setRenderTarget(context, &buffers[currentBuffer]);
	sysUtilCheckCallback();
	return;
}

void NoRSX::Flip(){
	waitFlip();
	memcpy(buffers[currentBuffer].ptr, buffer, buffer_size);
	flip(context, currentBuffer);
	currentBuffer = !currentBuffer;
	setRenderTarget(context, &buffers[currentBuffer]);
	sysUtilCheckCallback();
	return;
}

void NoRSX::NoRSX_Exit(){
	gcmSetWaitFlip(context);
	for (int i=0;i<2;i++)
		rsxFree (buffers[i].ptr);
	rsxFinish (context, 1);
	free (host_addr);
	free (buffer);
	UnregisterCallBack(EVENT_SLOT0);
	already_done=1;
	return;
}

void NoRSX::ScaleLine(u32 *Target, u32 *Source, u32 SrcWidth, u32 TgtWidth){
 //Thanks to: http://www.compuphase.com/graphic/scale.htm
	int NumPixels = TgtWidth;
	int IntPart = SrcWidth / TgtWidth;
	int FractPart = SrcWidth % TgtWidth;
	int E = 0;

	while (NumPixels-- > 0) {
		*Target++ = *Source;
		Source += IntPart;
		E += FractPart;
		if (E >= (int)TgtWidth) {
			E -= TgtWidth;
			Source++;
		} /* if */
	} /* while */
	return;
}

void NoRSX::ResizeBuffer(){
	u32 TgtWidth  = buffers[0].width;
	u32 TgtHeight = buffers[0].height;
	u32 *Target = buffers[currentBuffer].ptr;
	u32 *Source = buffer;
	if(TgtWidth == width && TgtHeight == height){
		memcpy(Target, Source, TgtHeight*TgtWidth*sizeof(u32));
		return;
	}
	int NumPixels = TgtHeight;
	int IntPart = (height / TgtHeight) * width;
	int FractPart = height % TgtHeight;
	int E = 0;
	u32 *PrevSource = NULL;

	while (NumPixels-- > 0) {
		if (Source == PrevSource) {
			memcpy(Target, Target-TgtWidth, TgtWidth*sizeof(*Target));
		} else {
			ScaleLine(Target, Source, width, TgtWidth);
			PrevSource = Source;
		}
		Target += TgtWidth;
		Source += IntPart;
		E += FractPart;
		if (E >= (int)TgtHeight) {
			E -= TgtHeight;
			Source += width;
		}
	}
	return;
}

