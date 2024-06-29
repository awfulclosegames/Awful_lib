// Fill out your copyright notice in the Description page of Project Settings.


#include "Spline/SDZ_KBSpline.h"
#include "Spline/KBSpline_Utilis.h"

static TAutoConsoleVariable<bool> CVarSDZ_SplineDebug(TEXT("sdz.Spline.Debug"), true, TEXT("Enable/Disable debug visualization for the KB Spline"));

UKBSplineConfig* USDZ_KBSpline::CreateSplineConfig(FVector Location)
{
	if (UKBSplineConfig* newItem = NewObject<UKBSplineConfig>())
	{
		// anchor it on the given location and set that as the *base* point for all splines
		newItem->OriginPoint.Location = Location;
		newItem->ControlPoints.Enqueue(newItem->OriginPoint);
		return newItem;
	}
	return nullptr;
}

void USDZ_KBSpline::AddSplinePoint(UKBSplineConfig* Config, FKBSplinePoint Point)
{
	if (IsValid(Config))
	{
		Config->ControlPoints.Enqueue(Point);
	}
}

//void USDZ_KBSpline::RemoveLastSplinePoint(UKBSplineConfig* Config)
//{
//	if (!Config->ControlPoints.IsEmpty())
//	{
//		Config->ControlPoints.RemoveAtSwap(Config->ControlPoints.Num() - 1);
//	}
//}

void USDZ_KBSpline::Reset(UKBSplineConfig* Config)
{
	Config->Reset();
}

//void USDZ_KBSpline::GetChord(UKBSplineConfig* Config, int SegmentID, FVector& outChord)
//{
//	if (IsValid(Config))
//	{
//		if(Config->IsValidSegment(SegmentID))
//		{
//			outChord = Config->ControlPoints[SegmentID + 1].Location - Config->ControlPoints[SegmentID].Location;
//		}
//	}
//}

void USDZ_KBSpline::GetCurrentChord(FKBSplineState& State, FVector& outChord)
{
	if (State.IsValidSegment())
	{
		outChord = State.WorkingSet[FKBSplineState::ToPoint].Location - State.WorkingSet[FKBSplineState::FromPoint].Location;
	}
}

void USDZ_KBSpline::AddSegmentConstraint(UKBSplineConfig* Config, FKBSplineBounds Bound, int SegmentID)
{
	if (IsValid(Config))
	{
		Config->SegmentBounds.FindOrAdd(SegmentID) = Bound;
	}
}

FKBSplineState USDZ_KBSpline::PrepareForEvaluation(UKBSplineConfig* Config, int PointID )
{
	if (IsValid(Config))
	{
		FKBSplineState State;
		PrepareStaateForEvaluation(Config, State, PointID);
		return State;
	}

	return FKBSplineState{};
}

void USDZ_KBSpline::PrepareStaateForEvaluation(UKBSplineConfig* Config, FKBSplineState& State, int PointID)
{
	State.CurrentTraversalSegment = PointID;
	KBSplineUtils::Prepare(*Config, State);
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

void USDZ_KBSpline::DrawDebug(AActor* Actor, const UKBSplineConfig* Config, FKBSplineState State, FColor CurveColour, float Width)
{
#if !UE_BUILD_SHIPPING
	//TODO
	// Rewrite debug drawing

	//if (!CVarSDZ_SplineDebug.GetValueOnGameThread())
	//	return;

	//if (IsValid(Actor) && IsValid(Config) && Config->IsValidSegment(State.CurrentTraversalSegment))
	//{
	//	const FVector TraversalStart = Config->ControlPoints[State.CurrentTraversalSegment].Location;
	//	int CPIdx = State.CurrentTraversalSegment - 1;
	//	//FVector prevPoint = Config->ControlPoints[CPIdx].Location;
	//	//for (int pointNum = 0; pointNum < 4; ++pointNum)
	//	//{
	//	//	FVector Point = Config->ControlPoints[CPIdx + pointNum].Location;
	//	//	DrawDebugLine(Actor->GetWorld(), prevPoint, Point, FColor::White, false, 10.0f);
	//	//	prevPoint = Point;
	//	//}


	//	float step = 0.01f;
	//	FVector prev = TraversalStart;
	//	for (float Time = 0.0f; Time <= 1.0f; Time += step)
	//	{
	//		FVector sample = SampleExplicit(State, Time);
	//		DrawDebugLine(Actor->GetWorld(), prev, sample, CurveColour, false, 1.0f,0.0f, Width);
	//		prev = sample;
	//	}

	//	DrawDebugConstraints(Actor, Config, State);
	//}
#endif
}

FKBSplineState USDZ_KBSpline::Split(UKBSplineConfig* Config, const FKBSplineState State, float Alpha)
{
	FKBSplineState SplitState = State;

	if (IsValid(Config))
	{
		KBSplineUtils::Split(*Config, SplitState, Alpha);
	}

	return SplitState;
}

#if !UE_BUILD_SHIPPING
void USDZ_KBSpline::DrawDebugConstraints(AActor* Actor, const UKBSplineConfig* Config, FKBSplineState State)
{
	// TODO
	// just disabling the debug draw, this needs to be tweaked to go off of the working set and
	// the pending point queue
	
	//if (IsValid(Actor) && IsValid(Config) && Config->IsValidSegment(State.CurrentTraversalSegment))
	//{
	//	if (const auto* Bounds = Config->SegmentBounds.Find(State.CurrentTraversalSegment))
	//	{
	//		DrawDebugLine(Actor->GetWorld(), Bounds->FromBoundMin, Bounds->ToBoundMin, FColor::Red, false, 1.0f);
	//		DrawDebugLine(Actor->GetWorld(), Bounds->FromBoundMax, Bounds->ToBoundMax, FColor::Red, false, 1.0f);

	//		float step = 0.01f;
	//		FVector prev = KBSplineUtils::Sample(State.OriginalCoeffs, 0.0f);
	//		for (float Time = 0.0f; Time <= 1.0f; Time += step)
	//		{
	//			FVector sample = KBSplineUtils::Sample(State.OriginalCoeffs, Time);
	//			DrawDebugLine(Actor->GetWorld(), prev, sample, FColor::Red, false, 1.0f, 0, 1.5f);
	//			prev = sample;
	//		}

	//		// Draw the Undulation times
	//		const FVector TraversalStart = Config->ControlPoints[State.CurrentTraversalSegment].Location;
	//		const FVector TraversalEnd = Config->ControlPoints[State.CurrentTraversalSegment + 1].Location;

	//		FVector TraversalChord = TraversalEnd - TraversalStart;
	//		FVector TraversalDir = TraversalChord.GetSafeNormal();
	//		for (float U : State.UndulationTimes)
	//		{
	//			if (U <= 0.0f)
	//			{
	//				continue;
	//			}
	//			FVector ExtremePt = KBSplineUtils::Sample(State.OriginalCoeffs, U);

	//			FVector EPRelative = ExtremePt - TraversalStart;
	//			FVector FromPt = TraversalStart + (TraversalDir * EPRelative.Dot(TraversalDir));

	//			DrawDebugLine(Actor->GetWorld(), FromPt, ExtremePt, FColor::Yellow, false, 1.0f);
	//		}

	//	}
	//}
}
#endif