// Fill out your copyright notice in the Description page of Project Settings.


#include "02_Ability/02_Projectile/ProjectileActor.h"

#include "00_Character/BaseCharacter.h"
#include "00_Character/01_Component/AbilityComponent.h"
#include "00_Character/02_Animation/00_NotifyState/AnimNotifyState_SpawnProjectile.h"
#include "00_Character/03_Monster/00_Controller/MonsterAIController.h"
#include "96_Library/AbilityHelperLibrary.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Logging/StructuredLog.h"

// Sets default values
AProjectileActor::AProjectileActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	SphereComponent->SetCollisionProfileName("Projectile");
	RootComponent = SphereComponent;

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->ProjectileGravityScale = 0.f;
	ProjectileMovementComponent->HomingAccelerationMagnitude = 5000.f;
	ProjectileMovementComponent->SetAutoActivate(false);
}

// Called when the game starts or when spawned
void AProjectileActor::BeginPlay()
{
	Super::BeginPlay();
	SphereComponent->IgnoreActorWhenMoving(GetOwner(),true);	
}

void AProjectileActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void AProjectileActor::DetachifAttached()
{
	if(IsAttachedTo(GetOwner()))
	{
		DetachFromActor(FDetachmentTransformRules(EDetachmentRule::KeepWorld,true));
	}
}

// Called every frame
void AProjectileActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if(CurHomingProjectileType == EHomingProjectileType::HomingButStopTooClosed || CurHomingProjectileType == EHomingProjectileType::HomingWhenClosedButStopTooClosed)
	{
		if (ProjectileMovementComponent->HomingTargetComponent!=nullptr)
		{
			float DistanceToTarget = FVector::Dist(GetActorLocation(), ProjectileMovementComponent->HomingTargetComponent->GetComponentLocation());
			if (DistanceToTarget < TargetMinimumHomingDistance)
			{
				ProjectileMovementComponent->HomingAccelerationMagnitude = 0.f;  // 최소 거리 내에서는 호밍 비활성화
			}
		}
	}


	switch (CurProjectileShootType) {
	case EProjectileShootType::DelayUntilRotation:
	case EProjectileShootType::DelayUntilRotationWithTimeLimit:		
		if (bRotationComplete == false)
		{
			FHitResult Hit;

			TArray<TEnumAsByte<EObjectTypeQuery>> objTypes;
			objTypes.Emplace(UEngineTypes::ConvertToObjectType(ECC_Player));
			
			if(UKismetSystemLibrary::LineTraceSingleForObjects(this,GetActorLocation(),(GetActorForwardVector() * 10000.f) + GetActorLocation(),objTypes,false,TArray<AActor*>(),EDrawDebugTrace::ForOneFrame,Hit,true))
			{
				DetachifAttached();
				bRotationComplete = true;
				ProjectileMovementComponent->Velocity = GetActorForwardVector() * ProjectileSpeed;
				ProjectileMovementComponent->SetActive(true);
				SetActorTickEnabled(false);
				return;
			}
			
			SetActorRotation(
				FMath::RInterpTo(GetActorRotation(),
								 TargetRotation,
								 DeltaTime,
								 InterpSpeed
				));
			
		}
		break;
	}
	
		
	
}

/*
void AProjectileActor::LaunchProjectile(const FVector& ForwardVector, EProjectileType ProjectileType, AActor* HomingTarget, float MinimumHomingDistance)
{
	CurProjectileType = ProjectileType;
	TargetMinimumHomingDistance = MinimumHomingDistance;
	
	DetachifAttached();
	ProjectileMovementComponent->Velocity = ForwardVector * ProjectileSpeed;
	switch (ProjectileType)
	{
	case EProjectileType::Normal:
		break;
	case EProjectileType::Homing:
	case EProjectileType::HomingButStopTooClosed:
		if(HomingTarget!=nullptr)
		{
			ProjectileMovementComponent->HomingTargetComponent = HomingTarget->GetRootComponent();
			ProjectileMovementComponent->bIsHomingProjectile = true;
		}
		break;
	default: ;
	}
	ProjectileMovementComponent->SetActive(true);
	SetActorTickEnabled(true);
}
*/

void AProjectileActor::LaunchProjectile(const FVector& ForwardVector)
{
	DetachifAttached();
	ProjectileMovementComponent->Velocity = ForwardVector * ProjectileSpeed;
	ProjectileMovementComponent->SetActive(true);
	SetActorTickEnabled(true);
}

void AProjectileActor::LaunchHomingProjectile_Implementation(const FVector& ForwardVector, EHomingProjectileType HomingProjectileType,
	AActor* HomingTarget, float MinimumHomingDistance)
{
	CurHomingProjectileType = HomingProjectileType;
	TargetMinimumHomingDistance = MinimumHomingDistance;
	
	DetachifAttached();
	ProjectileMovementComponent->Velocity = ForwardVector * ProjectileSpeed;
	switch (HomingProjectileType)
	{
	case EHomingProjectileType::Homing:
	case EHomingProjectileType::HomingButStopTooClosed:
		if(HomingTarget != nullptr)
		{
			ProjectileMovementComponent->HomingTargetComponent = HomingTarget->GetRootComponent();
			ProjectileMovementComponent->bIsHomingProjectile = true;
		}
		break;
	case EHomingProjectileType::HomingWhenClosed:
		break;
	case EHomingProjectileType::HomingWhenClosedButStopTooClosed:
		break;
	default: ;
	}
	ProjectileMovementComponent->SetActive(true);
	SetActorTickEnabled(true);
}

void AProjectileActor::ShootSetting(EProjectileDirection ProjectileDirection)
{
	switch (ProjectileDirection)
	{
	case EProjectileDirection::Forward:
		SetActorRotation(FRotationMatrix::MakeFromX(GetOwner()->GetActorForwardVector()).Rotator());
		ProjectileMovementComponent->Velocity = GetOwner()->GetActorForwardVector() * ProjectileSpeed;
		break;
	case EProjectileDirection::BlackboardTarget:
		{
			if (auto aiCon = Cast<ACharacter>(GetOwner())->GetController<AMonsterAIController>())
			{
				if (auto blackboard = aiCon->GetBlackboardComponent())
				{
					if (auto target = Cast<AActor>(blackboard->GetValueAsObject("Target")))
					{
						const FRotator& lookRot = UKismetMathLibrary::FindLookAtRotation(
							GetActorLocation(), target->GetActorLocation());
						SetActorRotation(lookRot);
						ProjectileMovementComponent->Velocity = lookRot.Vector() * ProjectileSpeed;
					}
				}
			}
		}
		break;
	default: ;
	}
}

void AProjectileActor::LaunchProjectileWithOption_Implementation(
	EProjectileDirection P_Direction, EProjectileShootType P_ShootType,float LimitTime,class USoundBase* SoundToPlay)
{
	CurProjectileDirectionType = P_Direction;
	CurProjectileShootType = P_ShootType;

	if (auto owner = GetOwner<ABaseCharacter>())
	{


		UGameplayStatics::PlaySoundAtLocation(this,SoundToPlay,GetActorLocation());
		
		switch (P_ShootType)
		{
		case EProjectileShootType::Immediately:
		case EProjectileShootType::ImmediatelyButNotRotationFollowsVelocity:
			ShootSetting(P_Direction);
			break;
		case EProjectileShootType::DelayUntilRotation:
			break;
		case EProjectileShootType::DelayUntilRotationWithTimeLimit:
			break;
		default: ;
		}


		switch (P_ShootType)
		{
		case EProjectileShootType::Immediately:
			ProjectileMovementComponent->bRotationFollowsVelocity = false;
			SetActorTickEnabled(true);
			ProjectileMovementComponent->SetActive(true);
			break;
		case EProjectileShootType::ImmediatelyButNotRotationFollowsVelocity:
			ProjectileMovementComponent->bRotationFollowsVelocity = false;
			SetActorTickEnabled(true);
			ProjectileMovementComponent->SetActive(true);
			break;
		case EProjectileShootType::DelayUntilRotation:
		case EProjectileShootType::DelayUntilRotationWithTimeLimit:
			ProjectileMovementComponent->bRotationFollowsVelocity = true;
			if (CurProjectileDirectionType == EProjectileDirection::Forward)
			{
				TargetRotation = UKismetMathLibrary::FindLookAtRotation(
					GetActorLocation(), GetOwner()->GetActorForwardVector() + GetOwner()->GetActorLocation());
			}
			else
			{
				if (auto aiCon = owner->GetController<AMonsterAIController>())
				{
					if (auto blackboard = aiCon->GetBlackboardComponent())
					{
						if (auto target = Cast<AActor>(blackboard->GetValueAsObject("Target")))
						{
							TargetRotation = UKismetMathLibrary::FindLookAtRotation(
								GetActorLocation(), target->GetActorLocation());
						}
					}
				}
			}

			if(P_ShootType == EProjectileShootType::DelayUntilRotationWithTimeLimit)
			{
				GetWorldTimerManager().SetTimer(ForceShootTimerHandle,this,&AProjectileActor::ForceShoot,LimitTime,false);
			}
			
			SetActorTickEnabled(true);
			break;
		default: ;
		}
	}
}

void AProjectileActor::ForceShoot()
{
	switch (CurProjectileShootType)
	{
	case EProjectileShootType::DelayUntilRotation:
	case EProjectileShootType::DelayUntilRotationWithTimeLimit:
		UE_LOGFMT(LogTemp,Log,"SHOOOOT!!");
		SetActorTickEnabled(false);
		ProjectileMovementComponent->Velocity = GetActorForwardVector() * ProjectileSpeed;
		ProjectileMovementComponent->SetActive(true);
		break;
	}
}

void AProjectileActor::LaunchProjectileDelayWithOption(float DelayTime, EProjectileDirection P_Direction,
                                                       EProjectileShootType P_ShootType, float LimitTime, USoundBase* SoundToPlay)
{
	if(DelayTime<=0)
	{
		LaunchProjectileWithOption(P_Direction,P_ShootType,LimitTime,SoundToPlay);
		return;
	}
	
	if (!GetWorldTimerManager().TimerExists(DelayLaunchTimerHandle))
	{
		FTimerDelegate timerDel = FTimerDelegate::CreateUObject(this, &AProjectileActor::LaunchProjectileWithOption,
		                                                        P_Direction, P_ShootType,LimitTime,SoundToPlay);
		GetWorldTimerManager().SetTimer(DelayLaunchTimerHandle, timerDel, DelayTime, false);
	}
}

void AProjectileActor::SetEffect(const TArray<TSubclassOf<UAbilityEffect>>& Effects)
{
	ProjectileEffects = Effects;
}


void AProjectileActor::ApplyEffects(ABaseCharacter* HitTarget, FVector HitLocation, FVector ImpactNormal)
{
	if (HitTarget && GetOwner() != nullptr)
	{
		if (const auto abComp = HitTarget->GetAbilityComponent())
		{
			auto addInfo = NewObject<UAbilityCueAdditionalInformation>(this);
			addInfo->HitLocation = HitLocation;
			addInfo->ImpactNormal = ImpactNormal;

			abComp->K2_ApplyEffectsWithReturn(ProjectileEffects, GetOwner(), addInfo);
		}
	}
}

void AProjectileActor::ProcessHit(const TArray<FHitResult>& Hits, bool bManualDestroy)
{
	for (const auto &iter : Hits)
	{
		if (iter.GetActor() != GetOwner())
		{
			if (iter.GetActor()->IsA<ABaseCharacter>())
			{
				ApplyEffects(Cast<ABaseCharacter>(iter.GetActor()), iter.Location, iter.ImpactNormal);
			}
			else
			{
				if (GetOwner() != nullptr && GetOwner()->IsA<ABaseCharacter>())
				{
					if (VisibilityHitCue.IsValid())
					{
						VisibilityHitCue.SpawnLocation = iter.Location;
						GetOwner<ABaseCharacter>()->GetAbilityComponent()->ApplyCue(VisibilityHitCue);
					}
				}
			}

			if (!bManualDestroy)
			{
				SetActorTickEnabled(false);
				Destroy();
			}
		}
	}
}

UAbilityComponent* AProjectileActor::GetOwnersAbilityComponent()
{
	return GetOwner<ABaseCharacter>()->GetAbilityComponent();
}

void AProjectileActor::ApplyCue(const FAbilityCueInformation& CueInformation)
{
	GetOwnersAbilityComponent()->ApplyCue(CueInformation);
}
