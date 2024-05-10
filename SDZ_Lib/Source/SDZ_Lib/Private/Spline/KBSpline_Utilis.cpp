#include "Spline/KBSpline_Utilis.h"

#include "GenericPlatform/GenericPlatformMath.h"

// NOTES:
//  -Not quite agnostic code, Exposing some unneeded Unreal concepts here
//  -I don't love the parameter management through this. 

bool KBSplineUtils::Prepare(const UKBSplineConfig& Config, FKBSplineState& State, bool bIgnoreBoundes)
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
    if (!bIgnoreBoundes)
    {
        EnsureBoundingConstraint(Config, State, Block);
    }

    State.PrecomputedCoefficients.Empty();
    State.PrecomputedCoefficients.SetNum(4);
    State.Tau[0] = Block.Tau[0];
    State.Tau[1] = Block.Tau[1];
    State.Beta[0] = Block.Beta[0];
    State.Beta[1] = Block.Beta[1];
    GenerateCoeffisients(Block.RawPoints, Block, State.PrecomputedCoefficients.GetData());
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

void KBSplineUtils::EnsureBoundingConstraint(const UKBSplineConfig& Config, FKBSplineState& State, ParameterBlock& Block)
{
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
            State.UndulationTimes[0] = UndulationTimes[0];
            State.UndulationTimes[1] = UndulationTimes[1];

#if !UE_BUILD_SHIPPING
            // for debugging I should record all iterations but for now
            // just the first ones 
            if (Attempts == 3)
            {
                State.OriginalUndulationTimes[0] = UndulationTimes[0];
                State.OriginalUndulationTimes[1] = UndulationTimes[1];
            }
#endif
        }
    }
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

void KBSplineUtils::MatchSlopeAtStart(ParameterBlock& Block)
{
    /*
    float e, f;
    delta_prev = (p1 - p0).Length();
    delta_cur = (p2 - p1).Length();
    delta_next = (p3 - p2).Length();
    delta_new_cur = (p4 - p1).Length();
    delta__new_next = (p2 - p4).Length();
    e = 0.5 * (beta_2 + 1.0) * (-tau_2 + 1.0);
    f = 0.5 * (-beta_2 + 1.0) * (-tau_2 + 1.0);
    {
        u0 = beta_2 + 1;
        u1 = pow(delta_cur, 2.0);
        u2 = pow(delta_new_cur, 3.0);
        u3 = pow(delta_new_cur, 2.0);
        u4 = pow(delta_new_next, 2.0);
        u5 = u0 * delta_cur * u3 * u4 + u0 * delta_cur * u2 * delta_new_next;
        u6 = pow(delta_next, 2.0);
        u7 = -beta_2;
        u8 = u7 - 1;
        u9 = u8 * delta_cur * u3 * u4 + u8 * delta_cur * u2 * delta_new_next;
        u10 = u7 + 1;
        u11 = u10 * delta_cur * u4 + u10 * delta_cur * delta_new_cur * delta_new_next;
        u12 = pow(delta_prev, 2.0);
        u13 = pow(delta_prev, 3.0);
        u14 = beta_2 - 1;
        u15 = u14 * delta_cur * u4 + u14 * delta_cur * delta_new_cur * delta_new_next;
        u16 = (((u14 * u4 + u14 * delta_new_cur * delta_new_next) * u6 + u15 * delta_next) * u13 + (u15 * u6 + (u14 * u1 * u4 + u14 * u1 * delta_new_cur * delta_new_next) * delta_next) * u12) * p4;
        u17 = -4 * beta_2;
        u18 = u17 - 4;
        u19 = u18 * delta_cur * u3 * u4 + u18 * delta_cur * u2 * delta_new_next;
        u20 = (((u18 * u3 * u4 + u18 * u2 * delta_new_next) * u6 + u19 * delta_next) * delta_prev + u19 * u6 + (u18 * u1 * u3 * u4 + u18 * u1 * u2 * delta_new_next) * delta_next) * p0;
        u21 = 4 * beta_2;
        u22 = u21 + 4;
        u23 = (u22 * u1 * u3 * u4 + u22 * u1 * u2 * delta_new_next) * delta_next;
        u24 = u22 * delta_cur * u3 * u4 + u22 * delta_cur * u2 * delta_new_next;
        u25 = u24 * u6;
        u26 = u21 - 4;
        u27 = u26 * delta_cur * u4 + u26 * delta_cur * delta_new_cur * delta_new_next;
        u28 = u17 + 4;
        u29 = u28 * delta_cur * u4 + u28 * delta_cur * delta_new_cur * delta_new_next;
        u30 = 3 * beta_2;
        u31 = u30 + 3;
        u32 = u31 * delta_cur * u3 * u4 + u31 * delta_cur * u2 * delta_new_next;
        u33 = (((u31 * u3 * u4 + u31 * u2 * delta_new_next) * u6 + u32 * delta_next) * delta_prev + u32 * u6 + (u31 * u1 * u3 * u4 + u31 * u1 * u2 * delta_new_next) * delta_next) * p0;
        u34 = -3 * beta_2;
        u35 = u34 - 3;
        u36 = (u35 * u1 * u3 * u4 + u35 * u1 * u2 * delta_new_next) * delta_next;
        u37 = u35 * delta_cur * u3 * u4 + u35 * delta_cur * u2 * delta_new_next;
        u38 = u37 * u6;
        u39 = u34 + 3;
        u40 = u39 * delta_cur * u4 + u39 * delta_cur * delta_new_cur * delta_new_next;
        u41 = u30 - 3;
        u42 = u41 * delta_cur * u4 + u41 * delta_cur * delta_new_cur * delta_new_next;
        u43 = pow(r, 2.0);
        u44 = pow(delta_cur, 3.0);
        u45 = -2 * a * u44 * delta_new_cur;
        u46 = u0 * delta_cur * u2;
        u47 = -2 * a * u1 * delta_new_cur;
        u48 = u0 * delta_cur * u3;
        u49 = 2 * a * u44 * delta_new_cur;
        u50 = u8 * delta_cur * u2;
        u51 = 2 * a * u1 * delta_new_cur;
        u52 = u8 * delta_cur * u3;
        u53 = u7 - 2 * b + 1;
        u54 = 12 * u1 * u3;
        u55 = -4 * c;
        u56 = u55 + 12;
        u57 = 12 * delta_cur * u3;
        u58 = u56 * delta_cur * delta_new_cur;
        u59 = u21 + 8;
        u60 = u55 + u21 + 8;
        u61 = 12 * u3;
        u62 = 4 * d * u1 * u3;
        u63 = -12 * u1 * u3 * delta_new_next;
        u64 = 4 * c;
        u65 = u64 - 12;
        u66 = -4 * d * delta_cur * u2;
        u67 = -4 * d * u1 * u3;
        u68 = u17 - 8;
        u69 = -12 * delta_cur * u3;
        u70 = u64 + u17 - 8;
        u71 = u65 * delta_cur * delta_new_cur;
        u72 = -4 * d * delta_cur * u3;
        u73 = -12 * u3;
        u74 = -12 * u1 * u3;
        u75 = 6 * c;
        u76 = u75 - 12;
        u77 = u76 * delta_cur * delta_new_cur;
        u78 = u34 - 9;
        u79 = u75 + u34 - 9;
        u80 = -6 * d * u1 * u3;
        u81 = 12 * u1 * u3 * delta_new_next;
        u82 = -6 * c;
        u83 = u82 + 12;
        u84 = 6 * d * delta_cur * u2;
        u85 = 6 * d * u1 * u3;
        u86 = u30 + 9;
        u87 = u82 + u30 + 9;
        u88 = u83 * delta_cur * delta_new_cur;
        u89 = 6 * d * delta_cur * u3;
        u90 = -12 * u1 * delta_new_cur;
        u91 = 8 * b;
        u92 = u91 - 12;
        u93 = -12 * u1;
        u94 = u64 + u91 - 12;
        u95 = 12 * u1 * delta_new_cur;
        u96 = -8 * b;
        u97 = u96 + 12;
        u98 = 12 * u1;
        u99 = u55 + u96 + 12;
        u100 = -6 * b;
        u101 = u100 + 12;
        u102 = u82 + u100 + 12;
        u103 = 6 * b;
        u104 = u103 - 12;
        u105 = u75 + u103 - 12;
        tau_2 = ((((6 * d * u1 * u4 + 6 * d * u1 * delta_new_cur * delta_new_next) * u13 + ((6 * d * u1 * delta_new_cur + 6 * d * u44) * u4 + (u85 + 6 * d * u44 * delta_new_cur) * delta_new_next) * u12 + (6 * d * u44
            * delta_new_cur * u4 + 6 * d * u44 * u3 * delta_new_next) * delta_prev) * p3 + (((u105 * u4 + u105 * delta_new_cur * delta_new_next) * u6 + (u104 * delta_cur * u4 + u104 * delta_cur *
                delta_new_cur * delta_new_next) * delta_next - 6 * d * u1 * u4 - 6 * d * u1 * delta_new_cur * delta_new_next) * u13 + (((u105 * delta_new_cur + u76 * delta_cur) * u4 + (u105 * u3 + u77) *
                    delta_new_next) * u6 + ((u104 * delta_cur * delta_new_cur + u93) * u4 + (u104 * delta_cur * u3 + u90) * delta_new_next) * delta_next + (-6 * d * u1 * delta_new_cur - 6 * d * u44) * u4 + (u80 - 6 *
                        d * u44 * delta_new_cur) * delta_new_next) * u12 + ((u76 * delta_cur * delta_new_cur * u4 + u76 * delta_cur * u3 * delta_new_next) * u6 + (-12 * u1 * delta_new_cur * u4 + u63) * delta_next
                            - 6 * d * u44 * delta_new_cur * u4 - 6 * d * u44 * u3 * delta_new_next) * delta_prev) * p2 + (((u102 * u4 + u102 * delta_new_cur * delta_new_next) * u6 + (u101 * delta_cur * u4 + u101 * delta_cur *
                                delta_new_cur * delta_new_next) * delta_next) * u13 + (((u102 * delta_new_cur + u83 * delta_cur) * u4 + (u102 * u3 + u88) * delta_new_next) * u6 + ((u101 * delta_cur * delta_new_cur +
                                    u98) * u4 + (u101 * delta_cur * u3 + u95) * delta_new_next) * delta_next) * u12 + (((u88 + 6 * a * u1) * u4 + (u83 * delta_cur * u3 + 6 * a * u1 * delta_new_cur) * delta_new_next) * u6 + ((u95 + 6 * a *
                                        u44) * u4 + (u54 + 6 * a * u44 * delta_new_cur) * delta_new_next) * delta_next) * delta_prev + (6 * a * u1 * delta_new_cur * u4 + 6 * a * u1 * u3 * delta_new_next) * u6 + (6 * a * u44 * delta_new_cur
                                            * u4 + 6 * a * u44 * u3 * delta_new_next) * delta_next) * p1 + (((-6 * a * u1 * u4 - 6 * a * u1 * delta_new_cur * delta_new_next) * u6 + (-6 * a * u44 * u4 - 6 * a * u44 * delta_new_cur * delta_new_next) *
                                                delta_next) * delta_prev + (-6 * a * u1 * delta_new_cur * u4 - 6 * a * u1 * u3 * delta_new_next) * u6 + (-6 * a * u44 * delta_new_cur * u4 - 6 * a * u44 * u3 * delta_new_next) * delta_next) * p0) * pow
                                                (t, 2.0) + (((-4 * d * u1 * u4 - 4 * d * u1 * delta_new_cur * delta_new_next) * u13 + ((-4 * d * u1 * delta_new_cur - 4 * d * u44) * u4 + (u67 - 4 * d * u44 * delta_new_cur) * delta_new_next) * u12 + (-4 *
                                                    d * u44 * delta_new_cur * u4 - 4 * d * u44 * u3 * delta_new_next) * delta_prev) * p3 + (((u99 * u4 + u99 * delta_new_cur * delta_new_next) * u6 + (u97 * delta_cur * u4 + u97 * delta_cur *
                                                        delta_new_cur * delta_new_next) * delta_next + 4 * d * u1 * u4 + 4 * d * u1 * delta_new_cur * delta_new_next) * u13 + (((u99 * delta_new_cur + u56 * delta_cur) * u4 + (u99 * u3 + u58) *
                                                            delta_new_next) * u6 + ((u97 * delta_cur * delta_new_cur + u98) * u4 + (u97 * delta_cur * u3 + u95) * delta_new_next) * delta_next + (4 * d * u1 * delta_new_cur + 4 * d * u44) * u4 + (u62 + 4 * d *
                                                                u44 * delta_new_cur) * delta_new_next) * u12 + ((u56 * delta_cur * delta_new_cur * u4 + u56 * delta_cur * u3 * delta_new_next) * u6 + (12 * u1 * delta_new_cur * u4 + u81) * delta_next + 4 * d
                                                                    * u44 * delta_new_cur * u4 + 4 * d * u44 * u3 * delta_new_next) * delta_prev) * p2 + (((u94 * u4 + u94 * delta_new_cur * delta_new_next) * u6 + (u92 * delta_cur * u4 + u92 * delta_cur *
                                                                        delta_new_cur * delta_new_next) * delta_next) * u13 + (((u94 * delta_new_cur + u65 * delta_cur) * u4 + (u94 * u3 + u71) * delta_new_next) * u6 + ((u92 * delta_cur * delta_new_cur + u93)
                                                                            * u4 + (u92 * delta_cur * u3 + u90) * delta_new_next) * delta_next) * u12 + (((u71 - 8 * a * u1) * u4 + (u65 * delta_cur * u3 - 8 * a * u1 * delta_new_cur) * delta_new_next) * u6 + ((u90 - 8 * a * u44) *
                                                                                u4 + (u74 - 8 * a * u44 * delta_new_cur) * delta_new_next) * delta_next) * delta_prev + (-8 * a * u1 * delta_new_cur * u4 - 8 * a * u1 * u3 * delta_new_next) * u6 + (-8 * a * u44 * delta_new_cur * u4
                                                                                    - 8 * a * u44 * u3 * delta_new_next) * delta_next) * p1 + (((8 * a * u1 * u4 + 8 * a * u1 * delta_new_cur * delta_new_next) * u6 + (8 * a * u44 * u4 + 8 * a * u44 * delta_new_cur * delta_new_next) *
                                                                                        delta_next) * delta_prev + (8 * a * u1 * delta_new_cur * u4 + 8 * a * u1 * u3 * delta_new_next) * u6 + (8 * a * u44 * delta_new_cur * u4 + 8 * a * u44 * u3 * delta_new_next) * delta_next) * p0) * t + (((
                                                                                            (u87 * u4 + u86 * delta_new_cur * delta_new_next + 6 * d * u3) * u6 + (u87 * delta_cur * u4 + u86 * delta_cur * delta_new_cur * delta_new_next + u89) * delta_next) * u13 + (((u83 *
                                                                                                delta_new_cur + u87 * delta_cur) * u4 + (u61 + u86 * delta_cur * delta_new_cur) * delta_new_next + 6 * d * u2 + u89) * u6 + ((u88 + u87 * u1) * u4 + (u57 + u86 * u1 * delta_new_cur) *
                                                                                                    delta_new_next + u84 + u85) * delta_next) * u12 + ((u83 * delta_cur * delta_new_cur * u4 + 12 * delta_cur * u3 * delta_new_next + u84) * u6 + (u83 * u1 * delta_new_cur * u4 + u81 + 6 * d * u1 * u2)
                                                                                                        * delta_next) * delta_prev) * p4 + ((-6 * d * u3 * u6 - 6 * d * delta_cur * u3 * delta_next) * u13 + ((-6 * d * u2 - 6 * d * delta_cur * u3) * u6 + (-6 * d * delta_cur * u2 + u80) * delta_next) * u12 + (-6 * d *
                                                                                                            delta_cur * u2 * u6 - 6 * d * u1 * u2 * delta_next) * delta_prev) * p2 + (((u79 * u4 + u78 * delta_new_cur * delta_new_next) * u6 + (u79 * delta_cur * u4 + u78 * delta_cur * delta_new_cur *
                                                                                                                delta_new_next) * delta_next) * u13 + (((u76 * delta_new_cur + u79 * delta_cur) * u4 + (u73 + u78 * delta_cur * delta_new_cur) * delta_new_next) * u6 + ((u77 + u79 * u1) * u4 + (u69 + u78 *
                                                                                                                    u1 * delta_new_cur) * delta_new_next) * delta_next) * u12 + (((u35 * u3 + u77) * u4 + (u35 * u2 + u69) * delta_new_next) * u6 + ((u35 * delta_cur * u3 + u76 * u1 * delta_new_cur) * u4 + (u35 *
                                                                                                                        delta_cur * u2 + u74) * delta_new_next) * delta_next) * delta_prev + u38 + u36) * p1 + u33) * u43 + ((((u70 * u4 + u68 * delta_new_cur * delta_new_next - 4 * d * u3) * u6 + (u70 * delta_cur * u4 +
                                                                                                                            u68 * delta_cur * delta_new_cur * delta_new_next + u72) * delta_next) * u13 + (((u65 * delta_new_cur + u70 * delta_cur) * u4 + (u73 + u68 * delta_cur * delta_new_cur) * delta_new_next
                                                                                                                                - 4 * d * u2 + u72) * u6 + ((u71 + u70 * u1) * u4 + (u69 + u68 * u1 * delta_new_cur) * delta_new_next + u66 + u67) * delta_next) * u12 + ((u65 * delta_cur * delta_new_cur * u4 - 12 * delta_cur * u3 *
                                                                                                                                    delta_new_next + u66) * u6 + (u65 * u1 * delta_new_cur * u4 + u63 - 4 * d * u1 * u2) * delta_next) * delta_prev) * p4 + ((4 * d * u3 * u6 + 4 * d * delta_cur * u3 * delta_next) * u13 + ((4 * d * u2 + 4 * d *
                                                                                                                                        delta_cur * u3) * u6 + (4 * d * delta_cur * u2 + u62) * delta_next) * u12 + (4 * d * delta_cur * u2 * u6 + 4 * d * u1 * u2 * delta_next) * delta_prev) * p2 + (((u60 * u4 + u59 * delta_new_cur *
                                                                                                                                            delta_new_next) * u6 + (u60 * delta_cur * u4 + u59 * delta_cur * delta_new_cur * delta_new_next) * delta_next) * u13 + (((u56 * delta_new_cur + u60 * delta_cur) * u4 + (u61 + u59 *
                                                                                                                                                delta_cur * delta_new_cur) * delta_new_next) * u6 + ((u58 + u60 * u1) * u4 + (u57 + u59 * u1 * delta_new_cur) * delta_new_next) * delta_next) * u12 + (((u22 * u3 + u58) * u4 + (u22 * u2 + u57) *
                                                                                                                                                    delta_new_next) * u6 + ((u22 * delta_cur * u3 + u56 * u1 * delta_new_cur) * u4 + (u22 * delta_cur * u2 + u54) * delta_new_next) * delta_next) * delta_prev + u25 + u23) * p1 + u20) * r + u16 + (((
                                                                                                                                                        2 * b * u4 + 2 * b * delta_new_cur * delta_new_next) * u6 + (2 * b * delta_cur * u4 + 2 * b * delta_cur * delta_new_cur * delta_new_next) * delta_next) * u13 + ((2 * b * delta_new_cur * u4 + 2 * b * u3
                                                                                                                                                            * delta_new_next) * u6 + (2 * b * delta_cur * delta_new_cur * u4 + 2 * b * delta_cur * u3 * delta_new_next) * delta_next) * u12) * p2 + (((u53 * u4 + u53 * delta_new_cur * delta_new_next) * u6
                                                                                                                                                                + (u53 * delta_cur * u4 + u53 * delta_cur * delta_new_cur * delta_new_next) * delta_next) * u13 + (((-2 * b * delta_new_cur + u10 * delta_cur) * u4 + (-2 * b * u3 + u10 * delta_cur *
                                                                                                                                                                    delta_new_cur) * delta_new_next) * u6 + ((-2 * b * delta_cur * delta_new_cur + u10 * u1) * u4 + (-2 * b * delta_cur * u3 + u10 * u1 * delta_new_cur) * delta_new_next) * delta_next) * u12 + ((
                                                                                                                                                                        (u8 * u3 + 2 * a * u1) * u4 + (u8 * u2 + u51) * delta_new_next) * u6 + ((u52 + 2 * a * u44) * u4 + (u50 + u49) * delta_new_next) * delta_next) * delta_prev + ((u52 + u51) * u4 + (u50 + 2 * a * u1 * u3) *
                                                                                                                                                                            delta_new_next) * u6 + ((u8 * u1 * u3 + u49) * u4 + (u8 * u1 * u2 + 2 * a * u44 * u3) * delta_new_next) * delta_next) * p1 + ((((u0 * u3 - 2 * a * u1) * u4 + (u0 * u2 + u47) * delta_new_next) * u6 + ((u48 - 2 *
                                                                                                                                                                                a * u44) * u4 + (u46 + u45) * delta_new_next) * delta_next) * delta_prev + ((u48 + u47) * u4 + (u46 - 2 * a * u1 * u3) * delta_new_next) * u6 + ((u0 * u1 * u3 + u45) * u4 + (u0 * u1 * u2 - 2 * a * u44 * u3) *
                                                                                                                                                                                    delta_new_next) * delta_next) * p0) / (((((u41 * u4 + u41 * delta_new_cur * delta_new_next) * u6 + u42 * delta_next) * u13 + (u42 * u6 + (u41 * u1 * u4 + u41 * u1 * delta_new_cur *
                                                                                                                                                                                        delta_new_next) * delta_next) * u12) * p4 + (((u39 * u4 + u39 * delta_new_cur * delta_new_next) * u6 + u40 * delta_next) * u13 + (u40 * u6 + (u39 * u1 * u4 + u39 * u1 * delta_new_cur *
                                                                                                                                                                                            delta_new_next) * delta_next) * u12 + ((u35 * u3 * u4 + u35 * u2 * delta_new_next) * u6 + u37 * delta_next) * delta_prev + u38 + u36) * p1 + u33) * u43 + ((((u28 * u4 + u28 * delta_new_cur *
                                                                                                                                                                                                delta_new_next) * u6 + u29 * delta_next) * u13 + (u29 * u6 + (u28 * u1 * u4 + u28 * u1 * delta_new_cur * delta_new_next) * delta_next) * u12) * p4 + (((u26 * u4 + u26 * delta_new_cur *
                                                                                                                                                                                                    delta_new_next) * u6 + u27 * delta_next) * u13 + (u27 * u6 + (u26 * u1 * u4 + u26 * u1 * delta_new_cur * delta_new_next) * delta_next) * u12 + ((u22 * u3 * u4 + u22 * u2 * delta_new_next) * u6 +
                                                                                                                                                                                                        u24 * delta_next) * delta_prev + u25 + u23) * p1 + u20) * r + u16 + (((u10 * u4 + u10 * delta_new_cur * delta_new_next) * u6 + u11 * delta_next) * u13 + (u11 * u6 + (u10 * u1 * u4 + u10 * u1 *
                                                                                                                                                                                                            delta_new_cur * delta_new_next) * delta_next) * u12 + ((u8 * u3 * u4 + u8 * u2 * delta_new_next) * u6 + u9 * delta_next) * delta_prev + u9 * u6 + (u8 * u1 * u3 * u4 + u8 * u1 * u2 * delta_new_next)
                                                                                                                                                                                                            * delta_next) * p1 + (((u0 * u3 * u4 + u0 * u2 * delta_new_next) * u6 + u5 * delta_next) * delta_prev + u5 * u6 + (u0 * u1 * u3 * u4 + u0 * u1 * u2 * delta_new_next) * delta_next) * p0);
    }
    */
}

void KBSplineUtils::MatchSlopeAtEnd()
{
 /*
     float g,h;
    delta_prev=(p1-p0).Length();
    delta_cur=(p2-p1).Length();
    delta_next=(p3-p2).Length();
    delta_new_cur=(p4-p1).Length();
    delta__new_next=(p2-p4).Length();
    g=0.5*(beta_3+1.0)*(-tau_3+1.0);
    h=0.5*(-beta_3+1.0)*(-tau_3+1.0);
    {
        u0=2*beta_3;
        u1=u0+2;
        u2=pow(delta_cur,2.0);
        u3=pow(delta_new_next,2.0);
        u4=pow(delta_next,2.0);
        u5=pow(delta_prev,2.0);
        u6=pow(delta_prev,3.0);
        u7=u0-2;
        u8=pow(delta_new_cur,3.0);
        u9=pow(delta_new_cur,2.0);
        u10=u7*u2*u9;
        u11=u7*delta_cur*u8;
        u12=u7*delta_cur*u9;
        u13=u7*u8;
        u14=-2*beta_3;
        u15=u14+2;
        u16=u14-2;
        u17=u15*delta_cur*u8;
        u18=u15*u2*u9;
        u19=u15*delta_cur*u9;
        u20=u15*u8;
        u21=-3*beta_3;
        u22=u21-3;
        u23=u21+3;
        u24=u23*u2*u9;
        u25=u23*delta_cur*u8;
        u26=u23*delta_cur*u9;
        u27=u23*u8;
        u28=3*beta_3;
        u29=u28-3;
        u30=u28+3;
        u31=u29*delta_cur*u8;
        u32=u29*u2*u9;
        u33=u29*delta_cur*u9;
        u34=u29*u8;
        u35=pow(r,2.0);
        u36=pow(delta_cur,3.0);
        u37=-2*a*u36*delta_new_cur;
        u38=2*a*u2*u9;
        u39=-2*a*u2*u9;
        u40=2*a*delta_cur*u8;
        u41=-2*a*u2*delta_new_cur;
        u42=2*a*delta_cur*u9;
        u43=2*a*u36*delta_new_cur;
        u44=-2*a*delta_cur*u8;
        u45=2*a*u2*delta_new_cur;
        u46=-2*a*delta_cur*u9;
        u47=-2*b*delta_cur*u3-2*b*delta_cur*delta_new_cur*delta_new_next;
        u48=-8*a*delta_cur*u9*u3-8*a*delta_cur*u8*delta_new_next;
        u49=12*u2*u9;
        u50=u14+10;
        u51=12*delta_cur*u9;
        u52=u50*delta_cur*delta_new_cur;
        u53=-8*b;
        u54=u53+12;
        u55=u14+u53+10;
        u56=u54*delta_cur*delta_new_cur;
        u57=12*u9;
        u58=u54*delta_cur*delta_new_cur*delta_new_next;
        u59=-12*u2*u9*delta_new_next;
        u60=u0-10;
        u61=8*b;
        u62=u61-12;
        u63=-12*delta_cur*u9;
        u64=u0+u61-10;
        u65=u62*delta_cur*delta_new_cur;
        u66=-12*u9;
        u67=u62*delta_cur*delta_new_cur*delta_new_next;
        u68=6*a*delta_cur*u9*u3+6*a*delta_cur*u8*delta_new_next;
        u69=-12*u2*u9;
        u70=u28-9;
        u71=u70*delta_cur*delta_new_cur;
        u72=6*b;
        u73=u72-12;
        u74=u28+u72-9;
        u75=u73*delta_cur*delta_new_cur;
        u76=u73*delta_cur*delta_new_cur*delta_new_next;
        u77=12*u2*u9*delta_new_next;
        u78=u21+9;
        u79=-6*b;
        u80=u79+12;
        u81=u21+u79+9;
        u82=u80*delta_cur*delta_new_cur;
        u83=u80*delta_cur*delta_new_cur*delta_new_next;
        u84=-12*u2*delta_new_cur;
        u85=4*c;
        u86=u85-12;
        u87=u86*delta_cur*delta_new_cur;
        u88=-12*u2;
        u89=u85+u61-12;
        u90=-4*c;
        u91=u90+12;
        u92=12*u2*delta_new_cur;
        u93=12*u2;
        u94=u90+u53+12;
        u95=-6*c;
        u96=u95+12;
        u97=u96*delta_cur*delta_new_cur;
        u98=u95+u79+12;
        u99=6*c;
        u100=u99-12;
        u101=u99+u72-12;
        tau_3=-((((6*d*u2*u3+6*d*u2*delta_new_cur*delta_new_next)*u6+((6*d*u2*delta_new_cur+6*d*u36)*u3+(6*d*u2*u9+6*d*u36*delta_new_cur)*delta_new_next)*u5+(6*
         d*u36*delta_new_cur*u3+6*d*u36*u9*delta_new_next)*delta_prev)*p3+(((u101*u3+u101*delta_new_cur*delta_new_next)*u4+(u73*delta_cur*u3+u76)*delta_next-6*d
         *u2*u3-6*d*u2*delta_new_cur*delta_new_next)*u6+(((u101*delta_new_cur+u100*delta_cur)*u3+(u101*u9+u100*delta_cur*delta_new_cur)*delta_new_next)*u4+((u75
         +u88)*u3+(u73*delta_cur*u9+u84)*delta_new_next)*delta_next+(-6*d*u2*delta_new_cur-6*d*u36)*u3+(-6*d*u2*u9-6*d*u36*delta_new_cur)*delta_new_next)*u5+((
         u100*delta_cur*delta_new_cur*u3+u100*delta_cur*u9*delta_new_next)*u4+(-12*u2*delta_new_cur*u3+u59)*delta_next-6*d*u36*delta_new_cur*u3-6*d*u36*u9*
         delta_new_next)*delta_prev)*p2+(((u98*u3+u98*delta_new_cur*delta_new_next)*u4+(u80*delta_cur*u3+u83)*delta_next)*u6+(((u98*delta_new_cur+u96*delta_cur)
         *u3+(u98*u9+u97)*delta_new_next)*u4+((u82+u93)*u3+(u80*delta_cur*u9+u92)*delta_new_next)*delta_next)*u5+(((u97+6*a*u2)*u3+(u96*delta_cur*u9+6*a*u2*
         delta_new_cur)*delta_new_next)*u4+((u92+6*a*u36)*u3+(u49+6*a*u36*delta_new_cur)*delta_new_next)*delta_next)*delta_prev+(6*a*u2*delta_new_cur*u3+6*a*u2*
         u9*delta_new_next)*u4+(6*a*u36*delta_new_cur*u3+6*a*u36*u9*delta_new_next)*delta_next)*p1+(((-6*a*u2*u3-6*a*u2*delta_new_cur*delta_new_next)*u4+(-6*a*
         u36*u3-6*a*u36*delta_new_cur*delta_new_next)*delta_next)*delta_prev+(-6*a*u2*delta_new_cur*u3-6*a*u2*u9*delta_new_next)*u4+(-6*a*u36*delta_new_cur*u3-6
         *a*u36*u9*delta_new_next)*delta_next)*p0)*pow(t,2.0)+(((-4*d*u2*u3-4*d*u2*delta_new_cur*delta_new_next)*u6+((-4*d*u2*delta_new_cur-4*d*u36)*u3+(-4*d*u2
         *u9-4*d*u36*delta_new_cur)*delta_new_next)*u5+(-4*d*u36*delta_new_cur*u3-4*d*u36*u9*delta_new_next)*delta_prev)*p3+(((u94*u3+u94*delta_new_cur*
         delta_new_next)*u4+(u54*delta_cur*u3+u58)*delta_next+4*d*u2*u3+4*d*u2*delta_new_cur*delta_new_next)*u6+(((u94*delta_new_cur+u91*delta_cur)*u3+(u94*u9+
         u91*delta_cur*delta_new_cur)*delta_new_next)*u4+((u56+u93)*u3+(u54*delta_cur*u9+u92)*delta_new_next)*delta_next+(4*d*u2*delta_new_cur+4*d*u36)*u3+(4*d*
         u2*u9+4*d*u36*delta_new_cur)*delta_new_next)*u5+((u91*delta_cur*delta_new_cur*u3+u91*delta_cur*u9*delta_new_next)*u4+(12*u2*delta_new_cur*u3+u77)*
         delta_next+4*d*u36*delta_new_cur*u3+4*d*u36*u9*delta_new_next)*delta_prev)*p2+(((u89*u3+u89*delta_new_cur*delta_new_next)*u4+(u62*delta_cur*u3+u67)*
         delta_next)*u6+(((u89*delta_new_cur+u86*delta_cur)*u3+(u89*u9+u87)*delta_new_next)*u4+((u65+u88)*u3+(u62*delta_cur*u9+u84)*delta_new_next)*delta_next)*
         u5+(((u87-8*a*u2)*u3+(u86*delta_cur*u9-8*a*u2*delta_new_cur)*delta_new_next)*u4+((u84-8*a*u36)*u3+(u69-8*a*u36*delta_new_cur)*delta_new_next)*
         delta_next)*delta_prev+(-8*a*u2*delta_new_cur*u3-8*a*u2*u9*delta_new_next)*u4+(-8*a*u36*delta_new_cur*u3-8*a*u36*u9*delta_new_next)*delta_next)*p1+(((8
         *a*u2*u3+8*a*u2*delta_new_cur*delta_new_next)*u4+(8*a*u36*u3+8*a*u36*delta_new_cur*delta_new_next)*delta_next)*delta_prev+(8*a*u2*delta_new_cur*u3+8*a*
         u2*u9*delta_new_next)*u4+(8*a*u36*delta_new_cur*u3+8*a*u36*u9*delta_new_next)*delta_next)*p0)*t+((((u81*u3+u80*delta_new_cur*delta_new_next+u23*u9)*u4+
         (u81*delta_cur*u3+u83+u26)*delta_next)*u6+(((u78*delta_new_cur+u81*delta_cur)*u3+(u57+u82)*delta_new_next+u27+u26)*u4+((u78*delta_cur*delta_new_cur+u81
         *u2)*u3+(u51+u80*u2*delta_new_cur)*delta_new_next+u25+u24)*delta_next)*u5+((u78*delta_cur*delta_new_cur*u3+12*delta_cur*u9*delta_new_next+u25)*u4+(u78*
         u2*delta_new_cur*u3+u77+u23*u2*u8)*delta_next)*delta_prev)*p4+((u29*u9*u4+u29*delta_cur*u9*delta_next)*u6+((u34+u33)*u4+(u31+u32)*delta_next)*u5+(u29*
         delta_cur*u8*u4+u29*u2*u8*delta_next)*delta_prev)*p2+(((u74*u3+u73*delta_new_cur*delta_new_next)*u4+(u74*delta_cur*u3+u76)*delta_next)*u6+(((u70*
         delta_new_cur+u74*delta_cur)*u3+(u66+u75)*delta_new_next)*u4+((u71+u74*u2)*u3+(u63+u73*u2*delta_new_cur)*delta_new_next)*delta_next)*u5+(((-6*a*u9+u71)
         *u3+(-6*a*u8+u63)*delta_new_next)*u4+((-6*a*delta_cur*u9+u70*u2*delta_new_cur)*u3+(-6*a*delta_cur*u8+u69)*delta_new_next)*delta_next)*delta_prev+(-6*a*
         delta_cur*u9*u3-6*a*delta_cur*u8*delta_new_next)*u4+(-6*a*u2*u9*u3-6*a*u2*u8*delta_new_next)*delta_next)*p1+(((6*a*u9*u3+6*a*u8*delta_new_next)*u4+u68*
         delta_next)*delta_prev+u68*u4+(6*a*u2*u9*u3+6*a*u2*u8*delta_new_next)*delta_next)*p0)*u35+((((u64*u3+u62*delta_new_cur*delta_new_next+u7*u9)*u4+(u64*
         delta_cur*u3+u67+u12)*delta_next)*u6+(((u60*delta_new_cur+u64*delta_cur)*u3+(u66+u65)*delta_new_next+u13+u12)*u4+((u60*delta_cur*delta_new_cur+u64*u2)*
         u3+(u63+u62*u2*delta_new_cur)*delta_new_next+u11+u10)*delta_next)*u5+((u60*delta_cur*delta_new_cur*u3-12*delta_cur*u9*delta_new_next+u11)*u4+(u60*u2*
         delta_new_cur*u3+u59+u7*u2*u8)*delta_next)*delta_prev)*p4+((u15*u9*u4+u15*delta_cur*u9*delta_next)*u6+((u20+u19)*u4+(u17+u18)*delta_next)*u5+(u15*
         delta_cur*u8*u4+u15*u2*u8*delta_next)*delta_prev)*p2+(((u55*u3+u54*delta_new_cur*delta_new_next)*u4+(u55*delta_cur*u3+u58)*delta_next)*u6+(((u50*
         delta_new_cur+u55*delta_cur)*u3+(u57+u56)*delta_new_next)*u4+((u52+u55*u2)*u3+(u51+u54*u2*delta_new_cur)*delta_new_next)*delta_next)*u5+(((8*a*u9+u52)*
         u3+(8*a*u8+u51)*delta_new_next)*u4+((8*a*delta_cur*u9+u50*u2*delta_new_cur)*u3+(8*a*delta_cur*u8+u49)*delta_new_next)*delta_next)*delta_prev+(8*a*
         delta_cur*u9*u3+8*a*delta_cur*u8*delta_new_next)*u4+(8*a*u2*u9*u3+8*a*u2*u8*delta_new_next)*delta_next)*p1+(((-8*a*u9*u3-8*a*u8*delta_new_next)*u4+u48*
         delta_next)*delta_prev+u48*u4+(-8*a*u2*u9*u3-8*a*u2*u8*delta_new_next)*delta_next)*p0)*r+(((-2*b*u3-2*b*delta_new_cur*delta_new_next)*u4+u47*delta_next
         )*u6+(u47*u4+(-2*b*u2*u3-2*b*u2*delta_new_cur*delta_new_next)*delta_next)*u5)*p4+(((2*b*u3+2*b*delta_new_cur*delta_new_next)*u4+(2*b*delta_cur*u3+2*b*
         delta_cur*delta_new_cur*delta_new_next)*delta_next)*u6+((2*b*delta_new_cur*u3+2*b*u9*delta_new_next)*u4+(2*b*delta_cur*delta_new_cur*u3+2*b*delta_cur*
         u9*delta_new_next)*delta_next)*u5)*p2+((((-2*b*delta_new_cur+2*b*delta_cur)*u3+(-2*b*u9+2*b*delta_cur*delta_new_cur)*delta_new_next)*u4+((-2*b*
         delta_cur*delta_new_cur+2*b*u2)*u3+(-2*b*delta_cur*u9+2*b*u2*delta_new_cur)*delta_new_next)*delta_next)*u5+(((-2*a*u9+2*a*u2)*u3+(-2*a*u8+u45)*
         delta_new_next)*u4+((u46+2*a*u36)*u3+(u44+u43)*delta_new_next)*delta_next)*delta_prev+((u46+u45)*u3+(u44+u38)*delta_new_next)*u4+((u39+u43)*u3+(-2*a*u2
         *u8+2*a*u36*u9)*delta_new_next)*delta_next)*p1+((((2*a*u9-2*a*u2)*u3+(2*a*u8+u41)*delta_new_next)*u4+((u42-2*a*u36)*u3+(u40+u37)*delta_new_next)*
         delta_next)*delta_prev+((u42+u41)*u3+(u40+u39)*delta_new_next)*u4+((u38+u37)*u3+(2*a*u2*u8-2*a*u36*u9)*delta_new_next)*delta_next)*p0)/(((((u30*u3+u29*
         u9)*u4+(u30*delta_cur*u3+u33)*delta_next)*u6+(((u30*delta_new_cur+u30*delta_cur)*u3+u34+u33)*u4+((u30*delta_cur*delta_new_cur+u30*u2)*u3+u31+u32)*
         delta_next)*u5+((u30*delta_cur*delta_new_cur*u3+u31)*u4+(u30*u2*delta_new_cur*u3+u29*u2*u8)*delta_next)*delta_prev)*p4+((u23*u9*u4+u23*delta_cur*u9*
         delta_next)*u6+((u27+u26)*u4+(u25+u24)*delta_next)*u5+(u23*delta_cur*u8*u4+u23*u2*u8*delta_next)*delta_prev)*p2+((u22*u3*u4+u22*delta_cur*u3*delta_next
         )*u6+((u22*delta_new_cur+u22*delta_cur)*u3*u4+(u22*delta_cur*delta_new_cur+u22*u2)*u3*delta_next)*u5+(u22*delta_cur*delta_new_cur*u3*u4+u22*u2*
         delta_new_cur*u3*delta_next)*delta_prev)*p1)*u35+((((u16*u3+u15*u9)*u4+(u16*delta_cur*u3+u19)*delta_next)*u6+(((u16*delta_new_cur+u16*delta_cur)*u3+u20
         +u19)*u4+((u16*delta_cur*delta_new_cur+u16*u2)*u3+u17+u18)*delta_next)*u5+((u16*delta_cur*delta_new_cur*u3+u17)*u4+(u16*u2*delta_new_cur*u3+u15*u2*u8)*
         delta_next)*delta_prev)*p4+((u7*u9*u4+u7*delta_cur*u9*delta_next)*u6+((u13+u12)*u4+(u11+u10)*delta_next)*u5+(u7*delta_cur*u8*u4+u7*u2*u8*delta_next)*
         delta_prev)*p2+((u1*u3*u4+u1*delta_cur*u3*delta_next)*u6+((u1*delta_new_cur+u1*delta_cur)*u3*u4+(u1*delta_cur*delta_new_cur+u1*u2)*u3*delta_next)*u5+(
         u1*delta_cur*delta_new_cur*u3*u4+u1*u2*delta_new_cur*u3*delta_next)*delta_prev)*p1)*r);
    }

 */
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

void KBSplineUtils::Split(UKBSplineConfig& Config, FKBSplineState& State, float Alpha)
{
    // lets try and keep this light
        float A = FMath::Clamp(Alpha, 0.0f, 1.0f);
    int CurrentStartId = State.CurrentTraversalSegment;
    int CurrentDestId = CurrentStartId + 1;
    
    FKBSplinePoint NewDestPoint;
    NewDestPoint.Location = Sample(State.PrecomputedCoefficients.GetData(), A);
    NewDestPoint.Tau = (State.Tau[0] * (1.0f - A)) + (State.Tau[1] * A);
    NewDestPoint.Beta = (State.Beta[0] * (1.0f - A)) + (State.Beta[1] * A);

    // Insert the new point between the current and destination points
    Config.ControlPoints.Insert(NewDestPoint, CurrentDestId);

    if (State.Time > 0.0f)
    {
    // we want to split off a new segment entirely, so add a new start point
        FKBSplinePoint NewStartPoint;
        NewStartPoint.Location = Sample(State.PrecomputedCoefficients.GetData(), State.Time);
        NewStartPoint.Tau = (Config.ControlPoints[CurrentStartId].Tau * (1.0f - State.Time)) + (Config.ControlPoints[CurrentDestId].Tau * State.Time);
        NewStartPoint.Beta = (Config.ControlPoints[CurrentStartId].Beta * (1.0f - State.Time)) + (Config.ControlPoints[CurrentDestId].Beta * State.Time);
        Config.ControlPoints.Insert(NewStartPoint, CurrentDestId);
        // since we just inserted a new point between the old start and dest our new start
        // is our old dest ID
        CurrentStartId = CurrentDestId;
        ++CurrentDestId;
    }

    // split the curve up, this could be done better but this should maintain the 
    // shape and also be cheap
    // except for the buffer insertion, but I can fix that later

    // if the undulation points are in our cliping, then add them as separate points
    for (int i = 0; i < 2; ++i)
    {
        float currentUndulation = State.UndulationTimes[i];
        if (currentUndulation > State.Time && currentUndulation < A)
        {
            FKBSplinePoint NewUndulationPoint;
            // We haven't recomputed the coefficients yet so this will be the correct point for the 
            // undulation on the old spline
            NewUndulationPoint.Location = Sample(State.PrecomputedCoefficients.GetData(), currentUndulation);
            NewUndulationPoint.Tau = (Config.ControlPoints[CurrentStartId].Tau * (1.0f - currentUndulation)) + (Config.ControlPoints[CurrentDestId].Tau * currentUndulation);
            NewUndulationPoint.Beta = (Config.ControlPoints[CurrentStartId].Beta * (1.0f - currentUndulation)) + (Config.ControlPoints[CurrentDestId].Beta * currentUndulation);
            Config.ControlPoints.Insert(NewUndulationPoint, CurrentDestId);

            State.UndulationTimes[i] -= State.Time;
            continue;
        }
        State.UndulationTimes[i] = -1.0f;
    }

    State.Time = 0.0f;
    State.CurrentTraversalSegment = CurrentStartId;

    // now recompute the coefficients and update the state tau/beta
    Prepare(Config, State, true);
}

//void KBSplineUtils::Split(UKBSplineConfig& Config, FKBSplineState& State, float Alpha)
//{
//    float A = FMath::Clamp(Alpha, 0.0f, 1.0f);
//    int CurrentStartId = State.CurrentTraversalSegment;
//    int CurrentDestId = CurrentStartId + 1;
//    
//    FKBSplinePoint NewDestPoint;
//    NewDestPoint.Location = Sample(State.PrecomputedCoefficients.GetData(), A);
//    //NewDestPoint.Tau = (State.Tau[0] * (1.0f - A)) + (State.Tau[1] * A);
//    //NewDestPoint.Beta = (State.Beta[0] * (1.0f - A)) + (State.Beta[1] * A);
//    NewDestPoint.Tau = (Config.ControlPoints[CurrentStartId].Tau * (1.0f - A)) + (Config.ControlPoints[CurrentDestId].Tau * A);
//    NewDestPoint.Beta = (Config.ControlPoints[CurrentStartId].Beta * (1.0f - A)) + (Config.ControlPoints[CurrentDestId].Beta * A);
//
//    // Insert the new point between the current and destination points
//    Config.ControlPoints.Insert(NewDestPoint, CurrentDestId);
//
//    if (State.Time > 0.0f)
//    {
//    // we want to split off a new segment entirely, so add a new start point
//        FKBSplinePoint NewStartPoint;
//        NewStartPoint.Location = Sample(State.PrecomputedCoefficients.GetData(), State.Time);
//        NewStartPoint.Tau = (Config.ControlPoints[CurrentStartId].Tau * (1.0f - State.Time)) + (Config.ControlPoints[CurrentDestId].Tau * State.Time);
//        NewStartPoint.Beta = (Config.ControlPoints[CurrentStartId].Beta * (1.0f - State.Time)) + (Config.ControlPoints[CurrentDestId].Beta * State.Time);
//        Config.ControlPoints.Insert(NewStartPoint, CurrentDestId);
//        // since we just inserted a new point between the old start and dest our new start
//        // is our old dest ID
//        CurrentStartId = CurrentDestId;
//    }
//    State.CurrentTraversalSegment = CurrentStartId;
//    Prepare(Config, State);
//
//    float NewTime = State.Time / (Alpha > 0.0f ? Alpha : 1000000.0f);
//    State.Time = FMath::Clamp(NewTime, 0.0f, 1.0f);
//
//
//    //// for now just duplicate the cxonstraints. Not ideal but it's fine to start
//    //if (auto Bounds = Config.SegmentBounds.Find(CurrentStartId))
//    //{
//    //    Config.SegmentBounds.Add(CurrentDestId, *Bounds);
//    //}
//
//    //// now update the state
//    //ParameterBlock Block;
//    //Populate(Block, Config.ControlPoints[CurrentStartId-1],
//    //    Config.ControlPoints[CurrentStartId], 
//    //    Config.ControlPoints[CurrentStartId + 1], 
//    //    Config.ControlPoints[CurrentStartId + 2]);
//
//    //// Slight hack, since we've already run the restriction we know what the p1 tau should 
//    //// be so set it from the state and recompute A,B
//    ////Block.Tau[0] = State.Tau[0];
//    ////Block.Beta[0] = State.Beta[0];
//    ////ComputeAB(Block);
//
//    //State.Tau[1] = NewDestPoint.Tau;
//    //State.Beta[1] = NewDestPoint.Beta;
//
//    //GenerateCoeffisients(Block.RawPoints, Block, State.PrecomputedCoefficients.GetData());
//    //float NewTime = State.Time / (Alpha > 0.0f ? Alpha : 1000000.0f);
//    //State.Time = FMath::Clamp(NewTime, 0.0f, 1.0f);
//}

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
