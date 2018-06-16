/************************************************************************/
/* File: SkeletonBase.cpp
/* Author: Andrew Chase
/* Date: June 15th, 2018
/* Description: Implementation of the SkeletonBase class
/************************************************************************/
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Rendering/Animation/SkeletonBase.hpp"


//-----------------------------------------------------------------------------------------------
// Returns the bone data structure for the bone at the given index
//
BoneData_t SkeletonBase::GetBoneData(unsigned int boneIndex)
{
	ASSERT_OR_DIE(boneIndex < m_boneData.size(), Stringf("Error: SkeletonBase::SetOffsetMatrix received index out of bounds - size is %i, index is %i.", m_boneData.size(), boneIndex));

	return m_boneData[boneIndex];
}


//-----------------------------------------------------------------------------------------------
// Returns the index of the bone given by name in the mappings array
// Returns -1 if a bone of the given name doesn't exist
//
int SkeletonBase::GetBoneMapping(const std::string name)
{
	bool nameExists = m_boneNameMappings.find(name) != m_boneNameMappings.end();

	if (nameExists)
	{
		return m_boneNameMappings[name];
	}

	return -1;
}


//-----------------------------------------------------------------------------------------------
// Returns the index of the bone mapped by the given name if it already exists
// If it doesn't exist, it creates a mapping and index for it and returns the newly created index
//
int SkeletonBase::CreateOrGetBoneMapping(const std::string& boneName)
{
	bool nameExists = m_boneNameMappings.find(boneName) != m_boneNameMappings.end();
	int boneIndex;

	if (nameExists)
	{
		boneIndex = m_boneNameMappings[boneName];
	}
	else
	{
		boneIndex = (unsigned int) m_boneData.size();
		m_boneData.push_back(BoneData_t());
		m_boneNameMappings[boneName] = boneIndex;
	}

	return boneIndex;
}


//-----------------------------------------------------------------------------------------------
// Returns the inverse matrix for the assimp data tree
//
Matrix44 SkeletonBase::GetGlobalInverseTransform() const
{
	return m_globalInverseTransform;
}


//-----------------------------------------------------------------------------------------------
// Returns the number of bones in the skeleton
//
unsigned int SkeletonBase::GetBoneCount() const
{
	return (unsigned int) m_boneData.size();
}


//-----------------------------------------------------------------------------------------------
// Sets the offset matrix of the bone given by boneIndex
// The offset matrix is the matrix that converts the vertex from model space into bone space
//
void SkeletonBase::SetOffsetMatrix(unsigned int boneIndex, const Matrix44& offsetMatrix)
{
	ASSERT_OR_DIE(boneIndex < m_boneData.size(), Stringf("Error: SkeletonBase::SetOffsetMatrix received index out of bounds - size is %i, index is %i.", m_boneData.size(), boneIndex));

	m_boneData[boneIndex].offsetMatrix = offsetMatrix;
}


//-----------------------------------------------------------------------------------------------
// Sets the world matrix of the bone given by boneIndex
// The world matrix is the matrix that transforms from bone space directly to world space
//
void SkeletonBase::SetWorldTransform(unsigned int boneIndex, const Matrix44& worldTransform)
{
	m_boneData[boneIndex].worldTransform = worldTransform;
}


//-----------------------------------------------------------------------------------------------
// Sets the final transformation matrix of the bone given by boneIndex
// The final transformation is the matrix that is used in the shader, that transforms the vertex
// into local space with an applied weight per vertex. For this base, it just transforms to the
// bind pose
//
void SkeletonBase::SetFinalTransformation(unsigned int boneIndex, const Matrix44& toWorldMatrix)
{
	ASSERT_OR_DIE(boneIndex < m_boneData.size(), Stringf("Error: SkeletonBase::SetToWorldMatrix received index out of bounds - size is %i, index is %i.", m_boneData.size(), boneIndex));

	m_boneData[boneIndex].finalTransformation = toWorldMatrix;
}


//-----------------------------------------------------------------------------------------------
// Sets the parent index of the given bone at boneIndex
//
void SkeletonBase::SetParentBoneIndex(unsigned int boneIndex, int parentBoneIndex)
{
	ASSERT_OR_DIE(boneIndex < m_boneData.size(), Stringf("Error: SkeletonBase::SetToWorldMatrix received index out of bounds - size is %i, index is %i.", m_boneData.size(), boneIndex));

	m_boneData[boneIndex].parentIndex = parentBoneIndex;
}


//-----------------------------------------------------------------------------------------------
// Sets the global inverse transform for the skeleton to the one provided
//
void SkeletonBase::SetGlobalInverseTransform(const Matrix44& inverseTransform)
{
	m_globalInverseTransform = inverseTransform;
}