/************************************************************************/
/* Project: MP1 C27 A1 - Vector2, Basics, MathUtils
/* File: MathUtils.cpp
/* Author: Andrew Chase
/* Date: August 30th, 2017
/* Bugs: None
/* Description: Implementation of the MathUtils class
/************************************************************************/
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Quaternion.hpp"
#include <math.h>
#include <cstdlib>


//-----------------------------------------------------------------------------------------------
// Returns the largest integer less than or equal to the passed value
//
int Ceiling(float value)
{
	return (int) ceilf(value);
}


//-----------------------------------------------------------------------------------------------
// Return the floor of value (largest int that is less than value)
//
int Floor(float value)
{
	return (int)floorf(value);
}


//-----------------------------------------------------------------------------------------------
// Returns the log base 2 of the given value
//
float Log2(float value)
{
	return (float) log2(value);
}


//-----------------------------------------------------------------------------------------------
// Returns the log base 10 of the given value
//
float Log10(float value)
{
	return (float) log10(value);
}


//-----------------------------------------------------------------------------------------------
// Returns the value x modded by y, equivalent to x - n*y where n is x/y with the fraction truncated
//
float ModFloat(float x, float y)
{
	return fmodf(x, y);
}


//-----------------------------------------------------------------------------------------------
// Returns the square root of the value given
//
float Sqrt(float value)
{
	return sqrtf(value);
}


//-----------------------------------------------------------------------------------------------
// Returns the result of raising base to the exponent power
//
float Pow(float base, float exponent)
{
	return powf(base, exponent);
}


//-----------------------------------------------------------------------------------------------
// Converts the radius and radian angle to cartesian coordinates
//
Vector2 PolarToCartesian(float radius, float angleRadians)
{
	Vector2 result;
	result.x = radius * cosf(angleRadians);
	result.y = radius * sinf(angleRadians);
	return result;
}


//-----------------------------------------------------------------------------------------------
// Converts the (x,y) cartesian coordinate to polar coordinates
//
void CartesianToPolar(float x, float y, float& out_radius, float& out_angleRadians)
{
	out_radius			= sqrtf((x * x) + (y * y));
	out_angleRadians	= atan2f(y, x);
}


//-----------------------------------------------------------------------------------------------
// Converts a (r, theta, phi) spherical coordinate to an (x, y, z) cartesian coordinate.
// Assumes angles are in degrees.
//
Vector3 SphericalToCartesian(float radius, float rotationDegrees, float azimuthDegrees)
{
	Vector3 cartesianCoordinate;

	cartesianCoordinate.x = radius * CosDegrees(rotationDegrees) * SinDegrees(azimuthDegrees);
	cartesianCoordinate.z = radius * SinDegrees(rotationDegrees) * SinDegrees(azimuthDegrees);
	cartesianCoordinate.y = radius * CosDegrees(azimuthDegrees);

	return cartesianCoordinate;
}


//-----------------------------------------------------------------------------------------------
// Converts an angle in radians to its degree measure and returns it
//
float ConvertRadiansToDegrees(float radians) 
{
	return radians * (180.f / PI);
}


//-----------------------------------------------------------------------------------------------
// Converts an angle in degrees to its radian measure and returns it
//
float ConvertDegreesToRadians(float degrees)
{
	return degrees * (PI / 180.f);
}


//-----------------------------------------------------------------------------------------------
// Calculates the cosine of an angle in degrees and returns it
//
float CosDegrees(float degrees)
{
	float radians = ConvertDegreesToRadians(degrees);

	return cosf(radians);
}


//-----------------------------------------------------------------------------------------------
// Caluclates the arcosine, in degrees
//
float ACosDegrees(float ratio)
{
	ClampFloat(ratio, -1.f, 1.f);
	float radians = acosf(ratio);

	return ConvertRadiansToDegrees(radians);
}


//-----------------------------------------------------------------------------------------------
// Calculates the sine of an angle in degrees and returns it
//
float SinDegrees(float degrees) 
{
	float radians = ConvertDegreesToRadians(degrees);

	return sinf(radians);
}


//-----------------------------------------------------------------------------------------------
// Calculates the arcsine, in degrees
//
float ASinDegrees(float ratio)
{
	ratio = ClampFloat(ratio, -1.f, 1.f);
	float radians = asinf(ratio);

	return ConvertRadiansToDegrees(radians);
}


//-----------------------------------------------------------------------------------------------
// Calculates the tan(theta), expressed in degrees
//
float TanDegrees(float degrees)
{
	float radians = ConvertDegreesToRadians(degrees);

	return tanf(radians);
}


//-----------------------------------------------------------------------------------------------
// Calculates the arctan(y,x), expressed in degrees
//
float Atan2Degrees(float y, float x) 
{
	float radians = atan2f(y, x);

	return ConvertRadiansToDegrees(radians);
}


//-----------------------------------------------------------------------------------------------
// Calculates the arctan of the side ratio given by ratio, expressed in degrees
//
float Atan2Degrees(float ratio)
{
	float radians = atan2f(ratio, 1.f);

	return ConvertRadiansToDegrees(radians);
}


//-----------------------------------------------------------------------------------------------
// Returns the nearest cardinal direction to the given angle
//
float GetNearestCardinalAngle(float angle)
{
	float rightDistance		= abs(GetAngularDisplacement(angle, 0.f));
	float upDistance		= abs(GetAngularDisplacement(angle, 90.f));
	float leftDistance		= abs(GetAngularDisplacement(angle, 180.f));
	float downDistance		= abs(GetAngularDisplacement(angle, 270.f));

	float minDistance = MinFloat(rightDistance, upDistance, leftDistance, downDistance);

	// Return the direction corresponding to the min distance
	if		(minDistance == rightDistance)		{ return 0.f;}
	else if	(minDistance == leftDistance)		{ return 180.f; }
	else if	(minDistance == upDistance)			{ return 90.f; }
	else										{ return 270.f; }
}


//-----------------------------------------------------------------------------------------------
// Returns the nearest inter-cardinal angle to the one given
//
float GetNearestInterCardinalAngle(float angle)
{
	float neDistance		= abs(GetAngularDisplacement(angle, 45.f));
	float nwDistance		= abs(GetAngularDisplacement(angle, 135.f));
	float swDistance		= abs(GetAngularDisplacement(angle, 225.f));
	float seDistance		= abs(GetAngularDisplacement(angle, 315.f));

	float minDistance = MinFloat(neDistance, nwDistance, swDistance, seDistance);

	// Return the direction corresponding to the min distance
	if		(minDistance == neDistance)		{ return 45.f;}
	else if	(minDistance == nwDistance)		{ return 135.f; }
	else if	(minDistance == swDistance)		{ return 225.f; }
	else									{ return 315.f; }
}


//-----------------------------------------------------------------------------------------------
// Returns the nearest cardinal direction to the one given
//
Vector2 GetNearestCardinalDirection(const Vector2& direction)
{
	float northDot = DotProduct(direction, Vector2::Y_AXIS);
	float southDot = DotProduct(direction, Vector2::MINUS_Y_AXIS);
	float eastDot = DotProduct(direction, Vector2::X_AXIS);
	float westDot = DotProduct(direction, Vector2::MINUS_X_AXIS);

	float maxDot = MaxFloat(northDot, southDot, eastDot, westDot);

	if		(maxDot == northDot)		{ return Vector2::Y_AXIS;}
	else if	(maxDot == southDot)		{ return Vector2::MINUS_Y_AXIS; }
	else if	(maxDot == eastDot)			{ return Vector2::X_AXIS; }
	else								{ return Vector2::MINUS_X_AXIS; }
}


//-----------------------------------------------------------------------------------------------
//  Returns the nearest cardinal direction to the one corresponding with angle
//
Vector2 GetNearestCardinalDirection(float angle)
{
	Vector2 direction = Vector2::MakeDirectionAtDegrees(angle);
	return GetNearestCardinalDirection(direction);
}


//-----------------------------------------------------------------------------------------------
// Returns the equivalent angle between -180 and 180 degrees
//
float GetAngleBetweenMinusOneEightyAndOneEighty(float angleDegrees)
{
	while (angleDegrees > 180.f)
	{
		angleDegrees -= 360.f;
	}

	while (angleDegrees < -180.f)
	{
		angleDegrees += 360.f;
	}

	return angleDegrees;
}


//-----------------------------------------------------------------------------------------------
// Returns the angle coterminal to angle between 0 and 360 degrees
//
float GetAngleBetweenZeroThreeSixty(float angleDegrees)
{
	while (angleDegrees > 360.f)
	{
		angleDegrees -= 360.f;
	}

	while (angleDegrees < 0.f)
	{
		angleDegrees += 360.f;
	}

	return angleDegrees;
}


//-----------------------------------------------------------------------------------------------
// Generates a random float between zero and one (inclusively) and returns it
//
float GetRandomFloatZeroToOne()
{
	return static_cast<float>(rand()) * (1.f / static_cast<float>(RAND_MAX));
}


//-----------------------------------------------------------------------------------------------
// Generates a random float between minInclusive and maxInclusive and returns it
//
float GetRandomFloatInRange(float minInclusive, float maxInclusive)
{
	float ratio = GetRandomFloatZeroToOne();

	return ((maxInclusive - minInclusive) * ratio) + minInclusive;
}


//-----------------------------------------------------------------------------------------------
// Generates a random integer between zero (inclusive) and maxNotInclusive
//
int GetRandomIntLessThan(int maxNotInclusive)
{
	return rand() % maxNotInclusive;
}


//-----------------------------------------------------------------------------------------------
// Generates a random integer between minInclusive and maxInclusive and returns it
//
int GetRandomIntInRange(int minInclusive, int maxInclusive)
{
	return (rand() % (maxInclusive - minInclusive + 1)) + minInclusive; 
}


//-----------------------------------------------------------------------------------------------
// Randomly returns true or false (essentially a coin flip)
//
bool GetRandomTrueOrFalse()
{
	return ((rand() % 2) == 0);
}


//-----------------------------------------------------------------------------------------------
// Returns true based on the percent chance of chanceForSuccess
//
bool CheckRandomChance(float chanceForSuccess)
{
	if (chanceForSuccess >= 1.f)
	{
		return true;
	}
	else if (chanceForSuccess <= 0.f)
	{
		return false;
	}
	else
	{
		float outcome = GetRandomFloatZeroToOne();
		return (outcome <= chanceForSuccess);
	}
}

//-----------------------------------------------------------------------------------------------
// Returns a random point within the unit circle
//
Vector2 GetRandomPointWithinCircle()
{
	float randomMagnitude = GetRandomFloatInRange(0.1f, 0.9f);
	return Vector2::GetRandomVector(randomMagnitude);
}


//-----------------------------------------------------------------------------------------------
// Rounds inValue to the nearest integer, with 0.5 rounding to 1, -0.5 rounding to 0
//
Vector3 GetRandomPointOnSphere()
{
	float theta = GetRandomFloatInRange(0.f, 360.f);
	float phi = GetRandomFloatInRange(0.f, 360.f);

	return SphericalToCartesian(1, theta, phi);
}


//-----------------------------------------------------------------------------------------------
// Returns a random point within a unit sphere, excluding the boundary
//
Vector3 GetRandomPointWithinSphere()
{
	Vector3 boundaryVector = GetRandomPointOnSphere();
	float randomMagnitude = GetRandomFloatInRange(0.1f, 0.9f);

	return randomMagnitude * boundaryVector;
}


//-----------------------------------------------------------------------------------------------
// Rounds inValue to the nearest integer, with 0.5 rounding to 1, -0.5 rounding to 0
//
int RoundToNearestInt(float inValue)
{
	int castedInt = static_cast<int>(inValue);
	float fraction = (inValue - castedInt);

	int toReturn = castedInt;

	if (fraction >= 0.5)
	{
		toReturn = castedInt + 1;
	}

	if (fraction < -0.5f)
	{
		toReturn = castedInt - 1;
	}
	
	return toReturn;
}


//-----------------------------------------------------------------------------------------------
// Clamps inValue to be between minInclusive and maxInclusive
//
int ClampInt(int inValue, int minInclusive, int maxInclusive)
{
	if (inValue > maxInclusive)
	{
		return maxInclusive;
	}
	else if (inValue < minInclusive)
	{
		return minInclusive;
	}
	else
	{
		return inValue;
	}
}


//-----------------------------------------------------------------------------------------------
// Clamps inValue to be between minInclusive and maxInclusive
//
float ClampFloat(float inValue, float minInclusive, float maxInclusive)
{
	if (inValue > maxInclusive)
	{
		return maxInclusive;
	}
	else if (inValue < minInclusive)
	{
		return minInclusive;
	}
	else
	{
		return inValue;
	}
}


//-----------------------------------------------------------------------------------------------
// Clamps inValue to be between 0.f and 1.f
//
float ClampFloatZeroToOne(float inValue)
{
	return ClampFloat(inValue, 0.f, 1.f);
}


//-----------------------------------------------------------------------------------------------
// Clamps inValue to be between -1.f and 1.f
//
float ClampFloatNegativeOneToOne(float inValue)
{
	return ClampFloat(inValue, -1.f, 1.f);
}


//-----------------------------------------------------------------------------------------------
// Returns the given vector clamped between the two bounds
//
Vector2 ClampVector2(const Vector2& inValue, const Vector2& minInclusive, const Vector2& maxInclusive)
{
	Vector2 result;
	result.x = ClampFloat(inValue.x, minInclusive.x, maxInclusive.x);
	result.y = ClampFloat(inValue.y, minInclusive.y, maxInclusive.y);

	return result;
}


//-----------------------------------------------------------------------------------------------
// Returns the given vector with all components clamped between min and max inclusive
//
Vector3 ClampVector3(const Vector3& inValue, float minInclusive, float maxInclusive)
{
	Vector3 result;

	result.x = ClampFloat(inValue.x, minInclusive, maxInclusive);
	result.y = ClampFloat(inValue.y, minInclusive, maxInclusive);
	result.z = ClampFloat(inValue.z, minInclusive, maxInclusive);

	return result;
}


//-----------------------------------------------------------------------------------------------
// Finds the % (as fraction) of inValue in [rangeStart, rangeEnd]
//
float GetFractionInRange(float inValue, float rangeStart, float rangeEnd)
{
	float offsetIntoRange = (inValue - rangeStart);
	float rangeSize = (rangeEnd - rangeStart);
	return (offsetIntoRange / rangeSize);
}


//-----------------------------------------------------------------------------------------------
// Maps inValue from an inRange to an outRange
//
float RangeMapFloat(float inValue, float inStart, float inEnd, float outStart, float outEnd)
{
	// If in range has size 0, just return the average of the out range
	if (inStart == inEnd)
	{
		return (outStart + outEnd) * 0.5f;
	}

	float inRange = inEnd - inStart;
	float outRange = outEnd - outStart;

	float inRelativeToStart = inValue - inStart;

	float fractionIntoRange = inRelativeToStart / inRange;
	float outRelativeToStart = fractionIntoRange * outRange;

	return outRelativeToStart + outStart;
}


//-----------------------------------------------------------------------------------------------
// Maps the in Vector2 from the input range to the output range
//
Vector2 RangeMap(Vector2 inValue, Vector2 inStart, Vector2 inEnd, Vector2 outStart, Vector2 outEnd)
{
	float x = RangeMapFloat(inValue.x, inStart.x, inEnd.x, outStart.x, outEnd.x);
	float y = RangeMapFloat(inValue.y, inStart.y, inEnd.y, outStart.y, outEnd.y);

	return Vector2(x, y);
}


//-----------------------------------------------------------------------------------------------
// Returns the angular displacement from start to end, taking the shorter path
//
float GetAngularDisplacement(float startDegrees, float endDegrees)
{
	float angularDisp = (endDegrees - startDegrees);

	// Increment/decrement the displacement to represent the shorter turn direction
	while (angularDisp > 180)
	{
		angularDisp -= 360;
	}

	while (angularDisp < -180)
	{
		angularDisp += 360;
	}

	return angularDisp;
}


//-----------------------------------------------------------------------------------------------
// Returns an angle that is at most maxTurnDegrees from currentDegrees towards goalDegrees, in
// the direction of the shortest path. If current and goal are within maxTurn from eachother,
// goal is returned.
//
float TurnToward(float currentDegrees, float goalDegrees, float maxTurnDegrees)
{
	float angularDisplacement = GetAngularDisplacement(currentDegrees, goalDegrees);

	if (abs(angularDisplacement) <= maxTurnDegrees)
	{
		return goalDegrees;
	}
	
	float directionToTurn = (angularDisplacement / abs(angularDisplacement));
	float result = ((directionToTurn * maxTurnDegrees) + currentDegrees);

	return result;
}


//-----------------------------------------------------------------------------------------------
// Calculates the dot product between a and b
//
float DotProduct(const Vector2& a, const Vector2& b)
{
	return ((a.x * b.x) + (a.y * b.y));
}


//-----------------------------------------------------------------------------------------------
// Calculates the dot product between a and b
//
float DotProduct(const Vector3& a, const Vector3& b)
{
	return ((a.x * b.x) + (a.y * b.y) + (a.z * b.z));
}


//-----------------------------------------------------------------------------------------------
// Returns the dot product between a and b
//
float DotProduct(const Vector4& a, const Vector4& b)
{
	return ((a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w));
}


//-----------------------------------------------------------------------------------------------
// Returns the inner (dot) product between a and b
//
float DotProduct(const Quaternion& a, const Quaternion& b)
{
	return (a.s * b.s) + (a.v.x * b.v.x) + (a.v.y * b.v.y) + (a.v.z * b.v.z);
}


//-----------------------------------------------------------------------------------------------
// Returns the cross product between a and b
//
Vector3 CrossProduct(const Vector3& a, const Vector3& b)
{
	Vector3 result;

	result.x = (a.y * b.z) - (a.z * b.y);
	result.y = (a.z * b.x) - (a.x * b.z);
	result.z = (a.x * b.y) - (a.y * b.x);

	return result;
}


//-----------------------------------------------------------------------------------------------
// Reflects the incident vector on the plane defined by normal
//
Vector3 Reflect(const Vector3& incidentVector, const Vector3& normal)
{
	Vector3 alongNormal = DotProduct(incidentVector, normal) * normal;

	return incidentVector - 2 * alongNormal;
}


//-----------------------------------------------------------------------------------------------
// Attempts to refract the incident vector through the surface defined by normal, returining it in out_refracedVector
// Returns true if the vector was refracted, false otherwise (See Snell's Law)
//
bool Refract(const Vector3& incidentVector, const Vector3& normal, float niOverNt, Vector3& out_refractedVector)
{
	Vector3 normalizedIncident = incidentVector.GetNormalized();

	float dt = DotProduct(normalizedIncident, normal);
	float discriminant = 1.0f - niOverNt * niOverNt * (1.0f - dt * dt);

	if (discriminant > 0) // Can be refracted
	{
		out_refractedVector = niOverNt * (normalizedIncident - normal * dt) - normal * sqrtf(discriminant);
		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------------------------
// Checks to see if the bits are set in the 8 bit flag
//
bool AreBitsSet(unsigned char bitFlags8, unsigned char flagsToCheck)
{
	return ((bitFlags8 & flagsToCheck) == flagsToCheck);
}


//-----------------------------------------------------------------------------------------------
// Checks to see if the bits are set in the 32 bit flag
//
bool AreBitsSet(unsigned int bitFlags32, unsigned int flagsToCheck)
{
	return ((bitFlags32 & flagsToCheck) == flagsToCheck);
}


//-----------------------------------------------------------------------------------------------
// Sets the bits indicated by flagsToSet in the 8 bit flag
//
void SetBits(unsigned char& bitFlags8, unsigned char flagsToSet)
{
	bitFlags8 |= flagsToSet;
}


//-----------------------------------------------------------------------------------------------
// Sets the bits indicated by flagsToSet in the 32 bit flag
//
void SetBits(unsigned int& bitFlags32, unsigned int flagsToSet)
{
	bitFlags32 |= flagsToSet;
}


//-----------------------------------------------------------------------------------------------
// Clears the bits indicated by flagsToSet in the 8 bit flag
//
void ClearBits(unsigned char& bitFlags8, unsigned char flagsToClear)
{
	unsigned char bitMask = ~flagsToClear;
	bitFlags8 &= bitMask;
}


//-----------------------------------------------------------------------------------------------
// Clears the bits indicated by flagsToSet in the 32 bit flag
//
void ClearBits(unsigned int& bitFlags32, unsigned int flagsToClear)
{
	unsigned int bitMask = ~flagsToClear;
	bitFlags32 &= bitMask;
}


//-----------------------------------------------------------------------------------------------
// Returns the int given with the bits reversed (i.e. 0010010 -> 0100100)
//
uint32_t GetBitsReversed(const uint32_t& bits)
{
	uint32_t reversedBits = 0;

	for (int i = 0; i < 32; ++i)
	{
		uint32_t currBit = bits & (1 << i);

		if (currBit != 0)
		{
			int inverseI = 32 - i - 1;
			reversedBits |= (1 << inverseI);
		}
	}

	return reversedBits;
}


//-----------------------------------------------------------------------------------------------
// 2nd-degree smooth start (a.k.a "quadratic ease in")
//
float SmoothStart2(float t)
{
	return (t * t);
}


//-----------------------------------------------------------------------------------------------
// 3rd-degree smooth start (a.k.a "cubic ease in")
//
float SmoothStart3(float t)
{
	return (t * t * t);
}


//-----------------------------------------------------------------------------------------------
// 4th-degree smooth start (a.k.a "quartic ease in")
//
float SmoothStart4(float t)
{
	return (t * t * t * t);
}


//-----------------------------------------------------------------------------------------------
// 2nd-degree smooth start (a.k.a "quadratic ease out")
//
float SmoothStop2(float t)
{
	float flipped = (1 - t);
	float squaredFlipped = (flipped * flipped);
	float flippedSquaredFlipped = (1 - squaredFlipped);

	return flippedSquaredFlipped;
}


//-----------------------------------------------------------------------------------------------
// 3rd-degree smooth start (a.k.a "cubic ease out")
//
float SmoothStop3(float t)
{
	float flipped = (1 - t);
	float cubedFlipped = (flipped * flipped * flipped);
	float flippedCubedFlipped = (1 - cubedFlipped);

	return flippedCubedFlipped;
}


//-----------------------------------------------------------------------------------------------
// 4th-degree smooth start (a.k.a "quartic ease out")
//
float SmoothStop4(float t)
{
	float flipped = (1 - t);
	float quartedFlipped = (flipped * flipped * flipped * flipped);
	float flippedQuartedFlipped = (1 - quartedFlipped);

	return flippedQuartedFlipped;
}


//-----------------------------------------------------------------------------------------------
// 3rd-degree smooth start/stop (a.k.a "smoothstep")
//
float SmoothStep3(float t)
{
	return ((1 - t) * SmoothStart2(t)) + (t * SmoothStop2(t));
}


//-----------------------------------------------------------------------------------------------
// Finds the float at a certain percent (fraction) in [rangeStart, rangeEnd]
//
float Interpolate(float start, float end, float fractionTowardEnd)
{
	float rangeSize = (end - start);
	float offsetIntoRange = (fractionTowardEnd * rangeSize);
	return (offsetIntoRange + start);
}


//-----------------------------------------------------------------------------------------------
// Finds the int at a certain percent (fraction) in [rangeStart, rangeEnd]
//
int Interpolate(int start, int end, float fractionTowardEnd)
{
	int range = (end - start);
	return start + RoundToNearestInt(fractionTowardEnd * range);
}


//-----------------------------------------------------------------------------------------------
// Finds the unsigned char at a certain percent (fraction) in [rangeStart, rangeEnd]
//
unsigned char Interpolate(unsigned char start, unsigned char end, float fractionTowardEnd)
{
	float range = static_cast<float>(end - start);

	int change = RoundToNearestInt(fractionTowardEnd * range);
	int result = static_cast<int>(start) + change;
	result = ClampInt(result, 0, 255);

	return static_cast<unsigned char>(result);
}


//-----------------------------------------------------------------------------------------------
// Finds the IntVector2 at a certain percent (fraction) in [rangeStart, rangeEnd]
//
const IntVector2 Interpolate(const IntVector2& start, const IntVector2& end, float fractionTowardEnd)
{
	int interpolatedX = Interpolate(start.x, end.x, fractionTowardEnd);
	int interpolatedY = Interpolate(start.y, end.y, fractionTowardEnd);

	return IntVector2(interpolatedX, interpolatedY);
}


//-----------------------------------------------------------------------------------------------
// Finds the IntRange at a certain percent (fraction) in [rangeStart, rangeEnd]
//
const IntRange Interpolate(const IntRange& start, const IntRange& end, float fractionTowardEnd)
{
	int interpolatedMin = Interpolate(start.min, end.min, fractionTowardEnd);
	int interpolatedMax = Interpolate(start.max, end.max, fractionTowardEnd);

	return IntRange(interpolatedMin, interpolatedMax);
}


//-----------------------------------------------------------------------------------------------
// Finds the Rgba at a certain percent (fraction) in [rangeStart, rangeEnd]
//
const Rgba Interpolate(const Rgba& start, const Rgba& end, float fractionTowardEnd)
{
	unsigned char interpolatedR = Interpolate(start.r, end.r, fractionTowardEnd);
	unsigned char interpolatedG = Interpolate(start.g, end.g, fractionTowardEnd);
	unsigned char interpolatedB = Interpolate(start.b, end.b, fractionTowardEnd);
	unsigned char interpolatedA = Interpolate(start.a, end.a, fractionTowardEnd);

	return Rgba(interpolatedR, interpolatedG, interpolatedB, interpolatedA);
}


//-----------------------------------------------------------------------------------------------
// Finds the IntVector3 between start and end given the fractional parameter
//
const IntVector3 Interpolate(const IntVector3& start, const IntVector3& end, float fractionTowardEnd)
{
	int interpolatedX = Interpolate(start.x, end.x, fractionTowardEnd);
	int interpolatedY = Interpolate(start.y, end.y, fractionTowardEnd);
	int interpolatedZ = Interpolate(start.z, end.z, fractionTowardEnd);

	return IntVector3(interpolatedX, interpolatedY, interpolatedZ);
}


//-----------------------------------------------------------------------------------------------
// Finds the roots of the quadratic function given by the coefficients a, b, and c, and stores them
// in solutions
// Returns true if roots were found, false otherwise
//
bool Quadratic(Vector2& out_solutions, float a, float b, float c)
{
	// (-b +- sqrt(b^2 - 4ac)) / (2a)

	// First determine the inside of the square root - if it is negative, then there are no real solutions
	float insideSqrt = (b * b) - (4 * a * c);

	if (insideSqrt < 0)
	{
		return false;
	}

	// There is at least one solution
	float sqrtValue = sqrtf(insideSqrt);

	float firstSolution = (-b + sqrtValue) / (2 * a);
	float secondSolution = (-b - sqrtValue) / (2 * a);

	// Order the solutions in order of magnitude
	if (firstSolution < secondSolution)
	{
		out_solutions.x = firstSolution;
		out_solutions.y = secondSolution;
	}
	else
	{
		out_solutions.x = secondSolution;
		out_solutions.y = firstSolution;
	}

	return true;
}


//-----------------------------------------------------------------------------------------------
// Returns the min of 2 ints
//
int MinInt(int a, int b)
{
	return (a < b ? a : b);
}




//-----------------------------------------------------------------------------------------------
// Returns the min of 2 unsigned ints
//
unsigned int MinInt(unsigned int a, unsigned int b)
{
	return (a < b ? a : b);
}


//-----------------------------------------------------------------------------------------------
// Returns the max of 2 unsigned ints
//
unsigned int MaxInt(unsigned int a, unsigned int b)
{
	return (a > b ? a : b);
}


//-----------------------------------------------------------------------------------------------
// Returns the max of a and b
//
int MaxInt(int a, int b)
{
	return (a > b ? a : b);
}


//-----------------------------------------------------------------------------------------------
// Returns the max of 4 floats
//
float MaxFloat(float a, float b, float c, float d)
{
	float maxAB = ((a > b) ? a : b);
	float maxCD = ((c > d) ? c : d);

	return ((maxAB > maxCD) ? maxAB : maxCD);
}


//-----------------------------------------------------------------------------------------------
// Returns the max of 4 floats
//
float MaxFloat(float a, float b)
{
	return ((a > b) ? a : b);
}


//-----------------------------------------------------------------------------------------------
// Returns the min of 4 floats
//
float MinFloat(float a, float b, float c, float d)
{
	float minAB = ((a < b) ? a : b);
	float minCD = ((c < d) ? c : d);

	return ((minAB < minCD) ? minAB : minCD);
}


//-----------------------------------------------------------------------------------------------
// Returns the min of 2 floats
//
float MinFloat(float a, float b)
{
	return ((a < b) ? a : b);
}


//-----------------------------------------------------------------------------------------------
// Returns the absolute value of the integer inValue
//
int AbsoluteValue(int inValue)
{
	if (inValue < 0)
	{
		inValue *= -1;
	}

	return inValue;
}


//-----------------------------------------------------------------------------------------------
// Returns the absolute value of the float inValue
//
float AbsoluteValue(float inValue)
{
	if (inValue < 0.f)
	{
		inValue *= -1.f;
	}

	return inValue;
}


//-----------------------------------------------------------------------------------------------
// Returns the component-wise absolute value of the given Vector3
//
Vector3 AbsoluteValue(const Vector3& inValue)
{
	Vector3 result;
	result.x = AbsoluteValue(inValue.x);
	result.y = AbsoluteValue(inValue.y);
	result.z = AbsoluteValue(inValue.z);

	return result;
}


//-----------------------------------------------------------------------------------------------
// Returns true if the difference between each component is less than or equal to epsilon
//
bool AreMostlyEqual(const Vector3& a, const Vector3& b, float epsilon /*= 0.0001f*/)
{
	Vector3 difference = (a - b);

	bool mostlyEqual = (AbsoluteValue(difference.x) <= epsilon) && (AbsoluteValue(difference.y) <= epsilon) && (AbsoluteValue(difference.z) <= epsilon);

	return mostlyEqual;
}


//-----------------------------------------------------------------------------------------------
// Returns true if the difference between the floats are less than or equal to epsilon
//
bool AreMostlyEqual(float a, float b, float epsilon /*= 0.0001f*/)
{
	float difference = b - a;

	bool mostlyEqual = (AbsoluteValue(difference) <= epsilon);
	return mostlyEqual;
}


//-----------------------------------------------------------------------------------------------
// Returns true if the two matrices are equal within an epsilon error, false otherwise
//
bool AreMostlyEqual(const Matrix44& a, const Matrix44& b, float epsilon /*= 0.0001f*/)
{
	bool equal =   AreMostlyEqual(a.Ix, b.Ix, epsilon) 
				&& AreMostlyEqual(a.Iy, b.Iy, epsilon)
				&& AreMostlyEqual(a.Iz, b.Iz, epsilon)
				&& AreMostlyEqual(a.Iw, b.Iw, epsilon)
				&& AreMostlyEqual(a.Jx, b.Jx, epsilon)
				&& AreMostlyEqual(a.Jy, b.Jy, epsilon)
				&& AreMostlyEqual(a.Jz, b.Jz, epsilon)
				&& AreMostlyEqual(a.Jw, b.Jw, epsilon)
				&& AreMostlyEqual(a.Kx, b.Kx, epsilon)
				&& AreMostlyEqual(a.Ky, b.Ky, epsilon)
				&& AreMostlyEqual(a.Kz, b.Kz, epsilon)
				&& AreMostlyEqual(a.Kw, b.Kw, epsilon)
				&& AreMostlyEqual(a.Tx, b.Tx, epsilon)
				&& AreMostlyEqual(a.Ty, b.Ty, epsilon)
				&& AreMostlyEqual(a.Tz, b.Tz, epsilon)
				&& AreMostlyEqual(a.Tw, b.Tw, epsilon);

	return equal;
}


//-----------------------------------------------------------------------------------------------
// Returns true if the two spheres defined by the parameters overlap, false otherwise
//
bool DoSpheresOverlap(const Vector3& posA, float radiusA, const Vector3& posB, float radiusB)
{
	float distanceSquared = (posA - posB).GetLengthSquared();
	float radiiSquared = (radiusA + radiusB) * (radiusA + radiusB);

	return (distanceSquared <= radiiSquared);
}


//-----------------------------------------------------------------------------------------------
// Returns whether the given box and sphere overlap
//
bool DoesBoxSphereOverlap(const AABB3& boxBounds, const Vector3& sphereCenter, float sphereRadius)
{
	Vector3 boxCenter = boxBounds.GetCenter();

	if (boxBounds.ContainsPoint(sphereCenter))
	{
		return true;
	}

	Vector3 directionToBox = (boxCenter - sphereCenter);
	float distance = directionToBox.NormalizeAndGetLength();

	float magnitude = MinFloat(sphereRadius, distance);

	Vector3 closestPosition = sphereCenter + (magnitude * directionToBox);

	return boxBounds.ContainsPoint(closestPosition);
}
