#include "Spline/KBSpline_Utilis.h"

#include "GenericPlatform/GenericPlatformMath.h"

// Not quite agnostic code

bool KBSplineUtils::Prepare(const UKBSplineConfig& Config, FKBSplineState& State)
{
	// is the requested index valid to work with?
	if (State.CurrentTraversalSegment <= 0 || State.CurrentTraversalSegment >= (Config.ControlPoints.Num() - 2))
	{
		return false;
	}
	

	// gather the points of the active segment
	const FKBSplinePoint& raw_p0 = Config.ControlPoints[State.CurrentTraversalSegment - 1];
	const FKBSplinePoint& raw_p1 = Config.ControlPoints[State.CurrentTraversalSegment];
	const FKBSplinePoint& raw_p2 = Config.ControlPoints[State.CurrentTraversalSegment + 1];
	const FKBSplinePoint& raw_p3 = Config.ControlPoints[State.CurrentTraversalSegment + 2];

	// gather the baseline parameters too, we may adjust these based on constraints
	float tau[2] = { raw_p1.Tau, raw_p2.Tau };
	float beta[2] = { raw_p1.Beta, raw_p2.Beta };


	/* gamma is the continuity param)*/
	float A = 0.5 * (beta[0] + 1.0) * (-tau[0] + 1.0) /* * (1.0f + gamma_0)*/;
	float B = 0.5 * (-beta[0] + 1.0) * (-tau[0] + 1.0) /* * (1.0f - gamma_0)*/;
	float C = 0.5 * (beta[1] + 1.0) * (-tau[1] + 1.0) /* * (1.0f - gamma_1)*/;
	float D = 0.5 * (-beta[1] + 1.0) * (-tau[1] + 1.0)/* * (1.0f + gamma_1)*/;

	// if there seems like valid constraints for this section the we want to constrain
	// the spline
	if (Config.ControlPointBounds.Num() == Config.ControlPoints.Num())
	{
		// we have some constraints so try and adjust the tensioning

		// start by normalizing for the intended segment (cluster of four points)
		// against the current segment start p1
		FVector p[4] = { raw_p0.Location - raw_p1.Location
			, {0.0f,0.0f,0.0f}
			, raw_p2.Location - raw_p1.Location
			, raw_p3.Location - raw_p1.Location
		};
	}


	GenerateCoeffisients(&raw_p0, A, B, C, D, State.PrecomputedCoefficients);

	State.Time = 0.0f;

	return true;
}

// this is wrong because the deltas are chord length!
void KBSplineUtils::GenerateCoeffisients(const FKBSplinePoint* Points, float a, float b, float c, float d, TArray<FVector>& Coeffs)
{
    //FVector p0 = Points[0].Location;
    //FVector p1 = Points[1].Location;
    //FVector p2 = Points[2].Location;
    //FVector p3 = Points[3].Location;

    //FVector u0 = FMath::Pow(p0, 2.0);
    //FVector u1 = FMath::Pow(p1, 2.0);
    //FVector u2 = FMath::Pow(p2, 2.0);
    //FVector u3 = FMath::Pow(p2, 3.0);
    //FVector u4 = u2 + (-p1 - p0) * p2 + p0 * p1;
    //FVector u5 = 1 / (u4 * p3 - u3 + 2 * p1 * u2 + (-u1 - p0 * p1 + u0) * p2 + p0 * u1 - u0 * p1);
    //FVector u6 = FMath::Pow(p0, 4.0);
    //float u7 = -4 * a;
    //FVector u8 = FMath::Pow(p0, 3.0);
    //float u9 = 6 * a;
    //float u10 = -2 * d;
    //FVector u11 = FMath::Pow(p1, 3.0);
    //float u12 = -b;
    //float u13 = u12 + a;
    //FVector u14 = FMath::Pow(p1, 4.0);
    //float u15 = -d;
    //float u16 = -3 * a;
    //float u17 = -3 * b;
    //float u18 = 4 * b;
    //float u19 = 3 * b;
    //float u20 = -2 * c;
    //float u21 = -6 * b;
    //float u22 = -c;
    //float u23 = 2 * c;
    //FVector u24 = FMath::Pow(p2, 4.0);
    //FVector u25 = -a * u8;
    //FVector u26 = 3 * a * u0 * p1;
    //FVector u27 = u13 * u11;
    //float u28 = -8 * a;
    //float u29 = 2 * b;
    //float u30 = 2 * a;
    //float u31 = -2 * b;
    //float u32 = u31 + u30;
    //float u33 = -6 * a;
    //float u34 = 8 * b;
    //float u35 = 6 * b;
    //{
    //    Coeffs[0] = u5 * (((c + b - 2) * u3 + ((u20 + u17 + 4) * p1 + (u22 + 2) * p0) * u2 + ((c + u19 - 2) * u1 + (u23 - 4) * p0 * p1) * p2 + u27 + (u22 + u16 + 2) * p0 * u1 + u26 + u25) * p3 + (u22 + u12 + 2) * u24 +
    //        ((u23 + u18 - 6) * p1 + (c + u12) * p0) * u3 + ((d + u22 + u21 + 6) * u1 + (u10 + u20 + u19 + 2) * p0 * p1 + (d - 2) * u0) * u2 + ((u15 + u18 - a - 2) * u11 + (d + c + u17 + 3 * a - 4) * p0 * u1 + (d + u16 + 4) * u0 * p1 + (u15 + a
    //            ) * u8) * p2 + u13 * u14 + (d + b + u7 + 2) * p0 * u11 + (u10 + u9 - 2) * u0 * u1 + (d + u7) * u8 * p1 + a * u6);
    //    Coeffs[1] = -u5 * (((c + u29 - 3) * u3 + ((u20 + u21 + 6) * p1 + (u22 + 3) * p0) * u2 + ((c + u35 - 3) * u1 + (u23 - 6) * p0 * p1) * p2 + u32 * u11 + (u22 + u33 + 3) * p0 * u1 + 6 * a * u0 * p1 - 2 * a * u8) * p3 +
    //        (u22 + u31 + 3) * u24 + ((u23 + u34 - 9) * p1 + (c + u31) * p0) * u3 + ((d + u22 - 12 * b + 9) * u1 + (u10 + u20 + u35 + 3) * p0 * p1 + (d - 3) * u0) * u2 + ((u15 + u34 - 2 * a - 3) * u11 + (d + c + u21 + u9 - 6) * p0 * u1 + (d +
    //            u33 + 6) * u0 * p1 + (u15 + u30) * u8) * p2 + u32 * u14 + (d + u29 + u28 + 3) * p0 * u11 + (u10 + 12 * a - 3) * u0 * u1 + (d + u28) * u8 * p1 + 2 * a * u6);
    //    Coeffs[2] = (b * u3 - 3 * b * p1 * u2 + 3 * b * u1 * p2 + u27 - 3 * a * p0 * u1 + u26 + u25) / u4;
    //    Coeffs[3] = p1;
    //}
}
