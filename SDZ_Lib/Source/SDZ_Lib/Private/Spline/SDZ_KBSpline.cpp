// Fill out your copyright notice in the Description page of Project Settings.


#include "Spline/SDZ_KBSpline.h"

UKBSplineConfig::UKBSplineConfig(FVector Location)
	: Super()
{
	OriginPoint.Location = Location;
}

void USDZ_KBSpline::AddSplinePoint(UKBSplineConfig* Config, FKBSplinePoint Point)
{
	if (IsValid(Config))
		Config->ControlPoints.Add(Point);
}

FKBSplineState::FKBSplineState()
	: PrecomputedCoefficients({ 0.0f,0.0f,0.0f,0.0f })
	//, Tau({ 0.0f,0.0f})
	//, Beta({ 0.0f,0.0f })
{
}

UKBSplineConfig* USDZ_KBSpline::CreateSplineConfig(FVector Location)
{
	if (UKBSplineConfig* newItem = NewObject<UKBSplineConfig>())
	{
		newItem->OriginPoint.Location = Location;
		return newItem;
	}
	return nullptr;
}

FKBSplineState USDZ_KBSpline::PrepareForEvaluation(UKBSplineConfig* Config, int PointID)
{
	if (IsValid(Config) && Config->IsValidSegment(PointID))
	{
		FKBSplineState State;


	}

	return FKBSplineState{};
}

FVector USDZ_KBSpline::Sample(UKBSplineConfig* Config, FKBSplineState State)
{
	// compute the linear combination of 
	// State.PrecomputedCoefficients[0] * t^3 + 
	//    State.PrecomputedCoefficients[1] * t^2 + 
	//    State.PrecomputedCoefficients[2] * t + 
	//    State.PrecomputedCoefficients[3]  

	return FVector(0.0f);
}