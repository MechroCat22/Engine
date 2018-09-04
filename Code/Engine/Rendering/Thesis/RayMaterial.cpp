/************************************************************************/
/* File: RayMaterial.cpp
/* Author: Andrew Chase
/* Date: September 4th, 2018
/* Description: Implementation of the RayMaterial classes
/************************************************************************/
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Rendering/Thesis/Ray.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Rendering/Thesis/Hitable.hpp"
#include "Engine/Rendering/Thesis/RayMaterial.hpp"


//-----------------------------------------------------------------------------------------------
// Diffuse Constructor
//
RayMaterial_Diffuse::RayMaterial_Diffuse(const Vector3& albedo)
	: m_albedoColor(albedo)
{
}


//-----------------------------------------------------------------------------------------------
// Returns the albedo color and scatters the ray in a random direction based on the normal of the surface
//
bool RayMaterial_Diffuse::Scatter(const Ray& incomingRay, const HitRecord_t& record, Vector3& out_attentuation, Ray& out_scatteredRay)
{
	UNUSED(incomingRay);
	Vector3 target = record.position + record.normal + GetRandomPointWithinSphere();
	out_scatteredRay = Ray(record.position, target - record.position);
	out_attentuation = m_albedoColor;
	return true;
}


//-----------------------------------------------------------------------------------------------
// Metal Constructor
//
RayMaterial_Metal::RayMaterial_Metal(const Vector3& albedo, float fuzziness /*= 0.f*/)
	: m_albedoColor(albedo)
	, m_fuzziness(fuzziness)
{
}


//-----------------------------------------------------------------------------------------------
// Returns the albedo color and scatters the ray by reflection, using the fuzziness factor for some
// variable results
//
bool RayMaterial_Metal::Scatter(const Ray& incomingRay, const HitRecord_t& record, Vector3& out_attentuation, Ray& out_scatteredRay)
{
	Vector3 reflectedDirection = Reflect(incomingRay.GetDirection().GetNormalized(), record.normal);
	out_scatteredRay = Ray(record.position, reflectedDirection + m_fuzziness * GetRandomPointWithinSphere());
	out_attentuation = m_albedoColor;
	return (DotProduct(reflectedDirection, record.normal) > 0);
}


//-----------------------------------------------------------------------------------------------
// Dielectric Constructor
//
RayMaterial_Dielectric::RayMaterial_Dielectric(float indexOfRefraction)
	: m_indexOfRefraction(indexOfRefraction)
{
}


//-----------------------------------------------------------------------------------------------
// Scatters the ray by determining if the ray should refract or reflect
//
bool RayMaterial_Dielectric::Scatter(const Ray& incomingRay, const HitRecord_t& record, Vector3& out_attentuation, Ray& out_scatteredRay)
{
	Vector3 outwardNormal;
	Vector3 reflected = Reflect(incomingRay.GetDirection(), record.normal);

	float niOverNt;
	out_attentuation = Vector3(1.f, 1.f, 1.f);
	Vector3 refracted;

	float reflectionProbability;
	float cosine;

	// Check if we're within or outside the surface
	if (DotProduct(incomingRay.GetDirection(), record.normal) > 0)
	{
		outwardNormal = -1.f * record.normal;
		niOverNt = m_indexOfRefraction;
		cosine = m_indexOfRefraction * DotProduct(incomingRay.GetDirection(), record.normal) / incomingRay.GetDirection().GetLength();
	}
	else
	{
		outwardNormal = record.normal;
		niOverNt = 1.0f / m_indexOfRefraction;
		cosine = -1.0f * DotProduct(incomingRay.GetDirection(), record.normal) / incomingRay.GetDirection().GetLength();
	}

	// Check for reflect or refract
	if (Refract(incomingRay.GetDirection(), outwardNormal, niOverNt, refracted))
	{
		reflectionProbability = GetSchlickApproximation(cosine, m_indexOfRefraction);
	}
	else
	{
		reflectionProbability = 1.0f;
	}

	// Randomly reflect based on the schlick approximation
	if (CheckRandomChance(reflectionProbability))
	{
		out_scatteredRay = Ray(record.position, reflected);
	}
	else
	{
		out_scatteredRay = Ray(record.position, refracted);
	}

	return true;
}


//-----------------------------------------------------------------------------------------------
// Returns the Schlick Approximation for whether a ray should reflect or refract on a surface
//
float RayMaterial_Dielectric::GetSchlickApproximation(float cosine, float indexOfRefraction)
{
	float r0 = (1.0f - indexOfRefraction) / (1.0f + indexOfRefraction);
	r0 = r0 * r0;

	return r0 + (1.0f - r0) * Pow((1 - cosine), 5.f);
}
