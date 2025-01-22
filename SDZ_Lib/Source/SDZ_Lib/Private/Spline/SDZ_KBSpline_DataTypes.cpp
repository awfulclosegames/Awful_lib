// Copyright Strati D. Zerbinis, 2025. All Rights Reserved.


#include "Spline/SDZ_KBSpline_DataTypes.h"
#include "Spline/KBSpline_Utilis.h"

// 20x 4 point segments, overkill as a default but still not a ton of memory 
const int UKBSplineConfig::sDefaultBufferLength = 80;

UKBSplineConfig::UKBSplineConfig(FVector Location)
	: Super()
	, ControlPoints(sDefaultBufferLength)
{
	OriginPoint.Location = Location;
}


UKBSplineConfig::UKBSplineConfig(FVector Location, int NumPoints)
	: Super()
	, ControlPoints(NumPoints)
{
	OriginPoint.Location = Location;
}

void UKBSplineConfig::PeekSegment(int SegmentID, FKBSplinePoint Points[4]) const
{
	int normalizedID = NormalizeSegmentID(SegmentID);
	if (ControlPoints.Num() > normalizedID + 2)
	{
		// discrete points since the ring buffer might wrap so a contiguous series of points might be non-contiguous in memory
		Points[FKBSplineState::PreviousPoint] = ControlPoints[normalizedID - 1];
		Points[FKBSplineState::FromPoint] = ControlPoints[normalizedID];
		Points[FKBSplineState::ToPoint] = ControlPoints[normalizedID + 1];
		Points[FKBSplineState::NextPoint] = ControlPoints[normalizedID + 2];
	}
}

void UKBSplineConfig::ConsumeSegment(int SegmentID)
{
	if (IsValidSegment(SegmentID))
	{
		int normalizedID = NormalizeSegmentID(SegmentID);
		if (ControlPoints.Num() > normalizedID + 2)
		{
			ControlPoints.PopFront();
		}
		++MinimumSegment;
	}
}

void UKBSplineConfig::GetTravelChord(int SegmentID, FVector& outChord)
{
	if (IsValidSegment(SegmentID))
	{
		int normalizedID = NormalizeSegmentID(SegmentID);
		outChord = ControlPoints[normalizedID + 1].Location - ControlPoints[normalizedID].Location;

		return;
	}
	outChord = FVector(0.0f);
}

bool UKBSplineConfig::IsValidSegment(int SegmentID) const
{
	int normalizedID = NormalizeSegmentID(SegmentID);
	return normalizedID > 0 && normalizedID < (ControlPoints.Num() - 2);
}

void UKBSplineConfig::Add(FKBSplinePoint& Point)
{
	ControlPoints.Add(Point);
}

int UKBSplineConfig::GetLastSegment() const
{
	return ControlPoints.Num() + MinimumSegment;
}

void UKBSplineConfig::ClearToCommitments()
{
	int Length = FMath::Max(0, ControlPoints.Num() - CommitPoint);
	ControlPoints.Pop(Length);
}

void UKBSplineConfig::Reset()
{
	CommitPoint = 0;
	ControlPoints.Reset();
}

int UKBSplineConfig::GetNextCandidateSegment(int SegmentID) const
{
	if (IsValidSegment(SegmentID))
	{
		return ++SegmentID;
	}
	return MinimumSegment + 1;
}

FKBSplineState::FKBSplineState()
	: PrecomputedCoefficients({ {0.0f,0.0f,0.0f}, 
								{0.0f,0.0f,0.0f},
								{0.0f,0.0f,0.0f} })
{
}

void FKBSplineState::Reset()
{
	Tau[0] = -1.0f;
	Tau[1] = -1.0f;
	Beta[0] = 0.0f;
	Beta[1] = 0.0f;
	Gamma[0] = 0.0f;
	Gamma[1] = 0.0f;
}

bool FKBSplineState::IsValidSegment() const
{
	return false;
}

