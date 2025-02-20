// Copyright Strati D. Zerbinis, 2025. All Rights Reserved.


#include "Spline/AC_KBSpline.h"
#include "Spline/KBSpline_Utilis.h"
#include "HAL/IConsoleManager.h"
#include "GameFramework/Actor.h"
#include "DrawDebugHelpers.h"

static TAutoConsoleVariable<bool> CVarAC_SplineDebug(TEXT("Awful.Spline.Debug"), false, TEXT("Enable/Disable debug visualization for the KB Spline"));

UKBSplineConfig* UAC_KBSpline::CreateSplineConfig(FVector Location)
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

int UAC_KBSpline::AddSplinePoint(UKBSplineConfig* Config, FKBSplinePoint Point)
{
	int Segment = -1;
	if (IsValid(Config))
	{
		Segment = Config->GetLastSegment();
		Config->Add(Point);
	}
	return Segment;
}

void UAC_KBSpline::RemoveLastSplinePoint(UKBSplineConfig* Config)
{
	if (!Config->ControlPoints.IsEmpty())
	{
		Config->ControlPoints.Pop();
	}
}

void UAC_KBSpline::Reset(UKBSplineConfig* Config)
{
	Config->Reset();
	Config->ControlPoints.Empty();
	Config->SegmentBounds.Empty();
}

void UAC_KBSpline::GetChord(UKBSplineConfig* Config, int SegmentID, FVector& outChord)
{
	if (IsValid(Config))
	{
		if(Config->IsValidSegment(SegmentID))
		{
			Config->GetTravelChord(SegmentID, outChord);
		}
	}
}

void UAC_KBSpline::AddSegmentConstraint(UKBSplineConfig* Config, FKBSplineBounds Bound, int SegmentID)
{
	if (IsValid(Config))
	{
		Config->SegmentBounds.FindOrAdd(SegmentID) = Bound;
	}
}

FKBSplineState UAC_KBSpline::PrepareForEvaluation(UKBSplineConfig* Config, int PointID )
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

FVector UAC_KBSpline::ComputeTangent(FKBSplineState State)
{
	// compute the first derivative of the curve to get the tangent
	float TimeSquared = State.Time * State.Time;
	FVector SamplePoint = State.PrecomputedCoefficients[0] * TimeSquared +
		State.PrecomputedCoefficients[1] * State.Time +
		State.PrecomputedCoefficients[2];
	return SamplePoint;
}

// these sample operations are easily vectorizable and probably should be
FVector UAC_KBSpline::Sample(FKBSplineState State)
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

FVector UAC_KBSpline::SampleExplicit(FKBSplineState State, float Completion)
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

void UAC_KBSpline::DrawDebug(AActor* Actor, const UKBSplineConfig* Config, FKBSplineState State, FColor CurveColour, float Width, float DisplayTime)
{
#if !UE_BUILD_SHIPPING
	if (!CVarAC_SplineDebug.GetValueOnGameThread())
		return;
	

	if (IsValid(Actor) && IsValid(Config))
	{
		int CurrentIndex = 0;
		FVector prevPoint = Config->ControlPoints.First().Location;
		for (auto& point : Config->ControlPoints)
		{
			FColor Color = CurrentIndex <= Config->CommitPoint ? FColor::Yellow : FColor::White;
			float ChordDisplayTime = CurrentIndex <= Config->CommitPoint ? DisplayTime : 0.5f;
			DrawDebugLine(Actor->GetWorld(), prevPoint, point.Location, Color, false, ChordDisplayTime);
			prevPoint = point.Location;
			++CurrentIndex;
		}

		if (Config->IsValidSegment(State.CurrentTraversalSegment))
		{
			int normalizedSegment = Config->NormalizeSegmentID(State.CurrentTraversalSegment);
			const FVector TraversalStart = Config->ControlPoints[normalizedSegment].Location;

			int CPIdx = normalizedSegment - 1;

			float step = 0.01f;
			FVector prev = TraversalStart;
			for (float Time = 0.0f; Time <= 1.0f; Time += step)
			{
				FVector sample = SampleExplicit(State, Time);
				DrawDebugLine(Actor->GetWorld(), prev, sample, CurveColour, false, DisplayTime, 0.0f, Width);
				prev = sample;
			}

			DrawDebugConstraints(Actor, Config, State);
		}
	}
#endif
}


#if !UE_BUILD_SHIPPING
void UAC_KBSpline::DrawDebugConstraints(AActor* Actor, const UKBSplineConfig* Config, FKBSplineState State)
{
	if (IsValid(Actor) && IsValid(Config) && Config->IsValidSegment(State.CurrentTraversalSegment))
	{
		if (const auto* Bounds = Config->SegmentBounds.Find(State.CurrentTraversalSegment))
		{
			DrawDebugLine(Actor->GetWorld(), Bounds->Anchors[FKBSplineBounds::FromPoint].MaxBound, Bounds->Anchors[FKBSplineBounds::ToPoint].MaxBound, FColor::Orange, false, 1.0f, 0, 1.5f);
			DrawDebugLine(Actor->GetWorld(), Bounds->Anchors[FKBSplineBounds::FromPoint].MinBound, Bounds->Anchors[FKBSplineBounds::ToPoint].MinBound, FColor::Orange, false, 1.0f, 0, 1.5f);

			float step = 0.01f;
			FVector prev = KBSplineUtils::Sample(State.OriginalCoeffs, 0.0f);
			for (float Time = 0.0f; Time <= 1.0f; Time += step)
			{
				FVector sample = KBSplineUtils::Sample(State.OriginalCoeffs, Time);
				DrawDebugLine(Actor->GetWorld(), prev, sample, FColor::Red, false, 1.0f, 0, 1.5f);
				prev = sample;
			}

			// Draw the Undulation times
			const FVector TraversalStart = Config->ControlPoints[State.CurrentTraversalSegment].Location;
			const FVector TraversalEnd = Config->ControlPoints[State.CurrentTraversalSegment + 1].Location;

			FVector TraversalChord = TraversalEnd - TraversalStart;
			FVector TraversalDir = TraversalChord.GetSafeNormal();
			for (float U : State.UndulationTimes)
			{
				if (U <= 0.0f)
				{
					continue;
				}
				FVector ExtremePt = KBSplineUtils::Sample(State.OriginalCoeffs, U);

				FVector EPRelative = ExtremePt - TraversalStart;
				FVector FromPt = TraversalStart + (TraversalDir * EPRelative.Dot(TraversalDir));

				DrawDebugLine(Actor->GetWorld(), FromPt, ExtremePt, FColor::Yellow, false, 1.0f);
			}

		}
	}
}
#endif