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

#include <NoRSX/Spu.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>

#define SPU_SIZE(x)			(((x)+127) & ~127)

#define NORSX_SPU_LOADED		0
#define NORSX_SPU_NOT_LOADED		1

static u32 spu_init = SPU_NOT_STARTED;

Spu::Spu(){
	spu_init = SPU_NO_ERROR;

	if(spu_init == SPU_ALREADY_STARTED)
		error = SPU_ALREADY_STARTED;

	u32 ret = sysSpuInitialize(6, 5);

	if (ret == NORSX_OK)
		spu_init = SPU_STARTED;
	else
		error = SPU_ERROR_INIT;
}

Spu::~Spu(){
}

int Spu::GetSpuError() const{
	return error;
}

void Spu::SpuAttrubute(NoRSX_Spu *spus, const char *text){
	if(spus->loaded_attr != NORSX_SPU_LOADED){
		spus->attr.nameAddress = ptr2ea(text);
		spus->attr.nameSize    = strlen(text)+1;
		spus->attr.attribute   = SPU_THREAD_ATTR_NONE;
		spus->loaded_attr      = NORSX_SPU_LOADED;
	}
}

void Spu::SpuGroupAttribute(NoRSX_Spu *spus, const char* text, u32 group_type, u32 mem_container){
	if(spus->loaded_grp_attr != NORSX_SPU_LOADED){
		spus->group_attr.nameAddress  = ptr2ea(text);
		spus->group_attr.nameSize     = strlen(text)+1;
		spus->group_attr.groupType    = group_type;
		spus->group_attr.memContainer = mem_container;
		spus->group_num               = 1;
		spus->loaded_grp_attr         = NORSX_SPU_LOADED;
	}
}

void Spu::SpuArgs(NoRSX_Spu *spus, u64 arg1, u64 arg2, u64 arg3, u64 arg4){
	spus->args.arg0 = arg1;
	spus->args.arg1 = arg2;
	spus->args.arg2 = arg3;
	spus->args.arg3 = arg4;
}

void Spu::SpuThreadGroupCreate(NoRSX_Spu *spus, u32 priority){
	sysSpuThreadGroupCreate(&spus->group_id,
				1,
				priority,
				&spus->group_attr);
}

void Spu::SpuThreadInit(NoRSX_Spu *spus){
	sysSpuThreadInitialize(	&spus->thread_id,
				spus->group_id,
				0,
				&spus->image,
				&spus->attr,
				&spus->args);
	sysSpuThreadGroupStart(spus->group_id);
}

int Spu::LoadELF(const void *module, NoRSX_Spu *spus){
	sysSpuSegment *segment;

	size_t len;
	u32 entry, count;

	s32 ret = sysSpuRawCreate(&spus->spu_id, NULL);
	if(ret != NORSX_OK){
		error = SPU_CANNOT_CREATE_RAW;
		return ret;
	}
	sysSpuElfGetInformation(module, &entry, &count);
	len = sizeof(sysSpuSegment)*count;
	segment = (sysSpuSegment*) memalign(128, SPU_SIZE(len));
	memset(segment, 0, len);
	sysSpuElfGetSegments(module, segment, count);

	ret = sysSpuImageImport(&spus->image, module, 0);
	if (ret != NORSX_OK){
		error = SPU_CANNOT_IMPORT_IMAGE;
		goto Error;
	}

	ret = sysSpuRawImageLoad(spus->spu_id, &spus->image);
	if (ret != NORSX_OK){
		error = SPU_CANNOT_LOAD_IMAGE;
		goto Error;
	}

	return 0;

Error:
	if (segment)
		free(segment);

	return ret;
}

int Spu::LoadELF(const char *module_path, NoRSX_Spu *spus){
	s32 ret = sysSpuRawCreate(&spus->spu_id, NULL);
	if(ret != NORSX_OK){
		error = SPU_CANNOT_CREATE_RAW;
		return ret;
	}
	ret = sysSpuImageOpen(&spus->image, module_path);
	if (ret != NORSX_OK){
		error = SPU_CANNOT_OPEN_IMAGE;
		return ret;
	}

	ret = sysSpuRawImageLoad(spus->spu_id, &spus->image);

	if (ret != NORSX_OK){
		error = SPU_CANNOT_LOAD_IMAGE;
		return ret;
	}

	return 0;
}


void Spu::SpuCloseImage(NoRSX_Spu *spus){
	sysSpuThreadGroupJoin(	spus->group_id,
				&spus->cause,
				&spus->status);
	u32 ret = sysSpuRawDestroy(spus->spu_id);
	if (ret != NORSX_OK)
		error = SPU_CANNOT_DESTROY_RAW;

	ret = sysSpuImageClose(&spus->image);
	if (ret != NORSX_OK)
		error = SPU_CANNOT_CLOSE_IMAGE;
}
