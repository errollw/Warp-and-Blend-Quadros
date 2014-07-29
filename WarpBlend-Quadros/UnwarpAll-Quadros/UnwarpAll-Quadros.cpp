
//
// File:           WarpBlendSample.cpp
// Description:    Sample implementation of Nvidia warp and blend API
// Target OS:      Windows 7 only
// Notes:          Requires R334+ NVAPI and driver
//

//Version 1.2 Changelog
//- Uses DisplayIds instead of outputIds (deprecated)
//- Removed mosaic calls to get gisplay handle. Replace with NvAPI_DISP_GetDisplayHandleFromDisplayId instead
//- Added NvAPI_GPU_GetScanoutConfigurationEx for the case when viewport rect!=display rect

#include <stdio.h>
#include <stdlib.h>
#include "stdafx.h"

#include <iostream>
#include <fstream>

#include <windows.h>
#include <assert.h>
#include "include/nvapi.h"


int main(int argc, char **argv)
{
	NvAPI_Status error;
	NvPhysicalGpuHandle nvGPUHandles[NVAPI_MAX_PHYSICAL_GPUS];
	NvU32 gpuCount = 0;
	NvU32 gpu;
	NvU32 outputMask = 0;

	NV_SCANOUT_WARPING_DATA warpingData;
	NvAPI_ShortString estring;
	int maxNumVertices = 0;
	int sticky = 0;

	std::ofstream logfile;
	logfile.open ("log.txt", std::ios::out);
	logfile << "Logfile created \n";
	logfile.close();

	printf("App Version: 1.2\n");

	// Initialize NVAPI, get GPU handles, etc.
	error = NvAPI_Initialize();
	ZeroMemory(&nvGPUHandles , sizeof(nvGPUHandles));
	error = NvAPI_EnumPhysicalGPUs(nvGPUHandles, &gpuCount);	

	// At this point we have a list of accessible physical nvidia gpus in the system.
	// Loop over all gpus

	for (gpu = 0; gpu < gpuCount; gpu++) 
	{
		NvU32 dispIdCount = 0;

		// Query the active physical display connected to each gpu.
		error =  NvAPI_GPU_GetConnectedDisplayIds(nvGPUHandles[gpu], NULL, &dispIdCount, 0);
		if((error != NVAPI_OK)||(dispIdCount ==0))
		{
			NvAPI_GetErrorMessage(error, estring);
			printf("NvAPI_GPU_GetConnectedDisplayIds: %s\n", estring);
			printf("Display count %d\n",dispIdCount);
			return error;
		}

		NV_GPU_DISPLAYIDS* dispIds = NULL;
		dispIds = new NV_GPU_DISPLAYIDS[dispIdCount];
		dispIds->version = NV_GPU_DISPLAYIDS_VER;
		
		error = NvAPI_GPU_GetConnectedDisplayIds(nvGPUHandles[gpu],dispIds,&dispIdCount,0);
		if(error != NVAPI_OK)
		{
			delete [] dispIds;
			NvAPI_GetErrorMessage(error, estring);
			printf("NvAPI_GPU_GetConnectedDisplayIds: %s\n", estring);
		}

		// Loop through all the displays
		for (NvU32 dispIndex = 0; (dispIndex < dispIdCount) && dispIds[dispIndex].isActive; dispIndex++) 
		{
			NV_SCANOUT_INFORMATION scanInfo;
			
			ZeroMemory(&scanInfo , sizeof(NV_SCANOUT_INFORMATION));
			scanInfo.version = NV_SCANOUT_INFORMATION_VER;
			
			printf("GPU %d, displayId 0x%08x\n",gpu,dispIds[dispIndex].displayId);

			warpingData.version =  NV_SCANOUT_WARPING_VER; 
			warpingData.vertexFormat = NV_GPU_WARPING_VERTICE_FORMAT_TRIANGLESTRIP_XYUVRQ;
			warpingData.textureRect = &scanInfo.sourceDesktopRect; 
			warpingData.vertices = NULL;
			warpingData.numVertices = 0;

			error = NvAPI_GPU_SetScanoutWarping(dispIds[dispIndex].displayId, &warpingData, &maxNumVertices, &sticky);

			if (error != NVAPI_OK)  
			{
				NvAPI_GetErrorMessage(error, estring);
				printf("NvAPI_GPU_SetScanoutWarping: %s\n", estring);
			}

			NV_SCANOUT_INTENSITY_DATA intensityData; 
			intensityData.blendingTexture = NULL;
 			 
			error =  NvAPI_GPU_SetScanoutIntensity(dispIds[dispIndex].displayId, &intensityData, &sticky);
			if (error != NVAPI_OK)  
			{
				NvAPI_GetErrorMessage(error, estring);
				printf("NvAPI_GPU_SetScanoutIntensity: %s\n", estring);
			} 
		} //end of for displays
		delete [] dispIds;
	}	//end of loop gpus

}



	 