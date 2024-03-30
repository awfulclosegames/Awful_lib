#include "Spline/KBSpline_Utilis.h"

#include "GenericPlatform/GenericPlatformMath.h"

// NOTES:
//  -Not quite agnostic code, Exposing some unneeded Unreal concepts here
//  -I don't love the parameter management through this. 

bool KBSplineUtils::Prepare(const UKBSplineConfig& Config, FKBSplineState& State )
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
    if (auto Bound = Config.SegmentBounds.Find(State.CurrentTraversalSegment))
	{
		// we have some constraints so try and adjust the tensioning
#if !UE_BUILD_SHIPPING
        // record the original curve state for debugging
        GenerateCoeffisients(Block.RawPoints, Block, State.OriginalCoeffs);
#endif
		// start by normalizing for the intended segment (cluster of four points)
		// against the current segment start p1
        Block.Localize(*Bound);
        // the rotation of the system onto the X axis isn't really needed but being able 
        // to do all the rest of the actions exclusively on the Y axis is ver convenient
        Block.AlignToX();

        int Attempts = 4;
        float UndulationTimes[2] = { 0.0f, 0.0f };
        bool ExceedsBounds = true;
        while (ExceedsBounds && Attempts > 0)
        {
            --Attempts;
            // update the local coefficients
            GenerateCoeffisients(Block.Points, Block, Block.Coeffs);
            // find the undulation points to identify the extremes
            ComputeUndulationTimes(UndulationTimes, Block);
            // test them against the bounds and then cinch in the tension 
            // if the curve exceedes the bounds
            ExceedsBounds = RestrictToBounds(UndulationTimes, Block);

#if !UE_BUILD_SHIPPING
            // for debugging I should record all iterations but for now
            // just the first ones 
            if (Attempts == 3)
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

    ComputeAB(Block);
    ComputeCD(Block);
}

// this is wrong because the deltas are chord length!
void KBSplineUtils::GenerateCoeffisients(FVector Points[4], ParameterBlock& Block, FVector Coeffs[4])
{
    const FVector& p0 = Points[0];
    const FVector& p1 = Points[1];
    const FVector& p2 = Points[2];
    const FVector& p3 = Points[3];
    const float delta_prev = Block.Deltas[prev];
    const float delta_cur = Block.Deltas[current];
    const float delta_next = Block.Deltas[next];
    const float a = Block.A;
    const float b = Block.B;
    const float c = Block.C;
    const float d = Block.D;

    const float u0 = pow(delta_cur, 2.0);
    const float u1 = pow(delta_next, 2.0);
    const float u2 = pow(delta_prev, 2.0);
    const float u3 = 1 / ((u1 + delta_cur * delta_next) * u2 + (delta_cur * u1 + u0 * delta_next) * delta_prev);
    const float u4 = pow(delta_cur, 3.0);
    const float u5 = -c;
    const float u6 = -b;
    const float u7 = -d * u4;
    const float u8 = -d * u0;
    const FVector u9 = (d * u0 * u2 + d * u4 * delta_prev) * p3;
    const float u10 = -2 * b;
    const float u11 = 2 * b;

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

bool KBSplineUtils::RestrictToBounds(const float UndulationTimes[2], ParameterBlock& Block)
{
    //FVector ExtremePoint = Sample(Block.Points, UndulationTimes[0]);

    // if any of the extreme points of the spline exceed the bounds then 
    // we perform the tensioning and return true. Since the tensioning 
    // changes the spline we want to re-run the computation of extreme points
    // and bounding test
    auto EnforceBoundry = [&Block](float Time)
        {
            FVector ExtremePoint = Sample(Block.Coeffs, Time);
            FVector RestrictedPoint;

            if (ExceedsBound(Block.Bounds, ExtremePoint, RestrictedPoint))
            {
                if (Time < 0.5f)
                {
                    TightenStart(RestrictedPoint, Time, Block);
                }
                else
                {
                    TightenEnd(RestrictedPoint, Time, Block);
                }
                return true;
            }
            return false;
        };

    if (EnforceBoundry(UndulationTimes[0]))
    {
        return true;
    }

    return EnforceBoundry(UndulationTimes[1]);
}

bool KBSplineUtils::ExceedsBound(const FVector Bounds[4], FVector TestPoint, FVector& RestrictedPoint)
{
    // For now, assumptions:
    // in normalized form the boundry Mins are negative Y and Maxes are positive Y

    int MinOrMax = TestPoint.Y > 0.0f;

    // our boundry edge
    const FVector& Start = Bounds[StartMin + MinOrMax];
    const FVector Edge = Bounds[EndMin + MinOrMax] - Start;

    // TODO the unit boundry edges can all be precomputed and cached!
    const FVector UnitEdge = Edge.GetSafeNormal();

    float EdgeProjection = (TestPoint - Start).Dot(UnitEdge);

    RestrictedPoint = Start + (UnitEdge * EdgeProjection);
    float difference = TestPoint.Y - RestrictedPoint.Y;

    // we want to know if the boundry at the point of the sample is nearer than the sample
    // But we could be compairing two positive or two negative numbers
    //
    // so if the sign of the result of the subtraction is the same as the test point 
    // then the restricted point was nearer and so we exceeded the boundry.
    // Multiply the two together, if they are both the same sign then the product is positive
    return (difference * TestPoint.Y) > 0.0f;
}

void KBSplineUtils::TightenStart(const FVector& RestrictedPoint, float t, ParameterBlock& Block)
{
    const float p0 = Block.Points[0].Y;
    const float p1 = Block.Points[1].Y;
    const float p2 = Block.Points[2].Y;
    const float p3 = Block.Points[3].Y;
    const float p4 = RestrictedPoint.Y;
    const float delta_prev = Block.Deltas[prev];
    const float delta_cur = Block.Deltas[current];
    const float delta_next = Block.Deltas[next];
    const float c = Block.C;
    const float d = Block.D;
    const float beta_0 = Block.Beta[0];

    const float u0 = beta_0 + 1;
    const float u1 = pow(delta_cur, 3.0);
    const float u2 = u0 * u1 * delta_next;
    const float u3 = pow(delta_cur, 2.0);
    const float u4 = pow(delta_next, 2.0);
    const float u5 = u0 * u3 * u4;
    const float u6 = -beta_0;
    const float u7 = u6 - 1;
    const float u8 = u7 * u1 * delta_next;
    const float u9 = u7 * u3 * u4;
    const float u10 = u6 + 1;
    const float u11 = u10 * u4 + u10 * delta_cur * delta_next;
    const float u12 = pow(delta_prev, 2.0);
    const float u13 = beta_0 - 1;
    const float u14 = u13 * u4 + u13 * delta_cur * delta_next;
    const float u15 = u14 * u12 * p2 + (u11 * u12 + u9 + u8) * p1 + (u5 + u2) * p0;
    const float u16 = -2 * beta_0;
    const float u17 = u16 - 2;
    const float u18 = u17 * u1 * delta_next;
    const float u19 = u17 * u3 * u4;
    const float u20 = 2 * beta_0;
    const float u21 = u20 + 2;
    const float u22 = u21 * u1 * delta_next;
    const float u23 = u21 * u3 * u4;
    const float u24 = u20 - 2;
    const float u25 = u16 + 2;
    const float u26 = pow(t, 2.0);
    const float u27 = pow(t, 3.0);
    const float u28 = (u9 + u8) * p0;
    const float u29 = 2 * c;
    const float u30 = -2 * c;

    Block.Tau[0] = -(((2 * d * u3 * u12 + 2 * d * u1 * delta_prev) * p3 + (((u29 + u6 - 3) * u4 + (u6 - 3) * delta_cur * delta_next - 2 * d * u3) * u12 + ((u29 - 4) * delta_cur * u4 - 4 * u3 * delta_next - 2 * d * u1) *
        delta_prev) * p2 + (((u30 + beta_0 + 3) * u4 + (beta_0 + 3) * delta_cur * delta_next) * u12 + ((u30 + 4) * delta_cur * u4 + 4 * u3 * delta_next) * delta_prev + u5 + u2) * p1 + u28) * u27 + ((-2 * d * u3 *
            u12 - 2 * d * u1 * delta_prev) * p3 + (((u30 + u20 + 4) * u4 + (u20 + 4) * delta_cur * delta_next + 2 * d * u3) * u12 + ((u30 + 6) * delta_cur * u4 + 6 * u3 * delta_next + 2 * d * u1) * delta_prev) * p2 + (((u29
                + u16 - 4) * u4 + (u16 - 4) * delta_cur * delta_next) * u12 + ((u29 - 6) * delta_cur * u4 - 6 * u3 * delta_next) * delta_prev + u19 + u18) * p1 + (u23 + u22) * p0) * u26 + (u11 * u12 * p2 + (u14 * u12 + u5 + u2
                    ) * p1 + u28) * t + ((-2 * u4 - 2 * delta_cur * delta_next) * u12 + (-2 * delta_cur * u4 - 2 * u3 * delta_next) * delta_prev) * p4 + ((2 * u4 + 2 * delta_cur * delta_next) * u12 + (2 * delta_cur * u4 + 2 *
                        u3 * delta_next) * delta_prev) * p1) / (u15 * u27 + ((u25 * u4 + u25 * delta_cur * delta_next) * u12 * p2 + ((u24 * u4 + u24 * delta_cur * delta_next) * u12 + u23 + u22) * p1 + (u19 + u18) * p0) * u26 +
                            u15 * t);

    Block.Tau[0] = FMath::Clamp(Block.Tau[0], -1.0f, 1.0f);
    ComputeAB(Block);
}

void KBSplineUtils::TightenEnd(const FVector& RestrictedPoint, float t, ParameterBlock& Block)
{
    const float p0 = Block.Points[0].Y;
    const float p1 = Block.Points[1].Y;
    const float p2 = Block.Points[2].Y;
    const float p3 = Block.Points[3].Y;
    const float p4 = RestrictedPoint.Y;
    const float delta_prev = Block.Deltas[prev];
    const float delta_cur = Block.Deltas[current];
    const float delta_next = Block.Deltas[next];
    const float a = Block.A;
    const float b = Block.B;
    const float beta_1 = Block.Beta[1];

    const float u0 = -beta_1;
    const float u1 = u0 - 1;
    const float u2 = pow(delta_next, 2.0);
    const float u3 = pow(delta_prev, 2.0);
    const float u4 = beta_1 - 1;
    const float u5 = pow(delta_cur, 3.0);
    const float u6 = u4 * u5;
    const float u7 = beta_1 + 1;
    const float u8 = pow(delta_cur, 2.0);
    const float u9 = u4 * u8;
    const float u10 = u0 + 1;
    const float u11 = (u10 * u8 * u3 + u10 * u5 * delta_prev) * p3;
    const float u12 = pow(t, 2.0);
    const float u13 = u10 * u5;
    const float u14 = u10 * u8;
    const float u15 = (u4 * u8 * u3 + u4 * u5 * delta_prev) * p3;
    const float u16 = pow(t, 3.0);
    const float u17 = (2 * a * u8 * u2 + 2 * a * u5 * delta_next) * p0;
    const float u18 = -2 * a * u5 * delta_next;
    const float u19 = -2 * a * u8 * u2;
    const float u20 = -4 * b;
    const float u21 = 4 * b;
    const float u22 = 2 * b;
    const float u23 = -2 * b;
    Block.Tau[1] = ((u15 + (((u0 + u23 + 3) * u2 + (u23 + 4) * delta_cur * delta_next + u14) * u3 + ((u0 + 3) * delta_cur * u2 + 4 * u8 * delta_next + u13) * delta_prev) * p2 + (((beta_1 + u22 - 3) * u2 + (u22 - 4) *
        delta_cur * delta_next) * u3 + ((beta_1 - 3) * delta_cur * u2 - 4 * u8 * delta_next) * delta_prev + u19 + u18) * p1 + u17) * u16 + (u11 + (((beta_1 + u21 - 5) * u2 + (u21 - 6) * delta_cur *
            delta_next + u9) * u3 + ((beta_1 - 5) * delta_cur * u2 - 6 * u8 * delta_next + u6) * delta_prev) * p2 + (((u0 + u20 + 5) * u2 + (u20 + 6) * delta_cur * delta_next) * u3 + ((u0 + 5) * delta_cur * u2 + 6 *
                u8 * delta_next) * delta_prev + 4 * a * u8 * u2 + 4 * a * u5 * delta_next) * p1 + (-4 * a * u8 * u2 - 4 * a * u5 * delta_next) * p0) * u12 + ((-2 * b * u2 - 2 * b * delta_cur * delta_next) * u3 * p2 + ((2 * b * u2 + 2 * b
                    * delta_cur * delta_next) * u3 + u19 + u18) * p1 + u17) * t + ((2 * u2 + 2 * delta_cur * delta_next) * u3 + (2 * delta_cur * u2 + 2 * u8 * delta_next) * delta_prev) * p4 + ((-2 * u2 - 2 * delta_cur *
                        delta_next) * u3 + (-2 * delta_cur * u2 - 2 * u8 * delta_next) * delta_prev) * p1) / ((u15 + ((u1 * u2 + u14) * u3 + (u1 * delta_cur * u2 + u13) * delta_prev) * p2 + (u7 * u2 * u3 + u7 * delta_cur * u2 *
                            delta_prev) * p1) * u16 + (u11 + ((u7 * u2 + u9) * u3 + (u7 * delta_cur * u2 + u6) * delta_prev) * p2 + (u1 * u2 * u3 + u1 * delta_cur * u2 * delta_prev) * p1) * u12);

    Block.Tau[1] = FMath::Clamp(Block.Tau[1], -1.0f, 1.0f);
    ComputeCD(Block);
}

void KBSplineUtils::ComputeAB(ParameterBlock& Block)
{
    /* gamma is the continuity param)*/
    Block.A = 0.5 * (Block.Beta[0] + 1.0) * (-Block.Tau[0] + 1.0) /* * (1.0f + gamma_0)*/;
    Block.B = 0.5 * (-Block.Beta[0] + 1.0) * (-Block.Tau[0] + 1.0) /* * (1.0f - gamma_0)*/;
}

void KBSplineUtils::ComputeCD(ParameterBlock& Block)
{
    /* gamma is the continuity param)*/
    Block.C = 0.5 * (Block.Beta[1] + 1.0) * (-Block.Tau[1] + 1.0) /* * (1.0f - gamma_1)*/;
    Block.D = 0.5 * (-Block.Beta[1] + 1.0) * (-Block.Tau[1] + 1.0) /* * (1.0f + gamma_1)*/;
}

FVector KBSplineUtils::Sample(const FVector Coeffs[4], float Time)
{
    float tSq = Time * Time;
    float tQb = tSq * Time;
    return (Coeffs[0] * tQb) + (Coeffs[1] * tSq) + (Coeffs[2] * Time) + (Coeffs[3]);
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

// A lot of this could beprecomputed if we don't expect it to be changing frequently
void KBSplineUtils::ParameterBlock::Localize(const FKBSplineBounds& RawBound)
{
    Points[0] = RawPoints[0] - RawPoints[1];
    Points[1] = { 0.0f,0.0f,0.0f };
    Points[2] = RawPoints[2] - RawPoints[1];
    Points[3] = RawPoints[3] - RawPoints[1];

    Bounds[StartMin] = RawBound.FromBoundMin - RawPoints[1];
    Bounds[StartMax] = RawBound.FromBoundMax - RawPoints[1];
    Bounds[EndMin] = RawBound.ToBoundMin - RawPoints[1];
    Bounds[EndMax] = RawBound.ToBoundMax - RawPoints[1];
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
    for (FVector& P : Bounds)
    {
        P.Set((P.X * CosTheta) - (P.Y * SinTheta),
            (P.X * SinTheta) + (P.Y * CosTheta),
            P.Z
        );
    }
}
