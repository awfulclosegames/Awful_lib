// Fill out your copyright notice in the Description page of Project Settings.


#include "Spline/SDZ_KBSpline.h"
#include "Spline/KBSpline_Utilis.h"

static TAutoConsoleVariable<bool> CVarSDZ_SplineDebug(TEXT("sdz.Spline.Debug"), false, TEXT("Enable/Disable debug visualization for the KB Spline"));

UKBSplineConfig* USDZ_KBSpline::CreateSplineConfig(FVector Location)
{
	if (UKBSplineConfig* newItem = NewObject<UKBSplineConfig>())
	{
		// anchor it on the given location and set that as the *base* point for all splines
		newItem->OriginPoint.Location = Location;
		newItem->ControlPoints.Add(newItem->OriginPoint);
		return newItem;
	}
	return nullptr;
}

int USDZ_KBSpline::AddSplinePoint(UKBSplineConfig* Config, FKBSplinePoint Point)
{
	int Segment = -1;
	if (IsValid(Config))
	{
		Segment = Config->ControlPoints.Num();
		Config->ControlPoints.Add(Point);
	}
	return Segment;
}

void USDZ_KBSpline::AddSegmentConstraint(UKBSplineConfig* Config, FKBSplineBounds Bound, int SegmentID)
{
	if (IsValid(Config))
	{
		Config->SegmentBounds.FindOrAdd(SegmentID) = Bound;
	}
}

FKBSplineState USDZ_KBSpline::PrepareForEvaluation(UKBSplineConfig* Config, int PointID)
{
	if (IsValid(Config) && Config->IsValidSegment(PointID))
	{
		FKBSplineState State;

		State.CurrentTraversalSegment = PointID;
		KBSplineUtils::Prepare(*Config, State);

		return State;
	}

	return FKBSplineState{};
}

FVector USDZ_KBSpline::Sample(FKBSplineState State)
{
	// compute the linear combination of 

	float TimeSquared = State.Time * State.Time;
	float TimeQubed = TimeSquared * State.Time;
	FVector SamplePoint = State.PrecomputedCoefficients[0] * TimeQubed +
		State.PrecomputedCoefficients[1] * TimeSquared +
		State.PrecomputedCoefficients[2] * State.Time +
		State.PrecomputedCoefficients[3];
	return SamplePoint;
}

FVector USDZ_KBSpline::SampleExplicit(FKBSplineState State, float Completion)
{
	// compute the linear combination of 

	float TimeSquared = Completion * Completion;
	float TimeQubed = TimeSquared * Completion;
	FVector SamplePoint = State.PrecomputedCoefficients[0] * TimeQubed +
		State.PrecomputedCoefficients[1] * TimeSquared +
		State.PrecomputedCoefficients[2] * Completion +
		State.PrecomputedCoefficients[3];
	return SamplePoint;
}

void USDZ_KBSpline::DrawDebug(AActor* Actor, const UKBSplineConfig* Config, FKBSplineState State)
{
#if !UE_BUILD_SHIPPING
	if (!CVarSDZ_SplineDebug.GetValueOnGameThread())
		return;
	if (IsValid(Actor) && IsValid(Config) && Config->IsValidSegment(State.CurrentTraversalSegment))
	{
		const FVector TraversalStart = Config->ControlPoints[State.CurrentTraversalSegment].Location;
		const FVector TraversalEnd = Config->ControlPoints[State.CurrentTraversalSegment + 1].Location;
		int CPIdx = State.CurrentTraversalSegment - 1;
		FVector prevPoint = Config->ControlPoints[CPIdx].Location;
		for (int pointNum = 0; pointNum < 4; ++pointNum)
		{
			FVector Point = Config->ControlPoints[CPIdx + pointNum].Location;
			DrawDebugLine(Actor->GetWorld(), prevPoint, Point, FColor::White, false, 10.0f);
			prevPoint = Point;

		}


		float step = 0.01f;
		FVector prev = TraversalStart;
		for (State.Time = 0.0f; State.Time <= 1.0f; State.Time += step)
		{
			FVector sample = Sample(State);
			DrawDebugLine(Actor->GetWorld(), prev, sample, FColor::Blue, false, 1.0f);
			prev = sample;
		}

		// Draw the Undulation times
		FVector TraversalChord = TraversalEnd - TraversalStart;
		FVector TraversalDir = TraversalChord.GetSafeNormal();
		for (float U : State.UndulationTimes)
		{
			if (U <= 0.0f)
			{
				continue;
			}
			FVector ExtremePt = SampleExplicit(State, U);

			FVector EPRelative = ExtremePt - TraversalStart;
			FVector FromPt = TraversalStart + (TraversalDir * EPRelative.Dot(TraversalDir));

			DrawDebugLine(Actor->GetWorld(), FromPt, ExtremePt, FColor::Yellow,false, 1.0f);
		}
	}
#endif
}
