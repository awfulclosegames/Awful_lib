// Fill out your copyright notice in the Description page of Project Settings.


#include "Spline/SDZ_KBSpline_DataTypes.h"
#include "Spline/KBSpline_Utilis.h"

UKBSplineConfig::UKBSplineConfig(FVector Location)
	: Super()
{
	OriginPoint.Location = Location;
}


FKBSplineState::FKBSplineState()
	: PrecomputedCoefficients({ {0.0f,0.0f,0.0f}, 
								{0.0f,0.0f,0.0f},
								{0.0f,0.0f,0.0f} })
	//, Tau({ 0.0f,0.0f})
	//, Beta({ 0.0f,0.0f })
{
}

