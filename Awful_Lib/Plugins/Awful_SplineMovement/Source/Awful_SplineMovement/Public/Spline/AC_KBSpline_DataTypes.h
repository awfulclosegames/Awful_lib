// Copyright Strati D. Zerbinis, 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "Containers/RingBuffer.h"

#include "AC_KBSpline_DataTypes.generated.h"


USTRUCT(BlueprintType)
struct AWFUL_SPLINEMOVEMENT_API FKBSplinePoint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Movement")
	FVector Location = { 0.0f, 0.0f, 0.0f };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Movement")
	float Tau = -1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Movement")
	float Beta = 0.0f;
};


USTRUCT(BlueprintType)
struct FKBAnchorPoint
{
	GENERATED_BODY()

	// THIS is very much a WiP. 
	// among the things I'm trying to decide:
	//		- should this be in absolute extents?
	//		- should I require an anchor point and make the extents relative?
	//		- should I make the extents radial or axial?
	//		- should I require constraints to be coherent on a single acnchor point for multiple segments?
	//			A- I think probably not. Letting the constraints be different between consecuative segments at a junction
	//				could be pretty useful and shouldn't cause any issues

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Movement")
	FKBSplinePoint Point;

	// this could be a radius instead. Assume tetragons for now
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Movement")
	FVector MinBound = { 0.0f, 0.0f, 0.0f };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Movement")
	FVector MaxBound = { 0.0f, 0.0f, 0.0f };

};


USTRUCT(BlueprintType)
struct AWFUL_SPLINEMOVEMENT_API FKBSplineBounds
{
	GENERATED_BODY()

	enum AnchorPointTypess
	{
		FromPoint = 0,
		ToPoint = 1,
	};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Movement")
	TArray<FKBAnchorPoint> Anchors;

};

// should this be a class instead of struct so I don't need to copy 36 bytes all the time?
USTRUCT(BlueprintType)
struct AWFUL_SPLINEMOVEMENT_API FKBSplineState
{
	GENERATED_BODY()

	enum WorkingSetPointTypes
	{
		PreviousPoint = 0,
		FromPoint = 1,
		ToPoint = 2,
		NextPoint = 3,
		NumberOfPoints = 4,
	};


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spline Movement")
	int CurrentTraversalSegment = -1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, EditFixedSize, Category = "Spline Movement")
	TArray<FVector> PrecomputedCoefficients;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Movement")
	float Time = 0.0f;

	UPROPERTY()
	FKBSplinePoint WorkingSet[WorkingSetPointTypes::NumberOfPoints];

	// this limited. Could reuse a negative time or other flag value instead. Should probably compact this structure when I get time
	UPROPERTY(BlueprintReadOnly, Category = "Spline Movement")
	bool Valid = false;

	// ************************************************************************
	// SUPPLIMENTARY STATE
	// ************************************************************************
	// 
	// I don't love putting this here, but since the restriction may change the
	// tensioning I need to store it after success. Ideally I should split off 
	// core state and supplimentary state since this isn't needed for sampling
	float Tau[2];
	float Beta[2];

	float UndulationTimes[2] = { -1.0f, -1.0f };

#if !UE_BUILD_SHIPPING
	FVector OriginalCoeffs[4] = { {0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	float OriginalUndulationTimes[2] = { -1.0f, -1.0f };
#endif

	FKBSplineState();

	void Reset();

	bool IsValidSegment() const;

};
 

UCLASS(BlueprintType)
class AWFUL_SPLINEMOVEMENT_API UKBSplineConfig : public UObject
{
	GENERATED_BODY()
public:

	TRingBuffer<FKBSplinePoint> ControlPoints;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spline Movement")
	TMap<int, FKBSplineBounds> SegmentBounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spline Movement")
	FKBSplinePoint OriginPoint;

	/** Distance from the head of the control points list that must be consumed before any new changes can take effect
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spline Movement")
	int CommitPoint = 0;

	UKBSplineConfig() : Super() {}

	UKBSplineConfig(FVector Location);
	UKBSplineConfig(FVector Location, int NumPoints);

	void PeekSegment(int SegmentID, FKBSplinePoint Points[4]) const;
	void ConsumeSegment(int SegmentID);

	void GetTravelChord(int SegmentID, FVector& outChord);

	bool IsValidSegment(int SegmentID) const;

	void Add(FKBSplinePoint& Point);
	int GetLastSegment()const;

	void ClearToCommitments();
	void Reset();

	int GetNextCandidateSegment(int SegmentID) const;

	// I believe this is only exposed for debug
	int NormalizeSegmentID(int SegmentID) const {return SegmentID - MinimumSegment;}

private:
	static const int sDefaultBufferLength;

	bool IsValidNormalizedSegment(int SegmentID) const;

	int MinimumSegment = 0;
};

