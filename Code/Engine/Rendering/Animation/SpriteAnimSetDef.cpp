/************************************************************************/
/* File: SpriteAnimSetDef.cpp
/* Author: Andrew Chase
/* Date: November 13th, 2017
/* Bugs: None
/* Description: Implementation of the SpriteAnimSetDef class
/************************************************************************/
#include "Engine/Assets/AssetDB.hpp"
#include "Engine/Core/Utility/StringUtils.hpp"
#include "Engine/Core/Utility/XmlUtilities.hpp"
#include "Engine/Rendering/Animation/SpriteAnimDef.hpp"
#include "Engine/Core/Utility/ErrorWarningAssert.hpp"
#include "Engine/Rendering/Animation/SpriteAnimSetDef.hpp"


//-----------------------------------------------------------------------------------------------
// Constructor - using an XMLElement representing an Animation Set Definition
//
SpriteAnimSetDef::SpriteAnimSetDef(const XMLElement& animationSetElement)
{
	// Create the spritesheet for the animation child elements
	std::string		spriteSheetName		= ParseXmlAttribute(animationSetElement, "spriteSheet", nullptr);
	std::string		spriteSheetFilePath = Stringf("Data/Images/%s", spriteSheetName.c_str());
	IntVector2		spriteLayout		= ParseXmlAttribute(animationSetElement, "spriteLayout", spriteLayout);
	Texture*		spriteSheetTexture	= AssetDB::CreateOrGetTexture(spriteSheetFilePath.c_str());
	SpriteSheet		setSpriteSheet		= SpriteSheet(*spriteSheetTexture, spriteLayout);


	// Iterate across animation elements to create animation definitions
	const XMLElement* animationElement = animationSetElement.FirstChildElement();

	while (animationElement != nullptr)
	{
		// Make the animation definition
		SpriteAnimDef* animationDefinition = new SpriteAnimDef(setSpriteSheet, *animationElement);

		// Add it to the list of animation definitions by name
		std::string animationName = animationDefinition->GetName();
		AddAnimationDefinition(animationName, animationDefinition);

		// Iterate to the next animation element
		animationElement = animationElement->NextSiblingElement();
	}
}


//-----------------------------------------------------------------------------------------------
// Returns the animation defintion given by the name animationName
//
SpriteAnimDef* SpriteAnimSetDef::GetAnimationDefinitionByName(const std::string& animationName) const
{
	// Ensure the animation exists
	bool animationExists = (m_animDefinitions.find(animationName) != m_animDefinitions.end());
	GUARANTEE_OR_DIE(animationExists, Stringf("Error: SpriteAnimSetDef::GetAnimationByName couldn't find animation \"%s\" in its map of animations", animationName.c_str()))

	return m_animDefinitions.at(animationName);
}


//-----------------------------------------------------------------------------------------------
// Adds the animation definition to this set, avoid duplicates
//
void SpriteAnimSetDef::AddAnimationDefinition(const std::string& animationName, SpriteAnimDef* animationToAdd)
{
	// Ensure the animation doesn't exist
	bool animationExists = (m_animDefinitions.find(animationName) != m_animDefinitions.end());
	GUARANTEE_OR_DIE(!animationExists, Stringf("Error: SpriteAnimSetDef::AddAnimationDefinition tried to add duplicate animation \"%s\"", animationName.c_str()))

	m_animDefinitions[animationName] = animationToAdd;
}
