#include "Engine/Rendering/Resources/VoxelTexture.hpp"
#include "Engine/Core/File.hpp"
#include "Engine/Core/Rgba.hpp"
#include "Engine/Math/MathUtils.hpp"
VoxelTexture::VoxelTexture()
{
}

VoxelTexture::~VoxelTexture()
{
	if (m_colorData != nullptr)
	{
		free(m_colorData);
		m_colorData = nullptr;
	}

	if (m_collisionFlags != nullptr)
	{
		free(m_collisionFlags);
		m_collisionFlags = nullptr;
	}
}

bool VoxelTexture::CreateFromFile(const char* filename)
{
	File* file = new File();
	bool opened = file->Open(filename, "r");

	if (!opened)
	{
		return false;
	}

	file->LoadFileToMemory();

	std::string currLine;

	// "Qubicle Exchange Format"
	file->GetNextLine(currLine);

	if (currLine != "Qubicle Exchange Format")
	{
		ERROR_RECOVERABLE(Stringf("Error: VoxelTexture::CreateFromFile() only supports QEF files"));
		return false;
	}

	// Version number
	file->GetNextLine(currLine);

	// Website
	file->GetNextLine(currLine);

	// Dimensions
	file->GetNextLine(currLine);
	
	if (!SetFromText(currLine, m_dimensions))
	{
		ERROR_RECOVERABLE(Stringf("Error: VoxelTexture::CreateFromFile() couldn't get the dimensions of the texture."));
		return false;
	}

	// Number of colors
	file->GetNextLine(currLine);

	int numColors;
	SetFromText(currLine, numColors);

	Rgba* colorPallette = (Rgba*)malloc(sizeof(Rgba) * numColors);
	memset(colorPallette, 0, sizeof(Rgba) * numColors);

	// Get the colors
	for (int i = 0; i < numColors; ++i)
	{
		file->GetNextLine(currLine);
		SetFromText(currLine, colorPallette[i]);
	}

	// Set up the texture colors
	if (m_colorData != nullptr)
	{
		free(m_colorData);
	}

	unsigned int voxelCount = m_dimensions.x * m_dimensions.y * m_dimensions.z;
	m_colorData = (Rgba*)malloc(sizeof(Rgba) * voxelCount);
	memset(m_colorData, 0, sizeof(Rgba) * voxelCount);

	// Set up the collision flags as well
	int flagCount = Ceiling((float)voxelCount / 8.f);
	m_collisionFlags = (uint8_t*)malloc(flagCount);
	memset(m_collisionFlags, 0, flagCount);

	// Now get all the voxel colors
	while (!file->IsAtEndOfFile())
	{
		file->GetNextLine(currLine);

		if (currLine.size() == 0)
		{
			break;
		}

		std::vector<std::string> voxelTokens = Tokenize(currLine, ' ');

		// Get the voxel coords
		int xCoord = StringToInt(voxelTokens[0]);

		// *Flip from right handed to left handed basis
		xCoord = m_dimensions.x - xCoord - 1;

		int yCoord = StringToInt(voxelTokens[1]);
		int zCoord = StringToInt(voxelTokens[2]);

		int index = yCoord * (m_dimensions.x * m_dimensions.z) + zCoord * m_dimensions.x + xCoord;

		int colorIndex = StringToInt(voxelTokens[3]);

		m_colorData[index] = colorPallette[colorIndex];

		// Set the collision flag as well
		if (m_colorData[index].a != 0)
		{
			int byteIndex = index / 8;
			int bitOffset = 7 - (index % 8);

			uint8_t& collisionByte = m_collisionFlags[byteIndex];
			collisionByte |= (1 << bitOffset);
		}
	}

	// Done!
	free(colorPallette);

	return true;
}

bool VoxelTexture::CreateFromColorStream(const Rgba* colors, const IntVector3& dimensions)
{
	m_dimensions = dimensions;

	if (m_colorData != nullptr)
	{
		free(m_colorData);
	}

	int numVoxels = dimensions.x * dimensions.y * dimensions.z;

	m_colorData = (Rgba*)malloc(sizeof(Rgba) * numVoxels);
	memcpy(m_colorData, colors, numVoxels * sizeof(Rgba));

	int voxelCount = m_dimensions.x * m_dimensions.y * m_dimensions.z;
	int flagCount = Ceiling((float)voxelCount / 8.f);
	m_collisionFlags = (uint8_t*)malloc(flagCount);

	for (int y = 0; y < m_dimensions.y; ++y)
	{
		for (int z = 0; z < m_dimensions.z; ++z)
		{
			for (int x = 0; x < m_dimensions.x; ++x)
			{
				Rgba color = GetColorAtCoords(IntVector3(x, y, z));

				if (color.a != 0)
				{
					int index = y * (m_dimensions.x * m_dimensions.z) + z * m_dimensions.x + x;
					int byteIndex = index / 8;
					int bitOffset = 7 - (index % 8);

					uint8_t& collisionByte = m_collisionFlags[byteIndex];
					collisionByte |= (1 << bitOffset);
				}
			}
		}
	}

	return true;
}

VoxelTexture* VoxelTexture::Clone() const
{
	VoxelTexture* newTexture = new VoxelTexture();
	newTexture->m_dimensions = m_dimensions;

	int voxelCount = m_dimensions.x * m_dimensions.y * m_dimensions.z;
	size_t byteSize = sizeof(Rgba) * voxelCount;
	newTexture->m_colorData = (Rgba*)malloc(byteSize);

	memcpy(newTexture->m_colorData, m_colorData, byteSize);


	// Collision
	int flagCount = Ceiling((float) voxelCount / 8.f);
	newTexture->m_collisionFlags = (uint8_t*)malloc(flagCount);
	memcpy(newTexture->m_collisionFlags, m_collisionFlags, flagCount);

	return newTexture;
}

void VoxelTexture::SetColorAtIndex(unsigned int index, const Rgba& color)
{
	m_colorData[index] = color;

	// Update the collision too
	int byteIndex = index / 8;
	int bitOffset = 7 - (index % 8);

	uint8_t& collisionByte = m_collisionFlags[byteIndex];

	uint8_t mask = (1 << bitOffset);

	if (color.a == 0)
	{
		mask = ~mask;
		collisionByte &= mask;
	}
	else
	{
		collisionByte |= mask;
	}
}

Rgba VoxelTexture::GetColorAtCoords(const IntVector3& coords) const
{
	int index = coords.y * (m_dimensions.x * m_dimensions.z) + coords.z * m_dimensions.x + coords.x;
	return m_colorData[index];
}

Rgba VoxelTexture::GetColorAtIndex(unsigned int index) const
{
	return m_colorData[index];
}

IntVector3 VoxelTexture::GetDimensions() const
{
	return m_dimensions;
}

unsigned int VoxelTexture::GetVoxelCount() const
{
	return m_dimensions.x * m_dimensions.y * m_dimensions.z;
}

uint8_t VoxelTexture::GetCollisionByteThatContainsCoords(const IntVector3& coords) const
{
	int index = coords.y * (m_dimensions.x * m_dimensions.z) + coords.z * m_dimensions.x + coords.x;
	index /= 8;
	return m_collisionFlags[index];
}

bool VoxelTexture::DoLocalCoordsHaveCollision(const IntVector3& coords) const
{
	int localIndex = coords.y * (m_dimensions.x * m_dimensions.z) + coords.z * m_dimensions.x + coords.x;
	
	int byteIndex = localIndex / 8;
	int bitOffset = 7 - (localIndex % 8);

	uint8_t flags = m_collisionFlags[byteIndex];

	bool hasCollision = ((flags & (1 << bitOffset)) != 0);
	return hasCollision;
}
