#pragma once

#include <string>
#include <fstream>
#include <vector>
#include <iostream>
#include <memory>
#include <map>


struct vec3
{
	vec3() :x(0), y(0), z(0){}
	vec3(uint32_t X, uint32_t Y, uint32_t Z) : x(X), y(Y), z(Z) {}

	uint32_t x, y, z;
};

struct vec4
{
	vec4() :x(0), y(0), z(0), w(0) {}
	vec4(uint32_t X, uint32_t Y, uint32_t Z, uint32_t W) : x(X), y(Y), z(Z), w(W){}

	uint32_t x, y, z, w;
};


struct Voxel
{
	Voxel() = default;
	Voxel(vec3 pos, uint16_t ind) : m_Position(pos), m_Index(ind) {}

	vec3 m_Position;
	uint16_t m_Index;
};

struct Material
{
	vec4 m_Colour;
	vec3 m_Protperites;
};


struct VoxelContainer
{
	std::vector<Voxel> m_Voxels;
	std::vector<Material> m_Palette;

	uint8_t m_NumColourChannels;
	uint8_t m_BitsPerChannel;

	vec3 m_FrameSize;
};


//Removes the unused elements from the palette
void OptimisePalette(std::unique_ptr<VoxelContainer> &Container)
{
	std::map<uint16_t, std::vector<uint16_t*>> indexMap;
	for (int i = 0; i < Container->m_Voxels.size(); ++i)
	{
		indexMap[Container->m_Voxels[i].m_Index].push_back(&Container->m_Voxels[i].m_Index);
	}

	std::vector<Material> newPalette;
	uint16_t matCounter = 0;
	for (auto it = indexMap.begin(); it != indexMap.end(); ++it)
	{
		newPalette.push_back(Container->m_Palette[it->first]);

		for (int i = 0; i < it->second.size(); ++i)
		{
			*it->second[i] = matCounter;
		}

		matCounter++;
	}

	Container->m_Palette = newPalette;
}



std::unique_ptr<VoxelContainer> ConvertXRAW(std::string filePath)
{
	//File Data//
	std::ifstream MyFile;
	std::streampos size;
	char* data;
	uint32_t offset = 0;

	MyFile.open(filePath, std::ios::binary | std::ios::ate);
	if (MyFile.is_open())
	{
		size = MyFile.tellg();
		data = new char[size];

		MyFile.seekg(0, std::ios::beg);
		MyFile.read(data, size);
		MyFile.close();
	}
	else
		return std::unique_ptr<VoxelContainer>(nullptr);

	//Header Data//
	char identifier[4];
	int8_t colourChannelType;
	int8_t channelNum;
	int8_t bitsPerChannel;
	int8_t bitsPerIndex;

	//get header identifier
	identifier[0] = *reinterpret_cast<int8_t*>(data + offset);
	identifier[1] = *reinterpret_cast<int8_t*>(data + offset + 1);
	identifier[2] = *reinterpret_cast<int8_t*>(data + offset + 2);
	identifier[3] = *reinterpret_cast<int8_t*>(data + offset + 3);
	offset += 4;

	colourChannelType = *reinterpret_cast<int8_t*>(data + offset);
	offset += 1;

	channelNum = *reinterpret_cast<int8_t*>(data + offset);
	offset += 1;

	bitsPerChannel = *reinterpret_cast<int8_t*>(data + offset);
	offset += 1;

	bitsPerIndex = *reinterpret_cast<int8_t*>(data + offset);
	offset += 1;




	//Voxel Container Size
	int32_t width;
	int32_t height;
	int32_t depth;

	int32_t paletteSize;


	width = *reinterpret_cast<int32_t*>(data + offset);
	offset += 4;

	height = *reinterpret_cast<int32_t*>(data + offset);
	offset += 4;

	depth = *reinterpret_cast<int32_t*>(data + offset);
	offset += 4;

	paletteSize = *reinterpret_cast<int32_t*>(data + offset);
	offset += 4;


	//----------------------------------------
	//END OF HEADER
	//----------------------------------------

	//create new voxel container;
	std::unique_ptr<VoxelContainer> Container(new VoxelContainer);


	//Voxel Data
	//std::vector<vec3> VoxelPositions;
	//std::vector<uint16_t> VoxelIndecies;


	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			for (int z = 0; z < depth; ++z)
			{
				uint16_t index;
				switch (bitsPerIndex)
				{
				case 8:
				{
					uint32_t address = (x + y * width + z * (width*height));
					index = *reinterpret_cast<uint8_t*>(data + offset + address);

				}
					break;

				case 16:
				{
					uint32_t address = 16 * (x + y * width + z * (width*height));
					index = *reinterpret_cast<uint16_t*>(data + offset + address);
				}
					break;

				case 0:
					std::cout << "This File does not have a palette. \n";
					break;

				}


				//if the index is zero, this voxel is empty and we dont need it
				if (index == 0)
					continue;

				//Add voxel to list
				Container->m_Voxels.push_back(Voxel(vec3(x, y, z), index));

			}
		}
	}
	//move the offset past the voxel index data
	offset += width * depth * height * (bitsPerIndex/8);

	std::vector<uint8_t> colourBuffer;
	for (int colour = 0; colour < paletteSize; ++colour)
	{
		for (int channel = 0; channel < channelNum; ++channel)
		{
			switch (bitsPerChannel)
			{
			case 8:
				colourBuffer.push_back(*reinterpret_cast<uint8_t*>(data + offset));
				offset += 1;
				break;

			case 16:
				colourBuffer.push_back(*reinterpret_cast<uint16_t*>(data + offset));
				offset += 2;
				break;

			case 32:
				colourBuffer.push_back(*reinterpret_cast<uint32_t*>(data + offset));
				offset += 4;
				break;
			}
		}
	}

	for (int i = 0; i < colourBuffer.size();)
	{
		Material mat;

		switch (channelNum)
		{
		case 4:
			mat.m_Colour = vec4(colourBuffer[i], colourBuffer[i + 1], colourBuffer[i + 2], colourBuffer[i + 3]);
			i += 4;
			break;
		case 3:
			mat.m_Colour = vec4(colourBuffer[i], colourBuffer[i + 1], colourBuffer[i + 2] , 0);
			i += 3;
			break;
		case 2:
			mat.m_Colour = vec4(colourBuffer[i], colourBuffer[i + 1], 0, 0);
			i += 2;
			break;
		case 1:
			mat.m_Colour = vec4(colourBuffer[i], 0, 0 ,0);
			i += 1;
			break;
		}

		Container->m_Palette.push_back(mat);
	}


	Container->m_BitsPerChannel = bitsPerChannel;
	Container->m_NumColourChannels = channelNum;

	OptimisePalette(Container);

	Container->m_FrameSize.x = width;
	Container->m_FrameSize.y = height;
	Container->m_FrameSize.z = depth;
	
	return Container;
}

//Convert data to custom O-VOX file
bool WriteOVOX(std::string filePath, std::unique_ptr<VoxelContainer> Container)
{
	std::ofstream File;
	File.open(filePath, std::ios::out | std::ios::binary);

	//------HEADER--------//
	//Write File Type (4 Bytes)
	char header[4] = { 'O','V','O','X' };
	File.write(header, 4);

	//Write number of colour channels(1 Byte)
	File.write((char*)&Container->m_NumColourChannels, 1);

	//Write bits per channel
	File.write((char*)&Container->m_BitsPerChannel, 1);

	//Frame Size
	File.write((char*)&Container->m_FrameSize.x, 12);

	//voxel Size
	size_t vSize = (uint32_t)Container->m_Voxels.size();
	File.write((char*)&vSize, 4);

	//Palette Size
	size_t pSize = (uint32_t)Container->m_Palette.size();
	File.write((char*)&pSize, 4);

	
	//--------DATA--------//

	//--Voxel Buffer--//
	//
	//Loop through each variavble in the stuct to avoid trasmitting 
	//garbage bytes used for alignment
	for (int i = 0; i < Container->m_Voxels.size(); ++i)
	{
		File.write((char*)&Container->m_Voxels[i].m_Position.x, 4);
		File.write((char*)&Container->m_Voxels[i].m_Position.y, 4);
		File.write((char*)&Container->m_Voxels[i].m_Position.z, 4);

		File.write((char*)&Container->m_Voxels[i].m_Index, 2);
	}



	//Palette Buffer
	for (int i = 0; i < Container->m_Palette.size(); ++i)
	{

		File.write((char*)&Container->m_Palette[i].m_Colour.x, 4);
		File.write((char*)&Container->m_Palette[i].m_Colour.y, 4);
		File.write((char*)&Container->m_Palette[i].m_Colour.z, 4);
		File.write((char*)&Container->m_Palette[i].m_Colour.w, 4);

		File.write((char*)&Container->m_Palette[i].m_Protperites.x, 4);
		File.write((char*)&Container->m_Palette[i].m_Protperites.y, 4);
		File.write((char*)&Container->m_Palette[i].m_Protperites.z, 4);
	}


	File.close();
	return true;
}