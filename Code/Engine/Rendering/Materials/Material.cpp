/************************************************************************/
/* File: Material.cpp
/* Author: Andrew Chase
/* Date: April 23rd, 2018
/* Description: Implementation of the Material class
/************************************************************************/
#include "Engine/Assets/AssetDB.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Rendering/Core/Renderer.hpp"
#include "Engine/Rendering/Shaders/Shader.hpp"
#include "Engine/Rendering/Resources/Sampler.hpp"
#include "Engine/Rendering/Materials/Material.hpp"
#include "Engine/Rendering/Shaders/ShaderProgram.hpp"
#include "Engine/Core/DeveloperConsole/DevConsole.hpp"
#include "Engine/Rendering/Shaders/ShaderDescription.hpp"
#include "Engine/Rendering/Shaders/PropertyDescription.hpp"
#include "Engine/Rendering/Materials/MaterialPropertyBlock.hpp"
#include "Engine/Rendering/Shaders/PropertyBlockDescription.hpp"

#define TEXTURE_DIFFUSE_BIND (0)
#define TEXTURE_NORMAL_BIND (1)
#define TEXTURE_SPECULAR_BIND (2)
#define TEXTURE_EMISSIVE_BIND (3)

//-----------------------------------------------------------------------------------------------
// Constructor
//
Material::Material(const std::string& name)
	: Material()
{
	m_name = name;
}


//-----------------------------------------------------------------------------------------------
// Default Constructor
//
Material::Material()
	: m_isInstancedShader(false)
	, m_name("NO_NAME_SPECIFIED")
{
	for (int index = 0; index < MAX_TEXTURES_SAMPLERS; ++index)
	{
		m_textures[index] = nullptr;
		m_samplers[index] = nullptr;
	}
}


//-----------------------------------------------------------------------------------------------
// Destructor
//
Material::~Material()
{
	if (m_isInstancedShader)
	{
		delete m_shader;
	}

	// Delete property blocks
	int numBlocks = (int) m_propertyBlocks.size();
	for (int blockIndex = 0; blockIndex < numBlocks; ++blockIndex)
	{
		delete m_propertyBlocks[blockIndex];
	}

	m_propertyBlocks.clear();
}


//-----------------------------------------------------------------------------------------------
// Loads the material from an xml file given by filepath
// Returns true on success, false otherwise
//
bool Material::LoadFromFile(const std::string& filepath)
{
	// Load the document
	XMLDocument document;
	XMLError error = document.LoadFile(filepath.c_str());

	if (error != tinyxml2::XML_SUCCESS)
	{
		if (DevConsole::GetInstance() != nullptr)
		{
			ConsoleErrorf("Error: Couldn't load material file \"%s\"", filepath.c_str());
			DebuggerPrintf("Error: Couldn't load material file \"%s\"", filepath.c_str());
			return false;
		}
	}

	// I keep a root around just because I like having a single root element
	XMLElement* materialElement = document.RootElement();

	// Shader
	const XMLElement* shaderElement = materialElement->FirstChildElement("shader");

	if (shaderElement != nullptr)
	{
		std::string shaderName = ParseXmlAttribute(*shaderElement, "name", "Default_Opaque");
		m_shader = AssetDB::CreateOrGetShader(shaderName);
		m_isInstancedShader = false;	// Always construct materials with shared materials
	}

	// Textures
	const XMLElement* texturesElement = materialElement->FirstChildElement("textures");

	if (texturesElement != nullptr)
	{
		const XMLElement* currElement = texturesElement->FirstChildElement();

		while (currElement != nullptr)
		{
			std::string textureName = ParseXmlAttribute(*currElement, "name", "Invalid");
			bool generateMipMaps = ParseXmlAttribute(*currElement, "generateMipMaps", false);

			const Texture* texture = AssetDB::CreateOrGetTexture(textureName, generateMipMaps);
			int bindPoint = ParseXmlAttribute(*currElement, "bind", 0);

			m_textures[bindPoint] = texture;

			currElement = currElement->NextSiblingElement();
		}
	}


	// Samplers
	const XMLElement* samplersElement = materialElement->FirstChildElement("samplers");

	if (samplersElement != nullptr)
	{
		const XMLElement* currElement = samplersElement->FirstChildElement();

		while (currElement != nullptr)
		{
			// Filter
			SamplerFilter filter;
			std::string filterText = ParseXmlAttribute(*currElement, "filter", "nearest");
			
			if		(filterText == "nearest")					{ filter = SAMPLER_FILTER_NEAREST; }
			else if (filterText == "linear")					{ filter = SAMPLER_FILTER_LINEAR; }
			else if (filterText == "nearest_mipmap_nearest")	{ filter = SAMPLER_FILTER_NEAREST_MIPMAP_NEAREST; }
			else if (filterText == "linear_mipmap_nearest")		{ filter = SAMPLER_FILTER_LINEAR_MIPMAP_NEAREST; }
			else if (filterText == "nearest_mipmap_linear")		{ filter = SAMPLER_FILTER_NEAREST_MIPMAP_LINEAR; }
			else if (filterText == "linear_mipmap_linear")		{ filter = SAMPLER_FILTER_LINEAR_MIPMAP_LINEAR; }
			else
			{
				filter = SAMPLER_FILTER_NEAREST;
			}

			// Edge sampling
			EdgeSampling sampling;
			std::string edgeText = ParseXmlAttribute(*currElement, "sampling", "repeat");

			if		(edgeText == "repeat")					{ sampling = EDGE_SAMPLING_REPEAT; }
			else if (edgeText == "mirrored_repeat")			{ sampling = EDGE_SAMPLING_MIRRORED_REPEAT; }
			else if (edgeText == "clamp_to_edge")			{ sampling = EDGE_SAMPLING_CLAMP_TO_EDGE; }
			else if (edgeText == "clamp_to_border")			{ sampling = EDGE_SAMPLING_CLAMP_TO_BORDER; }
			else if (edgeText == "mirror_clamp_to_edge")	{ sampling = EDGE_SAMPLING_MIRROR_CLAMP_TO_EDGE; }
			else
			{
				sampling = EDGE_SAMPLING_REPEAT;
			}

			// Bind point
			int bindPoint = ParseXmlAttribute(*currElement, "bind", 0);

			Sampler* sampler = new Sampler();
			sampler->Initialize(filter, sampling);

			m_samplers[bindPoint] = sampler;

			currElement = currElement->NextSiblingElement();
		}
	}

	// Properties
	const XMLElement* propertiesElement = materialElement->FirstChildElement("properties");

	if (propertiesElement != nullptr)
	{
		const XMLElement* currElement = propertiesElement->FirstChildElement();

		while (currElement != nullptr)
		{
			TODO("Add functionality to set other types");

			std::string propertyName	= ParseXmlAttribute(*currElement, "name", "");
			float propertyValue			= ParseXmlAttribute(*currElement, "value", 0.f);

			if (!IsStringNullOrEmpty(propertyName))
			{
				SetProperty(propertyName.c_str(), propertyValue);
			}

			currElement = currElement->NextSiblingElement();
		}
	}

	return true;
}


//-----------------------------------------------------------------------------------------------
// Returns the number of Material Property Blocks currently in the material
//
int Material::GetPropertyBlockCount() const
{
	return (int) m_propertyBlocks.size();
}


//-----------------------------------------------------------------------------------------------
// Returns the property block given by name if it exists, nullptr otherwise
//
MaterialPropertyBlock* Material::GetPropertyBlock(const char* blockName) const
{
	int numPropertyBlocks = (int) m_propertyBlocks.size();

	for (int blockIndex = 0; blockIndex < numPropertyBlocks; ++blockIndex)
	{
		if (m_propertyBlocks[blockIndex]->GetName() == blockName)
		{
			return m_propertyBlocks[blockIndex];
		}
	}

	return nullptr;
}


//-----------------------------------------------------------------------------------------------
// Returns the property block given by the index if it exists, nullptr otherwise
//
MaterialPropertyBlock* Material::GetPropertyBlock(int index) const
{
	return m_propertyBlocks[index];
}


//-----------------------------------------------------------------------------------------------
// Returns the shader of the material (constant)
//
const Shader* Material::GetShader() const
{
	return m_shader;
}


//-----------------------------------------------------------------------------------------------
// Returns the shader if it is instanced, otherwise clones it and returns the clone
//
Shader* Material::GetEditableShader()
{
	if (m_isInstancedShader)
	{
		return m_shader;
	}

	// Make it an instance by cloning it
	m_shader = m_shader->Clone();
	m_isInstancedShader = true;
	return m_shader;
}


//-----------------------------------------------------------------------------------------------
// Returns the texture at the given index
//
const Texture* Material::GetTexture(int textureIndex) const
{
	return m_textures[textureIndex];
}


//-----------------------------------------------------------------------------------------------
// Returns the sampler at the given index
//
const Sampler* Material::GetSampler(int samplerIndex) const
{
	return m_samplers[samplerIndex];
}


//-----------------------------------------------------------------------------------------------
// Returns true if the current material is using light data 
//
bool Material::IsUsingLights() const
{
	return (m_shader->GetProgram()->GetUniformDescription()->GetBlockDescription("lightUBO") != nullptr);
}


//-----------------------------------------------------------------------------------------------
// Sets the shader of the material to the one passed, deleting the existing one if it was instanced
//
void Material::SetShader(Shader* shader, bool isInstancedShader)
{
	// Don't do anything if it's the same shader
	if (m_shader != shader)
	{
		if (m_isInstancedShader)
		{
			delete m_shader;
		}
	
		m_shader = shader;
		m_isInstancedShader = isInstancedShader;
	
		// Delete all MaterialPropertyBlocks - the new shader may want to use them for other layouts
		for (int blockIndex = 0; blockIndex < (int) m_propertyBlocks.size(); ++blockIndex)
		{
			delete m_propertyBlocks[blockIndex];
		}
	
		m_propertyBlocks.clear();
	}
}


//-----------------------------------------------------------------------------------------------
// Sets the texture at the given index to the one specified
//
void Material::SetTexture(unsigned int bindPoint, const Texture* texture)
{
	m_textures[bindPoint] = texture;
}


//-----------------------------------------------------------------------------------------------
// Sets the sampler at the given index to the one specified
//
void Material::SetSampler(unsigned int bindPoint, const Sampler* sampler)
{
	m_samplers[bindPoint] = sampler;
}


//-----------------------------------------------------------------------------------------------
// Sets the diffuse texture of the material (bind point 0) to the one specified
//
void Material::SetDiffuse(const Texture* diffuse)
{
	if (diffuse == nullptr)
	{
		diffuse = AssetDB::CreateOrGetTexture("White");
	}

	SetTexture(TEXTURE_DIFFUSE_BIND, diffuse);
}


//-----------------------------------------------------------------------------------------------
// Sets the normal map of the material (bind point 1) to the one specified
//
void Material::SetNormal(const Texture* normal)
{
	if (normal == nullptr)
	{
		normal = AssetDB::CreateOrGetTexture("Flat");
	}

	SetTexture(TEXTURE_NORMAL_BIND, normal);
}


//-----------------------------------------------------------------------------------------------
// Sets the specular map of the material (bind point 2) to the one specified
//
void Material::SetSpecular(const Texture* specular)
{
	if (specular == nullptr)
	{
		specular = AssetDB::GetTexture("Black");
	}

	SetTexture(TEXTURE_SPECULAR_BIND, specular);
}


//-----------------------------------------------------------------------------------------------
// Sets the emissive map of the material (bind point 3) to the one specified
//
void Material::SetEmissive(const Texture* emissive)
{
	if (emissive == nullptr)
	{
		emissive = AssetDB::GetTexture("Black");
	}

	SetTexture(TEXTURE_EMISSIVE_BIND, emissive);
}


//-----------------------------------------------------------------------------------------------
// Sets the property given by propertyName to the data specified
// If the property block for the property hasn't been created yet, it is created
// Returns false of the block description doesn't exist on the current shader, true if the data was set
//
bool Material::SetProperty(const char* propertyName, const void* data, size_t byteSize)
{
	const ShaderDescription* shaderInfo = m_shader->GetProgram()->GetUniformDescription();
	int numBlocksOnShader = (int) shaderInfo->GetBlockCount();

	for (int blockIndex = 0; blockIndex < numBlocksOnShader; ++blockIndex)
	{
		const PropertyBlockDescription* blockDescription = shaderInfo->GetBlockDescription(blockIndex);

		// If the uniform block is an engine reserved one, continue without checking properties
		if (blockDescription->GetBlockBinding() < ENGINE_RESERVED_UNIFORM_BLOCK_COUNT) { continue; }

		const PropertyDescription* propertyDescription = blockDescription->GetPropertyDescription(propertyName);

		if (propertyDescription != nullptr)
		{
			// Found the block, so get the name
			std::string blockName = blockDescription->GetName();
			
			// Now get the material block associated with this description
			MaterialPropertyBlock* matBlock = GetPropertyBlock(blockName.c_str());
			
			// A material block doesn't exist for this description yet, so make one
			if (matBlock == nullptr)
			{
				matBlock = CreatePropertyBlock(blockDescription);
			}

			// Size is just for a check, to ensure it matches
			size_t actualSize = propertyDescription->GetSize();
			ASSERT_OR_DIE(actualSize == byteSize, Stringf("Error: Material::SetProperty() had size mismatch - for property \"%s\", the passed size was %i, where description size has size %i", propertyName, byteSize, actualSize));

			// Offset into the block
			size_t offset = propertyDescription->GetOffset();

			// Set the data and return
			matBlock->UpdateCPUData(offset, byteSize, data);
			return true;
		}
	}

	return false;
}


//-----------------------------------------------------------------------------------------------
// Sets the block to the data given, creating a block if one doesn't already exist
//
bool Material::SetPropertyBlock(const char* blockName, const void* data, size_t byteSize)
{
	MaterialPropertyBlock* block = GetPropertyBlock(blockName);

	if (block != nullptr)
	{
		block->SetCPUData(byteSize, data);	
		return true;
	}

	// No block exists, so see if we can create it
	const ShaderDescription*		shaderDescription	= m_shader->GetProgram()->GetUniformDescription();
	const PropertyBlockDescription* blockDescription	= shaderDescription->GetBlockDescription(blockName); 

	// If the block doesn't exist or if the uniform block is an engine reserved one, do nothing
	if (blockDescription == nullptr || blockDescription->GetBlockBinding() < ENGINE_RESERVED_UNIFORM_BLOCK_COUNT)
	{
		return false;
	}

	block = CreatePropertyBlock(blockDescription);
	block->SetCPUData(byteSize, data);
	return true;
}


//-----------------------------------------------------------------------------------------------
// Creates a new MaterialPropertyBlock on this material given the block description
//
MaterialPropertyBlock* Material::CreatePropertyBlock(const PropertyBlockDescription* blockDescription)
{
	TODO("Take these errors out/make them warnings when I'm sure the system works well");

	// If the uniform block is an engine reserved one, panic
	if (blockDescription->GetBlockBinding() < ENGINE_RESERVED_UNIFORM_BLOCK_COUNT) 
	{
		ERROR_AND_DIE("Error: Material::CreatePropertyBlock() tried to create a block with binding within the engine reserved set.");
	}

	// Ensure we don't duplicate bindings or names!
	// Delete any blocks that will have the same binding or name as this block
	int numBlocks = (int) m_propertyBlocks.size();
	unsigned int newBlockBinding = blockDescription->GetBlockBinding();
	std::string newBlockName = blockDescription->GetName();

	for (int blockIndex = 0; blockIndex < numBlocks; ++blockIndex)
	{
		MaterialPropertyBlock* currBlock = m_propertyBlocks[blockIndex];
		unsigned int currBlockBinding = currBlock->GetDescription()->GetBlockBinding();
		std::string currBlockName = currBlock->GetName();

		if (currBlockBinding == newBlockBinding || currBlockName == newBlockName)
		{
			ERROR_AND_DIE("Error: Material::CreatePropertyBlock() tried to create a block when an existing block already had the name/binding");
		}	
	}

	MaterialPropertyBlock* block = new MaterialPropertyBlock(blockDescription);
	m_propertyBlocks.push_back(block);
	return block;
}
