/************************************************************************/
/* File: Camera.cpp
/* Author: Andrew Chase
/* Date: February 13th, 2018
/* Description: Implementation of the Camera class
/************************************************************************/
#include "Game/Framework/EngineBuildPreferences.hpp"
#include "Engine/Rendering/Core/Camera.hpp"
#include "Engine/Core/Window.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/MathUtils.hpp"

// Struct used for uniform buffer
struct CameraBufferData
{
	Matrix44 m_viewMatrix;
	Matrix44 m_projectionMatrix;

	Matrix44 m_cameraMatrix;

	Vector3 m_cameraX;
	float	m_padding0;
	Vector3 m_cameraY;
	float	m_padding1;
	Vector3 m_cameraZ;
	float	m_padding2;
	Vector3 m_cameraPosition;
	float	m_padding3;

	Matrix44 m_inverseViewProjection;
};

//-----------------------------------------------------------------------------------------------
// Constructor
//
Camera::Camera()
	: m_drawOrder(0)
{
#ifdef COORDINATE_SYSTEM_LEFT_HAND_Y_UP
	m_changeOfBasisMatrix = Matrix44::IDENTITY; // If nothing is defined, will still default to Identity
#elif defined COORDINATE_SYSTEM_RIGHT_HAND_Z_UP
	m_changeOfBasisMatrix = Matrix44(	Vector4(0.f, 0.f, 1.f, 0.f),
										Vector4(-1.f, 0.f, 0.f, 0.f),
										Vector4(0.f, 1.f, 0.f, 0.f),
										Vector4(0.f, 0.f, 0.f, 1.f));
#endif
}


//-----------------------------------------------------------------------------------------------
// Moves the camera in world space, given the direction and speed
//
void Camera::TranslateWorld(const Vector3& translation)
{
	m_transform.TranslateWorld(translation);
	m_viewMatrix = InvertLookAtMatrix(m_transform.GetWorldMatrix());
}


//-----------------------------------------------------------------------------------------------
// Moves the camera in local space, given the direction and speed
//
void Camera::TranslateLocal(const Vector3& localTranslation)
{
	m_transform.TranslateLocal(localTranslation);
	m_viewMatrix = InvertLookAtMatrix(m_transform.GetWorldMatrix());
}


//-----------------------------------------------------------------------------------------------
// Rotates the camera by the given euler angle values
// Returns the rotation if we desire to clamp it
//
Vector3 Camera::Rotate(const Vector3& rotation)
{
	Vector3 newRotation = m_transform.rotation + rotation;
	SetRotation(newRotation);

	return newRotation;
}


//-----------------------------------------------------------------------------------------------
// Sets the rotation to the one provided
//
void Camera::SetRotation(const Vector3& newRotation)
{
	m_transform.SetRotation(newRotation);
	m_viewMatrix = InvertLookAtMatrix(m_transform.GetWorldMatrix());
}


//-----------------------------------------------------------------------------------------------
// Sets the camera transform to that specified, and updates the view matrix
//
void Camera::SetTransform(const Transform& transform)
{
	m_transform = transform;

	m_viewMatrix = InvertLookAtMatrix(m_transform.GetWorldMatrix());
}


//-----------------------------------------------------------------------------------------------
// Sets the position of the camera to the one given
//
void Camera::SetPosition(const Vector3& position)
{
	m_transform.position = position;
	m_viewMatrix = InvertLookAtMatrix(m_transform.GetWorldMatrix());
}


//-----------------------------------------------------------------------------------------------
// Sets the color target of the Camera's FrameBuffer to the one passed
//
void Camera::SetColorTarget(Texture* colorTarget)
{
	m_frameBuffer.SetColorTarget(colorTarget);
}


//-----------------------------------------------------------------------------------------------
// Sets the depth target of the Camera's FrameBuffer to the one passed
//
void Camera::SetDepthTarget(Texture* depthTarget)
{
	m_frameBuffer.SetDepthTarget(depthTarget);
}


//-----------------------------------------------------------------------------------------------
// Finalizes the Camera's FrameBuffer
//
void Camera::FinalizeFrameBuffer()
{
	m_frameBuffer.Finalize();
}


//-----------------------------------------------------------------------------------------------
// Sets the camera to look at target from position, with the reference up-vector up
//
void Camera::LookAt(const Vector3& position, const Vector3& target, const Vector3& up /*= Vector3::DIRECTION_UP*/)
{
	Matrix44 cameraMatrix = Matrix44::MakeLookAt(position, target, up);

	m_transform.position = position;
	m_transform.rotation = Matrix44::ExtractRotationDegrees(cameraMatrix);

	m_transform.SetModelMatrix(cameraMatrix);
	m_viewMatrix = InvertLookAtMatrix(cameraMatrix);
}


//-----------------------------------------------------------------------------------------------
// Sets the camera matrix to the one passed
//
void Camera::SetCameraMatrix(const Matrix44& cameraMatrix)
{
	m_transform.SetModelMatrix(cameraMatrix);
	m_viewMatrix = InvertLookAtMatrix(cameraMatrix);
}


//-----------------------------------------------------------------------------------------------
// Sets the view matrix to the one passed
//
void Camera::SetViewMatrix(const Matrix44& viewMatrix)
{
	m_viewMatrix = viewMatrix;
	m_transform.SetModelMatrix(InvertLookAtMatrix(viewMatrix));
}


//-----------------------------------------------------------------------------------------------
// Sets the projection matrix to the one passed
//
void Camera::SetProjection(const Matrix44& projection)
{
	m_projectionMatrix = projection;
}


//-----------------------------------------------------------------------------------------------
// Sets the camera matrix to an orthographic projection given the ortho parameters
//
void Camera::SetProjectionOrtho(float width, float height, float nearZ, float farZ)
{
	m_orthoSize = height;
	m_nearClipZ = nearZ;
	m_farClipZ = farZ;
	m_projectionMatrix = Matrix44::MakeOrtho(-width / 2.f, width / 2.f, -height / 2.f, height / 2.f, nearZ, farZ);
}


//-----------------------------------------------------------------------------------------------
// Sets this camera to be a perspective projection with the given params
//
void Camera::SetProjectionPerspective(float fovDegrees, float nearZ, float farZ)
{
	m_fov = fovDegrees;
	m_nearClipZ = nearZ;
	m_farClipZ = farZ;
	m_projectionMatrix = Matrix44::MakePerspective(fovDegrees, nearZ, farZ);
}


//-----------------------------------------------------------------------------------------------
// Sets the ortho size of the camera to the one given, not taking into consideration size
// Also recalculates the ortho projection matrix
//
void Camera::SetOrthoSize(float newSize)
{
	float width = m_frameBuffer.GetAspect() * newSize;
	SetProjectionOrtho(width, newSize, m_nearClipZ, m_farClipZ);
}


//-----------------------------------------------------------------------------------------------
// Adjusts the camera's ortho size by adding the modifier to it, clamping to the size limits
// Also recalculates the ortho projection matrix
//
void Camera::AdjustOrthoSize(float additiveModifier)
{
	m_orthoSize = ClampFloat(m_orthoSize + additiveModifier, m_orthoSizeLimits.min, m_orthoSizeLimits.max);
	float width = m_frameBuffer.GetAspect() * m_orthoSize;
	SetProjectionOrtho(width, m_orthoSize, m_nearClipZ, m_farClipZ);
}


//-----------------------------------------------------------------------------------------------
// Sets the ortho size limits to the ones given
//
void Camera::SetOrthoSizeLimits(float min, float max)
{
	m_orthoSizeLimits = FloatRange(min, max);
}


//-----------------------------------------------------------------------------------------------
// Sets the matrix appended to the projection matrix when buffered to the GPU;
// Useful when the space the camera lives in isn't OpenGL's basis
//
void Camera::SetChangeOfBasisMatrix(const Matrix44& changeOfBasisMatrix)
{
	m_changeOfBasisMatrix = changeOfBasisMatrix;
}


//-----------------------------------------------------------------------------------------------
// Sets the draw order for the camera, used in ForwardRenderPath sorting
//
void Camera::SetDrawOrder(unsigned int order)
{
	m_drawOrder = order;
}


//-----------------------------------------------------------------------------------------------
// Update the camera's uniform buffer with the camera's current state
//
void Camera::FinalizeUniformBuffer()
{
	CameraBufferData bufferData;

	bufferData.m_viewMatrix = m_changeOfBasisMatrix * m_viewMatrix;
	bufferData.m_projectionMatrix = m_projectionMatrix; // Append change of basis!
	bufferData.m_cameraMatrix = m_transform.GetWorldMatrix();

	bufferData.m_cameraX	= GetIVector();
	bufferData.m_cameraY	= GetJVector();
	bufferData.m_cameraZ	= GetKVector();
	bufferData.m_cameraPosition = m_transform.position;

	bufferData.m_inverseViewProjection = Matrix44::GetInverse(m_viewMatrix) * Matrix44::GetInverse(m_projectionMatrix * m_changeOfBasisMatrix);
	
	m_uniformBuffer.SetCPUAndGPUData(sizeof(CameraBufferData), &bufferData);
}


//-----------------------------------------------------------------------------------------------
// Returns this camera's gpu-side uniform buffer handle
//
GLuint Camera::GetUniformBufferHandle() const
{
	return m_uniformBuffer.GetHandle();
}


//-----------------------------------------------------------------------------------------------
// Returns the camera matrix (Camera's model matrix, inverse of the view matrix)
// (local to world)
//
Matrix44 Camera::GetCameraMatrix() const
{
	return m_transform.GetWorldMatrix();
}


//-----------------------------------------------------------------------------------------------
// Returns the view matrix (world to camera local, inverse of the camera matrix)
//
Matrix44 Camera::GetViewMatrix() const
{
	return m_viewMatrix;
}


//-----------------------------------------------------------------------------------------------
// Returns the projection matrix of the Camera
//
Matrix44 Camera::GetProjectionMatrix() const
{
	return m_projectionMatrix;
}


//-----------------------------------------------------------------------------------------------
// Returns the position of the camera
//
Vector3 Camera::GetPosition() const
{
	return m_transform.GetWorldPosition();
}


//-----------------------------------------------------------------------------------------------
// Returns the rotation of the camera's transform
//
Vector3 Camera::GetRotation() const
{
	return m_transform.rotation;
}


//-----------------------------------------------------------------------------------------------
// Returns the (K) vector of the camera's transform
//
Vector3 Camera::GetKVector() const
{
	return m_transform.GetWorldMatrix().GetKVector().xyz();
}


//-----------------------------------------------------------------------------------------------
// Returns the (I) vector of the camera's transform
//
Vector3 Camera::GetIVector() const
{
	return m_transform.GetWorldMatrix().GetIVector().xyz();
}


//-----------------------------------------------------------------------------------------------
// Returns the (J) vector of the camera's transform
//
Vector3 Camera::GetJVector() const
{
	return m_transform.GetWorldMatrix().GetJVector().xyz();
}


//-----------------------------------------------------------------------------------------------
// Returns the GPU handle of this camera's FrameBuffer member
//
unsigned int Camera::GetFrameBufferHandle() const
{
	return m_frameBuffer.GetHandle();
}


//-----------------------------------------------------------------------------------------------
// Returns the draw order of the camera
//
unsigned int Camera::GetDrawOrder() const
{
	return m_drawOrder;
}


//-----------------------------------------------------------------------------------------------
// Inverts the lookat matrix given, used to construct the view matrix from the camera matrix
//
Matrix44 Camera::InvertLookAtMatrix(const Matrix44& lookAt) const
{
	Matrix44 rotation = lookAt;

	rotation.Tx = 0.f;
	rotation.Ty = 0.f;
	rotation.Tz = 0.f;

	rotation.Transpose();

	Matrix44 translation = Matrix44::MakeTranslation(Vector3(-lookAt.Tx, -lookAt.Ty, -lookAt.Tz));
	rotation.Append(translation);
	return rotation;
}
