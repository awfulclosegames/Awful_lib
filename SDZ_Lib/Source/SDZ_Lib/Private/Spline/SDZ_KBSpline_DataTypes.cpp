// Fill out your copyright notice in the Description page of Project Settings.


#include "Spline/SDZ_KBSpline_DataTypes.h"
#include "Spline/KBSpline_Utilis.h"

UKBSplineConfig::UKBSplineConfig(FVector Location)
	: Super()
{
	//OriginPoint.Location = Location;
}

void UKBSplineConfig::UpdateWorkingSet(FKBSplineState& State)
{
	// trim old values
	int CurrentCount = State.WorkingSet.Num();
	if (CurrentCount > 2)
	{
		if (CurrentCount > 3)
		{
			State.WorkingSet.RemoveAt(1);
		}
		State.WorkingSet.RemoveAt(0);
	}
	else
	{
		State.WorkingSet.Empty();
	}
	int ToAdd = 4 - State.WorkingSet.Num();
	// try and restock from the control point buffer

	for (int i = 0; i < ToAdd; ++i)
	{
		FKBSplinePoint Point;
		if (ControlPoints.Dequeue(Point))
		{
			State.WorkingSet.Add(Point);
		}
	}
}

void UKBSplineConfig::Reset()
{
	ControlPoints.Empty();
	SegmentBounds.Empty();
}


FKBSplineState::FKBSplineState()
	: PrecomputedCoefficients({ {0.0f,0.0f,0.0f}, 
								{0.0f,0.0f,0.0f},
								{0.0f,0.0f,0.0f} })
{
}

void FKBSplineState::Reset()
{
	for (int i = 0; i < 2; ++i)
	{
		Tau[i] = 0.0f;
		Beta[i] = 0.0f;
		OriginalUndulationTimes[i] = -1.0f;
	}
	WorkingSet.Empty();
	for (int i = 0; i < 4; ++i)
	{
		OriginalCoeffs[i] = { 0.0f,0.0f,0.0f };
	}
}

bool FKBSplineState::IsValidSegment() const
{
	return WorkingSet.Num() == 4;
}


