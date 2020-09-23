// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "ClimbCharacter.generated.h"


UENUM(BlueprintType)
enum EMantleType {

	LowMantle,
	HightMantle
};

USTRUCT(BlueprintType)
struct FObstacleComponent {

	GENERATED_USTRUCT_BODY()

	USceneComponent* Component = nullptr;
	FTransform ComponentTransform;
	FObstacleComponent() {}
	FObstacleComponent(USceneComponent* comp, FTransform transform)
	{
		Component = comp;
		ComponentTransform = transform;
	}
};

USTRUCT(BlueprintType)
struct FMantleAsset {

	GENERATED_USTRUCT_BODY()
	UPROPERTY(EditDefaultsOnly, Category = "MantleSystem|AssetProperty")
    UAnimMontage* MantleMontage;
	UPROPERTY(EditDefaultsOnly, Category = "MantleSystem|AssetProperty")
	UCurveVector* MantelCurve;
	UPROPERTY(EditDefaultsOnly, Category = "MantleSystem|AssetProperty")
	FVector StartingOffset;
	UPROPERTY(EditDefaultsOnly, Category = "MantleSystem|AssetProperty")
	float LowHeight;
	UPROPERTY(EditDefaultsOnly, Category = "MantleSystem|AssetProperty")
	float LowPlayRate;
	UPROPERTY(EditDefaultsOnly, Category = "MantleSystem|AssetProperty")
	float LowStartingPosition;
	UPROPERTY(EditDefaultsOnly, Category = "MantleSystem|AssetProperty")
	float HightHeight;
	UPROPERTY(EditDefaultsOnly, Category = "MantleSystem|AssetProperty")
	float HightPlayRate;
	UPROPERTY(EditDefaultsOnly, Category = "MantleSystem|AssetProperty")
	float HightStartingPosition;
};

USTRUCT(BlueprintType)
struct FMantleParams {

	GENERATED_USTRUCT_BODY()

	UAnimMontage* MantleMontage;
	UCurveVector* MantelCurve;
	FVector StartingOffset;
	float PlayRate;
	float StartingPosition;
};

UCLASS()
class AIJUMP_API AClimbCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AClimbCharacter();

	UPROPERTY(EditDefaultsOnly, Category = "SpringArm")
		class USpringArmComponent* SpringArm;

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
		class UCameraComponent* ThirdCamera;

protected:

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


public:

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent);
protected:

	/** Find an obstacle that you can climb */
	UFUNCTION(Category = "MantleSystem")
		bool CheckMantle();

	/** GetTrace impact point and normal so that in future calc Downword location and transformtarget 
	    @param OutNormal - set impact normal from trace.
		@param OutImpactPoint - set impact point from trace. */
	UFUNCTION(Category = "MantleSystem")
		bool MantleTrace(FVector& OutNormal, FVector& OutImpactPoint);

	/** Calc Lower location out obstacle 
	    @param ClimbingCharacter - Character to will be climbed. 
		@param MantleNormal - Obstacle trace ImpactNormal.
		@param MantleImpactPoint - Obstacle trace ImpactPoint.
		@return - The lowest position we can climb */
	UFUNCTION(Category = "MantleSystem")
	    FVector FindDownwordLocation(ACharacter* ClimbingCharacter, const FVector MantleNormal, const FVector MantleImpactPoint);

	/** Checking if we can climb ?
	    @param Capsule - ClimbCharacter capsule for based on size, check desired location.
	    @param ClimbTarget - Location to we need to check.*/
	UFUNCTION(BlueprintPure, Category = "MantleSystem")
		bool CapsuleHasRoomCheck(UCapsuleComponent* Capsule, FVector ClimbTarget);

	/** Cacl Desired Transform for climbing 
	    @param LowerLocation - The lowest position we can climb 
		@param EstimateCapsule - Based on half height calc desired climb location */
	UFUNCTION(BlueprintPure, Category = "MantleSystem")
		FTransform GetTargetTransform(const FVector LowerLocation, const FVector InitialTraceNormal, UCapsuleComponent* EstimateCapsule);

	/** Get Offset between our character and target climbing */
	UFUNCTION(BlueprintPure, Category = "MantleSystem")
		float GetMantleHeight(FTransform MantleTarget, FVector CurrentLocation);

	/** Calc required values, start anim montage and timeline.*/
	UFUNCTION(Category = "MantleSystem")
		void StartMantle(FObstacleComponent MantleComponent, float MantleHeight, EMantleType MantleType);

	/** Choose correct MantleAsset params, based on the type */
	UFUNCTION(Category = "MantleSystem")
		FMantleAsset GetMantleAsset(EMantleType Type);

	/** Set required values for FMantleParams based on FMantleAsset 
	    @param Value - For this value usage MantleHeight 
		@warning - For Correct calc MantleParams, use MantleHeight for param Value.*/
	UFUNCTION(Category = "MantleSystem")
		void SetMantleParams(FMantleAsset InAsset, FMantleParams& OutParams, float Value);

	UFUNCTION(BlueprintImplementableEvent, Category = "MantleStystem")
		void StartTimeline(float NewLength, float NewPlayRate);

	/** Calc desired transform for our climbcharacter*/
	UFUNCTION(BlueprintCallable, Category = "MantleSystem")
		void TimelineProgress(float Value, UTimelineComponent* Timeline);
	UFUNCTION(BlueprintCallable, Category = "MantleSystem")
		void TimelineEnded();

	void AMMantleTimer();
	/** Setter for movement mode. */
	//UFUNCTION()
	FORCEINLINE void SetMovementMode(EMovementMode NewMovement) { GetCharacterMovement()->SetMovementMode(NewMovement); }

	/** Getter and Setter for MantleType */
	FORCEINLINE void SetMantleType(EMantleType Type) { HightMantle > 125.f ? Type = EMantleType::HightMantle : Type = EMantleType::LowMantle; }
	FORCEINLINE EMantleType GetMantleType() { return MantleHeightType; }

	/** Converter Our Obstacle component transform to local or world */
	FORCEINLINE FObstacleComponent ConvertWSToLS(FObstacleComponent ConvComponent) { 
		return FObstacleComponent(ConvComponent.Component, ConvComponent.Component->GetComponentTransform().Inverse() * ConvComponent.ComponentTransform); }
	FORCEINLINE FObstacleComponent ConvertLSToWS(FObstacleComponent ConvComponent) { 
		return FObstacleComponent(ConvComponent.Component, ConvComponent.Component->GetComponentTransform() * ConvComponent.ComponentTransform); }

	FTransform TAdd(FTransform A, FTransform B);
	FTransform TSubtract(FTransform A, FTransform B);

protected:

	UPROPERTY(EditDefaultsOnly, Category = "MantleSystem|TraceProperty", meta = (ClampMin = 0.f))
	FVector MantleTraceOffset;

	UPROPERTY(EditDefaultsOnly, Category = "MantleSystem|TraceProperty", meta=(ClampMin = 0.f))
	float TraceDistance;

	UPROPERTY(EditDefaultsOnly, Category = "MantleSystem|TraceProperty", meta = (ClampMin = 0.f))
	float TraceHalfHeight;

	UPROPERTY(EditDefaultsOnly, Category = "MantleSystem|TraceProperty", meta = (ClampMin = 0.f, ClampMax = 50.f))
	float TraceRadius;

	UPROPERTY(EditDefaultsOnly, Category = "MantleSystem")
	UAnimMontage* AM_LowMantle;

	UPROPERTY(EditDefaultsOnly, Category = "MantleSystem")
	UAnimMontage* AM_HighMantle;

	UPROPERTY(EditDefaultsOnly, Category = "MantleSystem")
	UCurveVector* CurveLowMantle;

	UPROPERTY(EditDefaultsOnly, Category = "MantleSystem")
	UCurveVector* CurveHighMantle;

	UPROPERTY(EditDefaultsOnly, Category = "MantleSystem")
	UCurveFloat* TimelineCurve;

	UPROPERTY(EditDefaultsOnly, Category = "MantleSystem")
	FMantleAsset LowMantleAsset;

	UPROPERTY(EditDefaultsOnly, Category = "MantleSystem")
	FMantleAsset HighMantleAsset;

	FMantleAsset MantleAsset;

	FMantleParams MantleParams;

	UPROPERTY(BlueprintReadWrite, Category = "MantleSystem")
	bool bHasMovementInput;

	UPROPERTY(BlueprintReadWrite, Category = "MantleSystem")
	bool bIsJump;


	EMantleType MantleHeightType;

	FVector ImpactPoint;
	FVector ImpactNormal;

	FVector DownwordLocation;

	FTransform TargetTransform;
	FTransform MantleStartedOffset;
	FTransform MantleAnimOffset;

	USceneComponent* HitComponent;

	float MantleHeight;
	

	FObstacleComponent MantleLedgeWS;
	FObstacleComponent MantleLedgeLS;
};
