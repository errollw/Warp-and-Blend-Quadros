#include <stdio.h>
#include <stdlib.h>
#include "stdafx.h"

#include <fstream>

#include <windows.h>
#include <assert.h>
#include "include/nvapi.h"

#include <string>
#include <regex>

#include "getCoords.h"

#include <map>

#include <utility>

#include "lodepng.h"

using namespace std;

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

	// for loading blending images
	std::vector<unsigned char> image;
	unsigned width, height;

	printf("App Version: 1.2\n");

	// Initialize NVAPI, get GPU handles, etc.
	error = NvAPI_Initialize();
	ZeroMemory(&nvGPUHandles , sizeof(nvGPUHandles));
	error = NvAPI_EnumPhysicalGPUs(nvGPUHandles, &gpuCount);	

	// a mapping from displayId to (row,column) location of projector
	std::map<int,pair<int,int>> locationsOfDisplay;
	locationsOfDisplay[0x80061082] = make_pair(0,0);
	locationsOfDisplay[0x80061081] = make_pair(0,1);
	locationsOfDisplay[0x80061080] = make_pair(0,2);
	locationsOfDisplay[0x82061082] = make_pair(1,0);
	locationsOfDisplay[0x82061081] = make_pair(1,1);
	locationsOfDisplay[0x82061080] = make_pair(1,2);

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

			// Query the desktop size and display location in it
			error = NvAPI_GPU_GetScanoutConfigurationEx(dispIds[dispIndex].displayId, &scanInfo);
			if(error != NVAPI_OK)
			{
				NvAPI_GetErrorMessage(error, estring);
				printf("NvAPI_GPU_GetScanoutConfiguration: %s\n", estring);
			}
			
			// Desktop -- the size & location of the virtual desktop in Windows, in a Mosaic this will include all displays and overalp
			printf("DesktopRect: sX = %6d, sY = %6d, sWidth = %6d sHeight = %6d\n", scanInfo.sourceDesktopRect.sX, scanInfo.sourceDesktopRect.sY, scanInfo.sourceDesktopRect.sWidth, scanInfo.sourceDesktopRect.sHeight);

			// source Viewport -- where in the desktop this selected display is
			printf("ViewportRect: sX = %6d, sY = %6d, sWidth = %6d sHeight = %6d\n", scanInfo.sourceViewportRect.sX, scanInfo.sourceViewportRect.sY, scanInfo.sourceViewportRect.sWidth, scanInfo.sourceViewportRect.sHeight);

			// What resolution is the display 
			printf("Display Scanout Rect: sX = %6d, sY = %6d, sWidth = %6d sHeight = %6d\n", scanInfo.targetViewportRect.sX, scanInfo.targetViewportRect.sY, scanInfo.targetViewportRect.sWidth, scanInfo.targetViewportRect.sHeight);

			// texture coordinates
			// Computing the texture coordinates is different if we are in Mosaic vs extended displays, so check to see if Mosaic is running
			NV_MOSAIC_TOPO_BRIEF  topo;
			topo.version = NVAPI_MOSAIC_TOPO_BRIEF_VER ;

			NV_MOSAIC_DISPLAY_SETTING dispSetting;
			dispSetting.version = NVAPI_MOSAIC_DISPLAY_SETTING_VER ;

			NvS32 overlapX, overlapY;
			float srcLeft, srcTop, srcWidth, srcHeight ;

			// Query the current Mosaic topology
			error = NvAPI_Mosaic_GetCurrentTopo(&topo, &dispSetting, &overlapX, &overlapY);
			if(error != NVAPI_OK)
			{
				NvAPI_GetErrorMessage(error, estring);
				printf("NvAPI_GPU_GetCurrentTopo: %s\n", estring);
			}

			if(topo.enabled == false)
			{
				// Extended mode
				// warp texture coordinates are defined in desktopRect coordinates
				srcLeft   = (float)scanInfo.sourceDesktopRect.sX;
				srcTop    = (float)scanInfo.sourceDesktopRect.sY;
				srcWidth  = (float)scanInfo.sourceDesktopRect.sWidth;
				srcHeight = (float)scanInfo.sourceDesktopRect.sHeight;	
			}
			else
			{
				// Mosaic -- we only want the pixels under the physical display here
				printf("Mosaic is enabled\n");
				srcLeft   = (float)scanInfo.sourceViewportRect.sX;
				srcTop    = (float)scanInfo.sourceViewportRect.sY;
				srcWidth  = (float)scanInfo.sourceViewportRect.sWidth;
				srcHeight = (float)scanInfo.sourceViewportRect.sHeight;	
			}

			// -----------------------------------------------------------------------------
			// WARPING
			// -----------------------------------------------------------------------------

			int row = locationsOfDisplay[dispIds[dispIndex].displayId].first;
			int col = locationsOfDisplay[dispIds[dispIndex].displayId].second;

			printf("Warping projector at ROW: %d, COL: %d\n", row, col);

			std::vector<float> vertices = get_warping_vertices(row,col,srcLeft,srcTop,srcWidth,srcHeight);
			int maxnumvert = 4;

			printf("vertices: %4.2f, %4.2f, %4.2f, %4.2f, %4.2f, %4.2f\n",vertices[0],vertices[1],vertices[2],vertices[3],vertices[4],vertices[5]);
			printf("vertices: %4.2f, %4.2f, %4.2f, %4.2f, %4.2f, %4.2f\n",vertices[6],vertices[7],vertices[8],vertices[9],vertices[10],vertices[11]);
			printf("vertices: %4.2f, %4.2f, %4.2f, %4.2f, %4.2f, %4.2f\n",vertices[12],vertices[13],vertices[14],vertices[15],vertices[16],vertices[17]);
			printf("vertices: %4.2f, %4.2f, %4.2f, %4.2f, %4.2f, %4.2f\n",vertices[18],vertices[19],vertices[20],vertices[21],vertices[22],vertices[23]);

			warpingData.version =  NV_SCANOUT_WARPING_VER; 
			warpingData.numVertices = maxnumvert;
			warpingData.vertexFormat = NV_GPU_WARPING_VERTICE_FORMAT_TRIANGLESTRIP_XYUVRQ;
			warpingData.textureRect = &scanInfo.sourceDesktopRect; 
			warpingData.vertices = &vertices[0];

			 //This call does the Warp
			error = NvAPI_GPU_SetScanoutWarping(dispIds[dispIndex].displayId, &warpingData, &maxNumVertices, &sticky);
			if (error != NVAPI_OK)  
			{ 
				NvAPI_GetErrorMessage(error, estring);
				printf("NvAPI_GPU_SetScanoutWarping: %s\n", estring);
			}

			// -----------------------------------------------------------------------------
			// BLENDING
			// -----------------------------------------------------------------------------

			NV_SCANOUT_INTENSITY_DATA intensityData; 

			image.clear();
			string blend_filename = "blend_(" + to_string(row) + ", " + to_string(col) + ").png";
			unsigned lodePng_error = lodepng::decode(image, width, height, blend_filename);
            if(lodePng_error) printf("LODEPNG ERROR %s\n ", lodepng_error_text(lodePng_error));

			// copy values from png, skip every 4th value - we don't want the alpha channel
			std::vector<float> intensityTexture_vec;
			for(int i=0; i<image.size(); i++){
				if ((i+1)%4 != 0){
					intensityTexture_vec.push_back(image[i] / 255.0f);
				}
			}

			float* intensityTexture = &intensityTexture_vec[0];

			intensityData.version           = NV_SCANOUT_INTENSITY_DATA_VER;
			intensityData.width             = 1920;
			intensityData.height            = 1080;
			intensityData.blendingTexture   = intensityTexture;

			// do not want to use an offset texture
			intensityData.offsetTexture		= NULL;
			intensityData.offsetTexChannels = 1;

			// this call does the intensity map
			error =  NvAPI_GPU_SetScanoutIntensity(dispIds[dispIndex].displayId, &intensityData, &sticky);

			if (error != NVAPI_OK)  
			{
				NvAPI_GetErrorMessage(error, estring);
				printf("NvAPI_GPU_SetScanoutIntensity: %s\n", estring);
			} 

		} //end of for displays
		
		delete [] dispIds;

	} //end of loop gpus

}



	 