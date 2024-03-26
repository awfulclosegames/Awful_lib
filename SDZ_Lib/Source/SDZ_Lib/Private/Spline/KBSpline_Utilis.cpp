#include "Spline/KBSpline_Utilis.h"

// Not quite agnostic code

bool KBSplineUtils::Prepare(UKBSplineConfig& Config, FKBSplineState& State)
{
	// is the requested index valid to work with?
	if (State.CurrentTraversalSegment <= 0 || State.CurrentTraversalSegment >= (Config.ControlPoints.Num() - 2))
	{
		return false;
	}
	

	// gather the points of the active segment
	FKBSplinePoint& raw_p0 = Config.ControlPoints[State.CurrentTraversalSegment - 1];
	FKBSplinePoint& raw_p1 = Config.ControlPoints[State.CurrentTraversalSegment];
	FKBSplinePoint& raw_p2 = Config.ControlPoints[State.CurrentTraversalSegment + 1];
	FKBSplinePoint& raw_p3 = Config.ControlPoints[State.CurrentTraversalSegment + 2];

	// gather the baseline parameters too, we may adjust these based on constraints
	float tau[2] = { raw_p1.Tau, raw_p2.Tau };
	float beta[2] = { raw_p1.Beta, raw_p2.Beta };


	// if there seems like valid constraints for this section the we want to constrain
	// the spline
	if (Config.ControlPointBounds.Num() != Config.ControlPoints.Num())
	{
		// Simple case, just compute the coefficients
		//GenerateCoeffisients();
		return true;
	}

	// start by normalizing for the intended segment (cluster of four points)
	// against the current segment start p1
	FVector p[4] = { raw_p0.Location - raw_p1.Location
		, {0.0f,0.0f,0.0f}
		, raw_p2.Location - raw_p1.Location
		, raw_p3.Location - raw_p1.Location
	};

	return true;
}
