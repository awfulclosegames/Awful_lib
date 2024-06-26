// Fill out your copyright notice in the Description page of Project Settings.


#include "Spline/SDZ_KBSpline_DataTypes.h"
#include "Spline/KBSpline_Utilis.h"

UKBSplineConfig::UKBSplineConfig(FVector Location)
	: Super()
{
	OriginPoint.Location = Location;
}

void UKBSplineConfig::UpdateWorkingSet()
{
	// trim old values
	int CurrentCount = WorkingSet.Num();
	if (CurrentCount > 2)
	{
		if (CurrentCount > 3)
		{
			WorkingSet.RemoveAt(1);
		}
		WorkingSet.RemoveAt(0);
	}
	else
	{
		WorkingSet.Empty();
	}
	int ToAdd = 4 - WorkingSet.Num();
	// try and restock from the control point buffer

	for (int i = 0; i < ToAdd; ++i)
	{
		FKBSplinePoint Point;
		if (ControlPoints.Dequeue(Point))
		{
			WorkingSet.Add(Point);
		}
	}
}

void UKBSplineConfig::Reset()
{
	ControlPoints.Empty();
	SegmentBounds.Empty();
	WorkingSet.Empty();
}


FKBSplineState::FKBSplineState()
	: PrecomputedCoefficients({ {0.0f,0.0f,0.0f}, 
								{0.0f,0.0f,0.0f},
								{0.0f,0.0f,0.0f} })
{
}


