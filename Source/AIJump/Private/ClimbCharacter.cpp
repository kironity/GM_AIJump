// Fill out your copyright notice in the Description page of Project Settings.


#include "ClimbCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Components/TimelineComponent.h"
#include "Kismet/KismetStringLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Curves/CurveVector.h"
#include "Animation/AnimInstance.h"
#include "Kismet/GameplayStatics.h"
//#include "TimerManager.h"

// Sets default values
AClimbCharacter::AClimbCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(GetRootComponent());
	ThirdCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	ThirdCamera->SetupAttachment(SpringArm);

	//Set Default Property to MantleTrace
	TraceDistance = 50.f;
	TraceHalfHeight = 50.f;
	TraceRadius = 15.f;
	bIsJump = false;
}

// Called when the game starts or when spawned
void AClimbCharacter::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AClimbCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bHasMovementInput && bIsJump)
	{
		CheckMantle();
	}

//	MantleTimeline.TickTimeline(DeltaTime);
}

// Called to bind functionality to input
void AClimbCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

bool AClimbCharacter::CheckMantle()
{
	if (MantleTrace(ImpactNormal, ImpactPoint))
	{
		DownwordLocation = FindDownwordLocation(this, ImpactNormal, ImpactPoint);
		if (DownwordLocation.Size() > 0.f)
		{
		//	UKismetSystemLibrary::DrawDebugSphere(this, DownwordLocation, 5.f, 5, FColor::Green, 5.f, 5.f);
			TargetTransform = GetTargetTransform(DownwordLocation, ImpactNormal, GetCapsuleComponent());
			if (CapsuleHasRoomCheck(GetCapsuleComponent(), TargetTransform.GetLocation()))
			{
				MantleHeight = GetMantleHeight(TargetTransform, GetActorLocation());

				MantleHeight > 125.f ? MantleHeightType = EMantleType::HightMantle : MantleHeightType = EMantleType::LowMantle;

				MantleLedgeWS.ComponentTransform = TargetTransform;
				MantleLedgeWS.Component = HitComponent;

				StartMantle(MantleLedgeWS, MantleHeight, MantleHeightType);
			}
		}
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Blue, FString("CheckMantle Return False"));
		return false;
	}

	return true;
}

bool AClimbCharacter::MantleTrace(FVector& MantleNormal, FVector& MantlePoint)
{

	FVector ForwardDirection = GetActorForwardVector() * TraceDistance;
	FVector BackwardDirection = GetActorForwardVector() * TraceDistance * -1;

	FVector StartTrace = GetActorLocation() + BackwardDirection + MantleTraceOffset;
	FVector EndTrace = GetActorLocation() + ForwardDirection + MantleTraceOffset;

	TArray <TEnumAsByte <EObjectTypeQuery>> ObjectTraces;
	ObjectTraces = { ObjectTypeQuery1, ObjectTypeQuery2 };

	const TArray <AActor*> CharactersToIgnor;
	FHitResult  Hit;

	UKismetSystemLibrary::CapsuleTraceSingleForObjects(this, StartTrace, EndTrace, TraceRadius, TraceHalfHeight,
		ObjectTraces, false, CharactersToIgnor, EDrawDebugTrace::ForOneFrame, Hit, true);

	if (Hit.bBlockingHit && !GetCharacterMovement()->IsWalkable(Hit) && !Hit.bStartPenetrating)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Emerald, FString("True"));
		MantlePoint = Hit.ImpactPoint;
		MantleNormal = Hit.ImpactNormal;

		return true;
	}
	else
	{
		FString HitValue = UKismetStringLibrary::Conv_BoolToString(GetCharacterMovement()->IsWalkable(Hit));
		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString(HitValue));
		return false;
	}

	return false;
}

FVector AClimbCharacter::FindDownwordLocation(ACharacter* ClimbingCharacter, const FVector MantleNormal, const FVector MantleImpactPoint)
{
	if (ClimbingCharacter != nullptr && ClimbingCharacter->GetCapsuleComponent() != nullptr)
	{
		UCapsuleComponent* CharacterCapsule = ClimbingCharacter->GetCapsuleComponent();

			FVector XYPosition = (MantleNormal * -15.f) + MantleImpactPoint;
			FVector StartTrace = FVector(XYPosition.X, XYPosition.Y,
				CharacterCapsule->GetScaledCapsuleHalfHeight() + 
				ClimbingCharacter->GetActorLocation().Z);

			FVector EndTrace = FVector(XYPosition.X, XYPosition.Y, 
				ClimbingCharacter->GetActorLocation().Z - CharacterCapsule->GetScaledCapsuleHalfHeight());

			TArray <TEnumAsByte <EObjectTypeQuery>> ObjectTraces;
			ObjectTraces = { ObjectTypeQuery1, ObjectTypeQuery2 };

			const TArray <AActor*> CharactersToIgnor;
			FHitResult  Hit;

			UKismetSystemLibrary::SphereTraceSingleForObjects(ClimbingCharacter, StartTrace, EndTrace, 30.f,
				ObjectTraces, false, CharactersToIgnor, EDrawDebugTrace::None, Hit, true);
			if (Hit.bBlockingHit && ClimbingCharacter->GetCharacterMovement()->IsWalkable(Hit))
			{
				HitComponent = Hit.GetComponent();
				return FVector(Hit.Location.X, Hit.Location.Y, Hit.ImpactPoint.Z);
			}
	}
	UE_LOG(LogTemp, Warning, TEXT("FindDownwordLocation Return Vector == 0 ;"));
	return FVector(0.f);
}

bool AClimbCharacter::CapsuleHasRoomCheck(UCapsuleComponent* Capsule, FVector TargetLocation)
{
	if (Capsule != nullptr)
	{
		float CapsuleSize = Capsule->GetScaledCapsuleHalfHeight_WithoutHemisphere();
		FVector StartTrace = TargetLocation + FVector(0.f, 0.f, CapsuleSize);
		FVector EndTrace = TargetLocation - FVector(0.f, 0.f, CapsuleSize);

		FHitResult Hit;

		TArray<TEnumAsByte<EObjectTypeQuery>> CollisionObjects;
		CollisionObjects = { ObjectTypeQuery1, ObjectTypeQuery2 };

		TArray<AActor*> CharactersToIgnor;

		UKismetSystemLibrary::SphereTraceSingleForObjects(this, StartTrace, EndTrace, Capsule->GetScaledCapsuleRadius(),
			CollisionObjects, false, CharactersToIgnor, EDrawDebugTrace::None, Hit, true);

		if (!Hit.bBlockingHit || !Hit.bStartPenetrating)
		{
			UE_LOG(LogTemp, Warning, TEXT("HalfCheckReturnTrue"));
			return true;
		}
	}
	return false;
}

float AClimbCharacter::GetMantleHeight(FTransform MantleTarget, FVector CurrentLocation)
{
	FVector Distance = MantleTarget.GetLocation() - CurrentLocation;
	return Distance.Z;
}

FTransform AClimbCharacter::GetTargetTransform(const FVector LowerLocation, const FVector InitialTraceNormal, UCapsuleComponent* EstimateCapsule)
{
	FVector MantleLocation = LowerLocation + FVector(0.f, 0.f, EstimateCapsule->GetScaledCapsuleHalfHeight() + 2.f);

	FRotator MantleRotation = UKismetMathLibrary::MakeRotFromX(InitialTraceNormal * FVector(-1.f, -1.f, 0.f));

	FTransform MantleTransform = FTransform(MantleRotation, MantleLocation, FVector::OneVector);

	return MantleTransform;
}

void AClimbCharacter::StartMantle(FObstacleComponent MantleComponent, float MantleHieght, EMantleType MantleType)
{
	bIsJump = true;
	MantleAsset = GetMantleAsset(MantleType);
	MantleLedgeLS = ConvertWSToLS(MantleLedgeWS);
	TargetTransform = MantleLedgeWS.ComponentTransform;
	SetMantleParams(MantleAsset, MantleParams, MantleHeight);

	FVector ComponentLocation = TargetTransform.GetLocation();
	FRotator ComponentRotation = TargetTransform.Rotator();

	FVector Direction = ComponentRotation.Vector();

	FVector AnimLocOffset = Direction * FVector(MantleParams.StartingOffset.Y,MantleParams.StartingOffset.Y,0.f);

	AnimLocOffset = FVector(AnimLocOffset.X, AnimLocOffset.Y, MantleParams.StartingOffset.Z);

	FTransform HeightOffset = FTransform(ComponentRotation, ComponentLocation - AnimLocOffset, FVector::OneVector);

	MantleStartedOffset = TSubtract(GetActorTransform(), TargetTransform);
	MantleAnimOffset = TSubtract(HeightOffset,TargetTransform);

	if (TimelineCurve != nullptr)
	{
 		if (MantleParams.MantleMontage != nullptr && MantleParams.MantelCurve != nullptr && this->GetMesh()->GetAnimInstance() != nullptr)
		{
			
			SetMovementMode(EMovementMode::MOVE_Flying);

			GetCharacterMovement()->StopMovementImmediately();
			
			float MinTime;
			float MaxTime; 
			MantleParams.MantelCurve->GetTimeRange(MinTime, MaxTime);
			
			StartTimeline(MaxTime - MantleParams.StartingPosition, MantleParams.PlayRate);
			
			GetMesh()->GetAnimInstance()->Montage_Play(MantleParams.MantleMontage, MantleParams.PlayRate,
				EMontagePlayReturnType::MontageLength, MantleParams.StartingPosition, false);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Mantle montage or mantle curve nullptr !"));
		}
	}
	else 
	{
		GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Red, FString("Float Curve In ClimbCharacter Not Valid !"));
		UE_LOG(LogTemp, Warning, TEXT("Float Curve In ClimbCharacter Not Valid !"));
	}
}

FMantleAsset AClimbCharacter::GetMantleAsset(EMantleType Type)
{
	switch (Type)
	{
	case LowMantle:
		MantleAsset = { AM_LowMantle, CurveLowMantle, FVector(0.f, 65.f, 100.f), 50.f, 1.f, 0.5f, 100.f, 1.f, 0.6f };
		return MantleAsset;
		break;
	case HightMantle:
		MantleAsset = { AM_HighMantle, CurveHighMantle, FVector(0.f, 65.f, 200.f), 125.f, 1.2f, 0.6f, 200.f, 1.2f, 0.0f };
		return MantleAsset;
		break;
	default:
		return FMantleAsset();
		break;
	}
}

void AClimbCharacter::SetMantleParams(FMantleAsset InAsset, FMantleParams& OutParams, float Value)
{
	OutParams.MantelCurve = InAsset.MantelCurve;
	OutParams.MantleMontage = InAsset.MantleMontage;

	OutParams.PlayRate = UKismetMathLibrary::MapRangeClamped(Value, InAsset.LowHeight, InAsset.HightHeight, 
		InAsset.LowPlayRate, InAsset.HightPlayRate);

	OutParams.StartingPosition = UKismetMathLibrary::MapRangeClamped(Value, InAsset.LowHeight, InAsset.HightHeight,
		InAsset.LowStartingPosition, InAsset.HightStartingPosition);
	OutParams.StartingOffset = InAsset.StartingOffset;
}

void AClimbCharacter::TimelineProgress(float Value, UTimelineComponent* Timeline)
{
	if (MantleParams.MantelCurve != nullptr)
	{
		TargetTransform = ConvertLSToWS(MantleLedgeLS).ComponentTransform;
		FVector CurrentCurveValue = MantleParams.MantelCurve->GetVectorValue(
			MantleParams.StartingPosition + Timeline->GetPlaybackPosition());

		float PositionAlpha = CurrentCurveValue.X;
		float XYCorrectionAlpha = CurrentCurveValue.Y;
		float ZCorrectionAlpha = CurrentCurveValue.Z;

		FTransform BlendetOffsetAnim = FTransform(MantleAnimOffset.Rotator(), FVector(MantleAnimOffset.GetLocation().X,
			MantleAnimOffset.GetLocation().Y, MantleStartedOffset.GetLocation().Z), FVector::OneVector);

		FTransform StartOffset = FTransform(MantleStartedOffset.Rotator(), FVector(MantleStartedOffset.GetLocation().X,
			MantleStartedOffset.GetLocation().Y, MantleAnimOffset.GetLocation().Z), FVector::OneVector);

		FTransform AnimLerp = UKismetMathLibrary::TLerp(MantleStartedOffset, BlendetOffsetAnim, XYCorrectionAlpha);
		FTransform StartLerp = UKismetMathLibrary::TLerp(MantleStartedOffset, StartOffset, ZCorrectionAlpha);

		FTransform TraslationOffset = TAdd(TargetTransform, FTransform(FRotator(AnimLerp.GetRotation()), FVector(AnimLerp.GetLocation().X, AnimLerp.GetLocation().Y,
			StartOffset.GetLocation().Z), FVector::OneVector));

		FTransform ZOffset = UKismetMathLibrary::TLerp(TraslationOffset, TargetTransform, PositionAlpha);
		FTransform InitialBlend = TAdd(TargetTransform, MantleStartedOffset);

		FTransform LerpedTarget = UKismetMathLibrary::TLerp(InitialBlend, ZOffset, Value);

		SetActorLocationAndRotation(FVector(LerpedTarget.GetLocation()), FRotator(LerpedTarget.GetRotation()));
	}
	else 
	{
		UE_LOG(LogTemp, Warning, TEXT("MantleCurve == nullptr"));
	}
}

void AClimbCharacter::TimelineEnded()
{
	//bIsJump = false;
	SetMovementMode(EMovementMode::MOVE_Walking);
	TimelineCurve->FloatCurve.Reset();
}

void AClimbCharacter::AMMantleTimer()
{
	SetMovementMode(EMovementMode::MOVE_Walking);
}

FTransform AClimbCharacter::TAdd(FTransform A, FTransform B)
{
	FRotator NewRotation = FRotator(A.Rotator().Pitch + B.Rotator().Pitch, A.Rotator().Yaw + B.Rotator().Yaw,
		A.Rotator().Roll + B.Rotator().Roll);
	FVector NewLocation = FVector(A.GetLocation() + B.GetLocation());
	FVector NewScale3D = FVector::OneVector;
	return FTransform(NewRotation, NewLocation, NewScale3D);
}

FTransform AClimbCharacter::TSubtract(FTransform A, FTransform B)
{
	FRotator NewRotation = FRotator(A.Rotator().Pitch - B.Rotator().Pitch, A.Rotator().Yaw - B.Rotator().Yaw,
		A.Rotator().Roll - B.Rotator().Roll);
	FVector NewLocation = FVector(A.GetLocation() - B.GetLocation());
	FVector NewScale3D = FVector::OneVector;
	return FTransform(NewRotation, NewLocation, NewScale3D);
}

