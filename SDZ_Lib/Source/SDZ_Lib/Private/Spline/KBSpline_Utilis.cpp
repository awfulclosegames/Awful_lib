#include "Spline/KBSpline_Utilis.h"

#include "GenericPlatform/GenericPlatformMath.h"
#pragma optimize ("",off)

// NOTES:
//  -Not quite agnostic code, Exposing some unneeded Unreal concepts here
//  -I don't love the parameter management through this. 

bool KBSplineUtils::Prepare(const UKBSplineConfig& Config, FKBSplineState& State)
{
	// is the requested index valid to work with?
	if (State.CurrentTraversalSegment <= 0 || State.CurrentTraversalSegment >= (Config.ControlPoints.Num() - 2))
	{
		return false;
	}
	
    ParameterBlock Block;
    
    // technically an unneeded reference, but it makes the code more readable
	// gather the points of the active segment
	const FKBSplinePoint& raw_p0 = Config.ControlPoints[State.CurrentTraversalSegment - 1];
	const FKBSplinePoint& raw_p1 = Config.ControlPoints[State.CurrentTraversalSegment];
	const FKBSplinePoint& raw_p2 = Config.ControlPoints[State.CurrentTraversalSegment + 1];
	const FKBSplinePoint& raw_p3 = Config.ControlPoints[State.CurrentTraversalSegment + 2];

    Populate(Block, raw_p0, raw_p1, raw_p2, raw_p3);

	// if there seems like valid constraints for this section the we want to constrain
	// the spline
//	if (Config.ControlPointBounds.Num() == Config.ControlPoints.Num())
	{
		// we have some constraints so try and adjust the tensioning

		// start by normalizing for the intended segment (cluster of four points)
		// against the current segment start p1
        Block.Localize();
        // the rotation of the system onto the X axis isn't really needed but being able 
        // to do all the rest of the actions exclusively on the Y axis is ver convenient
        Block.AlignToX();

        float UndulationTimes[2] = { 0.0f, 0.0f };
        bool ExceedsBounds = true;
        while (ExceedsBounds)
        {
            // update the local coefficients
            GenerateCoeffisients(Block.Points, Block, Block.Coeffs);
            // find the undulation points to identify the extremes
            ComputeUndulationTimes(UndulationTimes, Block);
            // test them against the bounds and then cinch in the tension 
            // if the curve exceedes the bounds
            ExceedsBounds = RestrictToBounds(UndulationTimes, Block);

#if !UE_BUILD_SHIPPING
            if (ExceedsBounds)
            {
                State.UndulationTimes[0] = UndulationTimes[0];
                State.UndulationTimes[1] = UndulationTimes[1];
            }
#endif
        }
    }


    State.PrecomputedCoefficients.Empty();
    State.PrecomputedCoefficients.SetNum(4);

	GenerateCoeffisients(Block.RawPoints, Block, &State.PrecomputedCoefficients[0]);
	State.Time = 0.0f;

	return true;
}


void KBSplineUtils::Populate(ParameterBlock& Block, const FKBSplinePoint& P0, const FKBSplinePoint& P1, const FKBSplinePoint& P2, const FKBSplinePoint& P3)
{
    Block.RawPoints[0] = P0.Location;
    Block.RawPoints[1] = P1.Location;
    Block.RawPoints[2] = P2.Location;
    Block.RawPoints[3] = P3.Location;

    // gather the baseline parameters too, we may adjust these based on constraints
    Block.Tau[0] = P1.Tau;
    Block.Tau[1] = P2.Tau;
    Block.Beta[0] = P1.Beta;
    Block.Beta[1] = P2.Beta;

    Block.Deltas[prev] = (Block.RawPoints[1] - Block.RawPoints[0]).Length();
    Block.Deltas[current] = (Block.RawPoints[2] - Block.RawPoints[1]).Length();
    Block.Deltas[next] = (Block.RawPoints[3] - Block.RawPoints[2]).Length();

    /* gamma is the continuity param)*/
    Block.A = 0.5 * (Block.Beta[0] + 1.0) * (-Block.Tau[0] + 1.0) /* * (1.0f + gamma_0)*/;
    Block.B = 0.5 * (-Block.Beta[0] + 1.0) * (-Block.Tau[0] + 1.0) /* * (1.0f - gamma_0)*/;
    Block.C = 0.5 * (Block.Beta[1] + 1.0) * (-Block.Tau[1] + 1.0) /* * (1.0f - gamma_1)*/;
    Block.D = 0.5 * (-Block.Beta[1] + 1.0) * (-Block.Tau[1] + 1.0) /* * (1.0f + gamma_1)*/;

}

// this is wrong because the deltas are chord length!
void KBSplineUtils::GenerateCoeffisients(FVector Points[4], ParameterBlock& Block, FVector Coeffs[4])
{
    FVector& p0 = Points[0];
    FVector& p1 = Points[1];
    FVector& p2 = Points[2];
    FVector& p3 = Points[3];
    float delta_prev = Block.Deltas[prev];
    float delta_cur = Block.Deltas[current];
    float delta_next = Block.Deltas[next];
    float a = Block.A;
    float b = Block.B;
    float c = Block.C;
    float d = Block.D;

    float u0 = pow(delta_cur, 2.0);
    float u1 = pow(delta_next, 2.0);
    float u2 = pow(delta_prev, 2.0);
    float u3 = 1 / ((u1 + delta_cur * delta_next) * u2 + (delta_cur * u1 + u0 * delta_next) * delta_prev);
    float u4 = pow(delta_cur, 3.0);
    float u5 = -c;
    float u6 = -b;
    float u7 = -d * u4;
    float u8 = -d * u0;
    FVector u9 = (d * u0 * u2 + d * u4 * delta_prev) * p3;
    float u10 = -2 * b;
    float u11 = 2 * b;

    Coeffs[0] = u3 * (u9 + (((c + b - 2) * u1 + (b - 2) * delta_cur * delta_next + u8) * u2 + ((c - 2) * delta_cur * u1 - 2 * u0 * delta_next + u7) * delta_prev) * p2 + (((u5 + u6 + 2) * u1 + (u6 + 2
            ) * delta_cur * delta_next) * u2 + ((u5 + 2) * delta_cur * u1 + 2 * u0 * delta_next) * delta_prev + a * u0 * u1 + a * u4 * delta_next) * p1 + (-a * u0 * u1 - a * u4 * delta_next) * p0);
   
    Coeffs[1] = -u3 * (u9 + (((c + u11 - 3) * u1 + (u11 - 3) * delta_cur * delta_next + u8) * u2 + ((c - 3) * delta_cur * u1 - 3 * u0 * delta_next + u7) * delta_prev) * p2 + (((u5 + u10 + 3) * u1
            + (u10 + 3) * delta_cur * delta_next) * u2 + ((u5 + 3) * delta_cur * u1 + 3 * u0 * delta_next) * delta_prev + 2 * a * u0 * u1 + 2 * a * u4 * delta_next) * p1 + (-2 * a * u0 * u1 - 2 * a * u4 * delta_next) *
            p0);
        
    Coeffs[2] = (b * u2 * p2 + (-b * u2 + a * u0) * p1 - a * u0 * p0) / (u2 + delta_cur * delta_prev);
        
    Coeffs[3] = p1;

  
}

void KBSplineUtils::ComputeUndulationTimes(float UndulationTimes[2], const ParameterBlock& Block)
{
    // to find the undulation points on the spline we just need to find the zeros of the
    // first derivative.
    // With the spline aligned to the X axis (making it a function of X in Y) this is easy
    // The first derivative of a cubic spline is a quadratic. In time it is 
    // 3t^2 * coeff0 + 2t * coeff1+ coeff2. So solve the quadratic roots.

    float A = Block.Coeffs[0].Y * 3.0f;
    float B = Block.Coeffs[1].Y * 2.0f;
    float C = Block.Coeffs[2].Y;

    BoundQuadraticRoots(A, B, C, UndulationTimes[0], UndulationTimes[1]);
}

bool KBSplineUtils::RestrictToBounds(const float UndulationTimes[2], const ParameterBlock& Block)
{
    return false;
}

void KBSplineUtils::BoundQuadraticRoots(float A, float B, float C, float& Root0, float& Root1)
{
    // good old qudratic formula
    // x = (-B +/- SQRT(B^2 - (4 * A * C))) / (2 * A)
    float A2 = A + A;
    float SqrtTerm = ((B * B) - ((A2 + A2) * C));
    // we only care about values between zero and 1
    Root0 = -1.0f;
    Root1 = -1.0f;

    if (SqrtTerm >= 0)
    {
        float SqrtVal = FMath::Sqrt(SqrtTerm);
        Root0 = (-B + SqrtVal) / A2;
        Root1 = (-B - SqrtVal) / A2;

        Root0 = Root0 <= 1.0f ? Root0 : -1.0f;
        Root1 = Root1 <= 1.0f ? Root1 : -1.0f;
    }
}

void KBSplineUtils::ParameterBlock::Localize()
{
    Points[0] = RawPoints[0] - RawPoints[1];
    Points[1] = { 0.0f,0.0f,0.0f };
    Points[2] = RawPoints[2] - RawPoints[1];
    Points[3] = RawPoints[3] - RawPoints[1];
}

void KBSplineUtils::ParameterBlock::AlignToX()
{
    // Since we normalized everything relative to traversal segment, then 
    // we can just use our traversal target for the rotation angle
    float OffAngle = FMath::Atan2(Points[2].Y, Points[2].X);

    float SinTheta;
    float CosTheta;

    FMath::SinCos(&SinTheta, &CosTheta, -OffAngle);
    for (FVector& P : Points)
    {
        P.Set ((P.X * CosTheta) - (P.Y * SinTheta),
            (P.X * SinTheta) + (P.Y * CosTheta), 
            P.Z
        );
    }
}
