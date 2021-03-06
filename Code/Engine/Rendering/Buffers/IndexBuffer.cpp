/************************************************************************/
/* File: IndexBuffer.cpp
/* Author: Andrew Chase
/* Date: March 25th, 2018
/* Description: Implementation of the IndexBuffer class
/************************************************************************/
#include "Engine/Rendering/Buffers/IndexBuffer.hpp"


//-----------------------------------------------------------------------------------------------
// Constructor
//
IndexBuffer::IndexBuffer()
	: m_indexCount(0)
	, m_indexStride(sizeof(unsigned int))
{
}


//-----------------------------------------------------------------------------------------------
// Copies the index data to the GPU
//
bool IndexBuffer::CopyToGPU(int indexCount, const unsigned int* indices)
{
	size_t byteCount = indexCount * m_indexStride;
	bool succeeded = RenderBuffer::CopyToGPU(byteCount, (const void*)indices);

	// Only update if data was copied
	if (succeeded)
	{
		m_indexCount = indexCount;
	}

	return succeeded;
}


//-----------------------------------------------------------------------------------------------
// Copies the data from the source handle into this index buffer
//
bool IndexBuffer::CopyFromGPUBuffer(unsigned int indexCount, unsigned int sourceHandle)
{
	size_t byteCount = indexCount * m_indexStride;
	bool succeeded = RenderBuffer::CopyFromGPUBuffer(byteCount, sourceHandle);

	// Only update if data was copied
	if (succeeded)
	{
		m_indexCount = indexCount;
	}

	return succeeded;
}


//-----------------------------------------------------------------------------------------------
// Sets the index count of the buffer to the value specified
//
void IndexBuffer::SetIndexCount(unsigned int indexCount)
{
	m_indexCount = indexCount;
	m_bufferSize = indexCount * m_indexStride;
}


//-----------------------------------------------------------------------------------------------
// Returns the current number of indices in the buffer
//
unsigned int IndexBuffer::GetIndexCount() const
{
	return m_indexCount;
}


//-----------------------------------------------------------------------------------------------
// Returns the current index stride of the buffer
//
unsigned int IndexBuffer::GetIndexStride() const
{
	return m_indexStride;
}
