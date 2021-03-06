/************************************************************************/
/* Project: SD1 A4 - Incursion Foundation
/* File: IntVector2.cpp
/* Author: Andrew Chase
/* Date: September 28th, 2017
/* Bugs: None
/* Description: Implementation of the IntVector2 class
/************************************************************************/
#include <math.h>
#include <string>
#include "Engine/Math/IntVector2.hpp"
#include "Engine/Math/MathUtils.hpp"


// Initialize the static constants
const IntVector2 IntVector2::ZERO				= IntVector2(0, 0);
const IntVector2 IntVector2::STEP_NORTH			= IntVector2(0, 1);
const IntVector2 IntVector2::STEP_SOUTH			= IntVector2(0, -1);
const IntVector2 IntVector2::STEP_EAST			= IntVector2(1, 0);
const IntVector2 IntVector2::STEP_WEST			= IntVector2(-1, 0);
const IntVector2 IntVector2::STEP_NORTHEAST		= IntVector2(1, 1);
const IntVector2 IntVector2::STEP_NORTHWEST		= IntVector2(-1, 1);
const IntVector2 IntVector2::STEP_SOUTHEAST		= IntVector2(1, -1);
const IntVector2 IntVector2::STEP_SOUTHWEST		= IntVector2(-1, -1);


//-----------------------------------------------------------------------------------------------
// Copy constructor
IntVector2::IntVector2( const IntVector2& copy )
  	: x( copy.x )
	, y( copy.y )
{
}


//-----------------------------------------------------------------------------------------------
// Explicit constructor
IntVector2::IntVector2( int initialX, int initialY )
	: x( initialX )
	, y( initialY )
{
}


//-----------------------------------------------------------------------------------------------
// Explicit float constructor
IntVector2::IntVector2(float initialX, float initialY)
	: x(static_cast<int>(initialX))
	, y(static_cast<int>(initialY))
{
}

//-----------------------------------------------------------------------------------------------
// Explicit float vector constructor
IntVector2::IntVector2(const Vector2& floatVector)
	: x(static_cast<int>(floatVector.x))
	, y(static_cast<int>(floatVector.y))
{
}


//-----------------------------------------------------------------------------------------------
// Constructor - from single int value
IntVector2::IntVector2(int initialValue)
	: x(initialValue), y(initialValue)
{
}

//------------------------------ Operator Overloads ---------------------------------------------

//-----------------------------------------------------------------------------------------------
const IntVector2 IntVector2::operator + ( const IntVector2& vecToAdd ) const
{
	return IntVector2( (x + vecToAdd.x), (y + vecToAdd.y) );
}


//-----------------------------------------------------------------------------------------------
const IntVector2 IntVector2::operator-( const IntVector2& vecToSubtract ) const
{
	return IntVector2( (x - vecToSubtract.x), (y - vecToSubtract.y) );
}


//-----------------------------------------------------------------------------------------------
bool IntVector2::operator<(const IntVector2& compare) const
{
	// Compare on y, using x as a tie breaker
	if		(y < compare.y) { return true; }
	else if (compare.y < y) { return false; }
	else if (x < compare.x) { return true; }
	else
	{
		return false;
	}
}


//-----------------------------------------------------------------------------------------------
const IntVector2 IntVector2::operator*( int uniformScale ) const
{
	return IntVector2( (x * uniformScale), (y * uniformScale) );
}


//-----------------------------------------------------------------------------------------------
const IntVector2 IntVector2::operator/(int divisor) const
{
	return IntVector2((x / divisor), (y / divisor));
}


//-----------------------------------------------------------------------------------------------
void IntVector2::operator+=( const IntVector2& vecToAdd )
{
	x += vecToAdd.x;
	y += vecToAdd.y;
}


//-----------------------------------------------------------------------------------------------
void IntVector2::operator-=( const IntVector2& vecToSubtract )
{
	x -= vecToSubtract.x;
	y -= vecToSubtract.y;
}


//-----------------------------------------------------------------------------------------------
void IntVector2::operator*=( const int uniformScale )
{
	x *= uniformScale;
	y *= uniformScale;
}


//-----------------------------------------------------------------------------------------------
void IntVector2::operator=( const IntVector2& copyFrom )
{
	x = copyFrom.x;
	y = copyFrom.y;
}


//-----------------------------------------------------------------------------------------------
const IntVector2 operator*( int uniformScale, const IntVector2& vecToScale )
{
	return IntVector2( static_cast<int>(vecToScale.x * uniformScale), static_cast<int>(vecToScale.y * uniformScale) );
}


//-----------------------------------------------------------------------------------------------
bool IntVector2::operator==( const IntVector2& compare ) const
{
	if ( x == compare.x && y == compare.y ) {
		return true;
	}
	return false;
}


//-----------------------------------------------------------------------------------------------
bool IntVector2::operator!=( const IntVector2& compare ) const
{
	if ( x != compare.x || y != compare.y ) {
		return true;
	}
	return false;
}


//-----------------------------------------------------------------------------------------------
// Calculates the magnitude (length) of the vector and returns it
//
float IntVector2::GetLength() const
{
	return sqrtf(static_cast<float>((x * x) + (y * y)));
}


//-----------------------------------------------------------------------------------------------
// Calculates the squared magnitude (length) of the vector and returns it
//
float IntVector2::GetLengthSquared() const
{
	return static_cast<float>((x * x) + (y * y));
}


//-----------------------------------------------------------------------------------------------
// Returns the degree orientation of a vector, as if the angle was on a unit circle
//
float IntVector2::GetOrientationDegrees() const
{
	return Atan2Degrees(static_cast<float>(y), static_cast<float>(x));
}


//-----------------------------------------------------------------------------------------------
// Sets the int values based on the text representation passed
//
void IntVector2::SetFromText(const char* text)
{
	std::string stringText = std::string(text);

	size_t commaPosition = stringText.find(",");

	// No comma present in text
	if (commaPosition == std::string::npos)
	{
		return;
	}

	x = atoi(std::string(stringText, 0, commaPosition).c_str());
	y = atoi(std::string(stringText, commaPosition + 1).c_str());
}


//-----------------------------------------------------------------------------------------------
// Returns the x,y value as floats
//
Vector2 IntVector2::GetAsFloats() const
{
	return Vector2(*this);
}


//-----------------------------------------------------------------------------------------------
// Returns the distance between points a and b on a 2D plane
//
IntVector2 IntVector2::GetRandomVector(int maxDeviation)
{
	IntVector2 randomVector;

	randomVector.x = GetRandomIntInRange(-maxDeviation, maxDeviation);
	randomVector.y = GetRandomIntInRange(-maxDeviation, maxDeviation);

	return randomVector;
}


//-----------------------------------------------------------------------------------------------
// Returns the distance between points a and b on a 2D plane
//
float GetDistance(const IntVector2& a, const IntVector2& b)
{
	return sqrtf(static_cast<float>((a.x - b.x) * (a.x - b.x)) + ((a.y - b.y) * (a.y - b.y)));
}


//-----------------------------------------------------------------------------------------------
// Returns the squared distance between points a and b on a 2D plane
//
float GetDistanceSquared(const IntVector2& a, const IntVector2& b)
{
	return (static_cast<float>((a.x - b.x) * (a.x - b.x)) + ((a.y - b.y) * (a.y - b.y)));
}
