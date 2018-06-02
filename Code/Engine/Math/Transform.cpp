/************************************************************************/
/* File: Transform.cpp
/* Author: Andrew Chase
/* Date: March 27th, 2017
/* Description: Implementation of the Transform class
/************************************************************************/
#include "Engine/Math/Transform.hpp"
#include "Engine/Math/MathUtils.hpp"


//-----------------------------------------------------------------------------------------------
// Constructor from position, rotation, and scale
//
Transform::Transform(const Vector3& position, const Vector3& rotation, const Vector3& scale)
	: position(position), rotation(rotation), scale(scale)
{
	CheckAndUpdateModelMatrix();
}


//-----------------------------------------------------------------------------------------------
// Default Constructor
//
Transform::Transform()
	: position(Vector3::ZERO), rotation(Vector3::ZERO), scale(Vector3::ONES)
{
	CheckAndUpdateModelMatrix();
}


//-----------------------------------------------------------------------------------------------
// Operator = for setting a transform to another transform
//
void Transform::operator=(const Transform& copyFrom)
{
	position = copyFrom.position;
	rotation = copyFrom.rotation;
	scale = copyFrom.scale;
}


//-----------------------------------------------------------------------------------------------
// Sets the position of the transform
//
void Transform::SetPosition(const Vector3& newPosition)
{
	position = newPosition;
}
 

//-----------------------------------------------------------------------------------------------
// Sets the rotation of the transform
//
void Transform::SetRotation(const Vector3& newRotation)
{
	rotation = newRotation;
}


//-----------------------------------------------------------------------------------------------
// Sets the scale of the transform
//
void Transform::SetScale(const Vector3& newScale)
{
	scale = newScale;
}


//-----------------------------------------------------------------------------------------------
// Sets the model matrix for this transform, updating its position, rotation, and scale
//
void Transform::SetModelMatrix(const Matrix44& model)
{
	m_modelMatrix = model;

	position	= Matrix44::ExtractTranslation(model);
	rotation	= Matrix44::ExtractRotationDegrees(model);
	scale		= Matrix44::ExtractScale(model);
}


//-----------------------------------------------------------------------------------------------
// Sets the parent transform of this transform to the one specified
//
void Transform::SetParentTransform(Transform* parent)
{
	m_parentTransform = parent;
}


//-----------------------------------------------------------------------------------------------
// Translates the position of the matrix by deltaPosition
//
void Transform::TranslateWorld(const Vector3& worldTranslation)
{
	position += worldTranslation;
}


//-----------------------------------------------------------------------------------------------
// Translates the transform by the local space translation
//
void Transform::TranslateLocal(const Vector3& localTranslation)
{
	Vector4 worldTranslation = GetToWorldMatrix() * Vector4(localTranslation, 0.f);
	TranslateWorld(worldTranslation.xyz());
}


//-----------------------------------------------------------------------------------------------
// Rotates the translation by deltaRotation
//
void Transform::Rotate(const Vector3& deltaRotation)
{
	rotation.x = GetAngleBetweenZeroThreeSixty(rotation.x + deltaRotation.x);
	rotation.y = GetAngleBetweenZeroThreeSixty(rotation.y + deltaRotation.y);
	rotation.z = GetAngleBetweenZeroThreeSixty(rotation.z + deltaRotation.z);
}


//-----------------------------------------------------------------------------------------------
// Scales the matrix by deltaScale
//
void Transform::Scale(const Vector3& deltaScale)
{
	scale.x *= deltaScale.x;
	scale.y *= deltaScale.y;
	scale.z *= deltaScale.z;
}


//-----------------------------------------------------------------------------------------------
// Returns the model matrix of this transform, recalculating it if it's outdated
//
Matrix44 Transform::GetModelMatrix()
{
	CheckAndUpdateModelMatrix();
	return m_modelMatrix;
}


//-----------------------------------------------------------------------------------------------
// Returns the matrix that transforms this space to absolute world space
//
Matrix44 Transform::GetToWorldMatrix()
{
	CheckAndUpdateModelMatrix();

	if (m_parentTransform != nullptr)
	{
		Matrix44 parentToWorld = m_parentTransform->GetToWorldMatrix();
		return parentToWorld * m_modelMatrix;
	}
	else
	{
		return m_modelMatrix;
	}
}


//-----------------------------------------------------------------------------------------------
// Returns the world right vector for this transform
//
Vector3 Transform::GetWorldRight()
{
	return GetModelMatrix().GetIVector().xyz();
}


//-----------------------------------------------------------------------------------------------
// Returns the world up vector for this transform
//
Vector3 Transform::GetWorldUp()
{
	return GetModelMatrix().GetJVector().xyz();
}


//-----------------------------------------------------------------------------------------------
// Returns the world forward vector for this transform
//
Vector3 Transform::GetWorldForward()
{
	return GetModelMatrix().GetKVector().xyz();
}


//-----------------------------------------------------------------------------------------------
// Recalculates the model matrix of this transform given its current position, rotation, and scale
//
void Transform::CheckAndUpdateModelMatrix()
{
	// Check if it needs to be updated first
	bool translationUpToDate = AreMostlyEqual(position, m_oldPosition);
	bool rotationUpToDate = AreMostlyEqual(rotation, m_oldRotation);
	bool scaleUpToDate = AreMostlyEqual(scale, m_oldScale);

	// If any out of date, recalculate the matrix
	if (!translationUpToDate || !rotationUpToDate || !scaleUpToDate)
	{
		Matrix44 translationMatrix	= Matrix44::MakeTranslation(position);
		Matrix44 rotationMatrix		= Matrix44::MakeRotation(rotation);
		Matrix44 scaleMatrix		= Matrix44::MakeScale(scale);

		m_modelMatrix = translationMatrix * rotationMatrix * scaleMatrix;

		// Set old values for the next call
		m_oldPosition = position;
		m_oldRotation = rotation;
		m_oldScale = scale;
	}
}