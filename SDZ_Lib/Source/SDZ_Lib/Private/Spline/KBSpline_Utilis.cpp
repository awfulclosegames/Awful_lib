#include "Spline/KBSpline_Utilis.h"

#include "GenericPlatform/GenericPlatformMath.h"

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
	if (Config.ControlPointBounds.Num() == Config.ControlPoints.Num())
	{
		// we have some constraints so try and adjust the tensioning

		// start by normalizing for the intended segment (cluster of four points)
		// against the current segment start p1
        Block.Localize();

    }


	GenerateCoeffisients(Block, State.PrecomputedCoefficients);
	State.Time = 0.0f;

	return true;
}

// this is wrong because the deltas are chord length!
void KBSplineUtils::GenerateCoeffisients(ParameterBlock& Block, TArray<FVector>& Coeffs)
{
    FVector& p0 = Block.RawPoints[0];
    FVector& p1 = Block.RawPoints[1];
    FVector& p2 = Block.RawPoints[2];
    FVector& p3 = Block.RawPoints[3];
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
    
    Coeffs.Empty();
    Coeffs.SetNum(4);

    Coeffs[0] = u3 * (u9 + (((c + b - 2) * u1 + (b - 2) * delta_cur * delta_next + u8) * u2 + ((c - 2) * delta_cur * u1 - 2 * u0 * delta_next + u7) * delta_prev) * p2 + (((u5 + u6 + 2) * u1 + (u6 + 2
            ) * delta_cur * delta_next) * u2 + ((u5 + 2) * delta_cur * u1 + 2 * u0 * delta_next) * delta_prev + a * u0 * u1 + a * u4 * delta_next) * p1 + (-a * u0 * u1 - a * u4 * delta_next) * p0);
   
    Coeffs[1] = -u3 * (u9 + (((c + u11 - 3) * u1 + (u11 - 3) * delta_cur * delta_next + u8) * u2 + ((c - 3) * delta_cur * u1 - 3 * u0 * delta_next + u7) * delta_prev) * p2 + (((u5 + u10 + 3) * u1
            + (u10 + 3) * delta_cur * delta_next) * u2 + ((u5 + 3) * delta_cur * u1 + 3 * u0 * delta_next) * delta_prev + 2 * a * u0 * u1 + 2 * a * u4 * delta_next) * p1 + (-2 * a * u0 * u1 - 2 * a * u4 * delta_next) *
            p0);
        
    Coeffs[2] = (b * u2 * p2 + (-b * u2 + a * u0) * p1 - a * u0 * p0) / (u2 + delta_cur * delta_prev);
        
    Coeffs[3] = p1;

  
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

void KBSplineUtils::ParameterBlock::Localize()
{
    Points[0] = RawPoints[0] - RawPoints[1];
    Points[1] = { 0.0f,0.0f,0.0f };
    Points[2] = RawPoints[2] - RawPoints[1];
    Points[3] = RawPoints[3] - RawPoints[1];
}
