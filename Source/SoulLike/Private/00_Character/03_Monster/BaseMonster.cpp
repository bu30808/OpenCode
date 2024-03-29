// Fill out your copyright notice in the Description page of Project Settings.


#include "00_Character/03_Monster/BaseMonster.h"

#include "AIController.h"
#include "BrainComponent.h"
#include "00_Character/00_Player/PlayerCharacter.h"
#include "00_Character/01_Component/AttributeComponent.h"
#include "00_Character/03_Monster/00_Controller/MonsterAIController.h"
#include "00_Character/03_Monster/01_Component/ItemDropComponent.h"
#include "01_GameMode/SoulLikeGameMode.h"
#include "02_Ability/AbilityEffect.h"
#include "03_Widget/02_Monster/HealthBarWidget.h"
#include "96_Library/AbilityHelperLibrary.h"
#include "96_Library/AIConHelperLibrary.h"
#include "96_Library/SaveGameHelperLibrary.h"
#include "97_Interface/BossMonsterInterface.h"
#include "98_GameInstance/SoulLikeInstance.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Logging/StructuredLog.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Damage.h"

DEFINE_LOG_CATEGORY(LogMonster)

ABaseMonster::ABaseMonster()
{
	GetCapsuleComponent()->SetCollisionProfileName("Pawn");
	GetMesh()->SetCollisionProfileName("MonsterMesh");
	GetMesh()->SetBodyNotifyRigidBodyCollision(true);
	GetMesh()->SetGenerateOverlapEvents(true);
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickMontagesWhenNotRendered;

	HealthBarWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarWidgetComponent"));
	HealthBarWidgetComponent->SetupAttachment(GetMesh(), "HealthBar");

	HealthBarWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	HealthBarWidgetComponent->SetDrawSize(FVector2D(125, 5));

	static ConstructorHelpers::FClassFinder<UUserWidget> widget(TEXT(
		"/Script/UMGEditor.WidgetBlueprint'/Game/Blueprints/02_Widget/UMG_HealthBar.UMG_HealthBar_C'"));
	if (widget.Succeeded())
	{
		HealthBarWidgetComponent->SetWidgetClass(widget.Class);
	}


	ItemDropComponent = CreateDefaultSubobject<UItemDropComponent>(TEXT("ItemDropComponent"));

	//이 두개의 옵션으로 AI가 MoveTo 함수 등으로 네비메시에서 이동할 때, 보간없이 휙휙 도는 현상이 제거됩니다.
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	AIControllerClass = AMonsterAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	/*GetArrowComponent()->ArrowColor = FColor::Blue;
	GetArrowComponent()->ArrowSize = 5.f;*/
}

void ABaseMonster::K2_SpawnAlly_Implementation(int32 SpawnCount)
{
	OnFinishSpawnAllyEvent.Broadcast(Cast<UBehaviorTreeComponent>(GetController<AAIController>()->GetBrainComponent()),
	                                 EBTNodeResult::Succeeded);
}

void ABaseMonster::SpawnAlly(int32 SpawnCount, const FOnFinishSpawnAlly& OnFinishSpawnAlly)
{
	OnFinishSpawnAllyEvent = OnFinishSpawnAlly;
	K2_SpawnAlly(SpawnCount);
	/*OnFinishSpawnAlly.Broadcast(Cast<UBehaviorTreeComponent>(GetController<AAIController>()->GetBrainComponent()),EBTNodeResult::Succeeded);*/
}

void ABaseMonster::ResetMonster(const FTransform& DefaultTr)
{
	//블랙보드 초기화
	if (auto aiCon = Cast<AMonsterAIController>(GetController()))
	{
		if (auto brain = aiCon->GetBrainComponent())
		{
			brain->StopLogic("Reset");
			aiCon->ResetBlackboard();
		}

		if (MonsterDataAsset->bStartBehaviorTreeImmediately)
		{
			aiCon->StartBehavior();
		}
	}

	//사망인 경우 사망 해제
	if (IsDead())
	{
		CharacterState = ECharacterState::NORMAL;
		GetMesh()->SetAllBodiesSimulatePhysics(false); 
		GetCapsuleComponent()->SetCollisionProfileName("Pawn");
		GetMesh()->SetCollisionProfileName("MonsterMesh");
		GetWorldTimerManager().ClearTimer(DeadTimerHandle);
	}

	//어트리뷰트 초기화
	if (AttributeComponent)
	{
		AttributeComponent->InitAttributes();
	}

	//UE_LOGFMT(LogTemp, Log, "{0}의 트렌스폼을 {1}로 되돌립니다.", GetActorNameOrLabel(), DefaultTr.ToString());
	//위치 복구
	SetActorTransform(DefaultTr);
}

void ABaseMonster::BeginPlay()
{
	Super::BeginPlay();

	//휴식시 되살리기 위해 저장합니다.
	if (MonsterDataAsset->bRespawnWhenPlayerRest)
	{
		if (auto gameMode = Cast<ASoulLikeGameMode>(UGameplayStatics::GetGameMode(this)))
		{
			//만약 사망으로 기록되어있는데 다시 호출되었다면, 사망상태로 전환합니다.
			//사망상태가 아니라면, 저장된 어트리뷰트값이 있다면 복구합니다
			gameMode->RestoreMonsterState(this);
			gameMode->AddToRespawnMonster(this);
			
			OnDead.AddUniqueDynamic(gameMode, &ASoulLikeGameMode::OnDeadMonsterEvent);
		}
	}

	if(!IsDead())
	{
		if(IsStartBehaviorTreeImmediately()){
			GetController<AMonsterAIController>()->StartImmediatelyBehavior();
		}
	}
}

void ABaseMonster::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (MonsterDataAsset == nullptr)
	{
		UE_LOGFMT(LogMonster, Error, "몬스터 클래스 내부 변수인 MonsterDataAsset가 비었습니다. 실행하면 큰 문제가 생길 것입니다.");
	}

	if (HealthBarWidgetComponent)
	{
		HealthBarWidgetComponent->InitWidget();
		if (HealthBarWidgetComponent->GetUserWidgetObject())
		{
			HealthBarWidgetComponent->GetUserWidgetObject()->SetVisibility(ESlateVisibility::Collapsed);
		}

		HealthBarWidgetComponent->AttachToComponent(
			GetMesh(), FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), "HealthBar");
	}
	AttributeComponent->OnChangeHPValue.AddUniqueDynamic(this, &ABaseMonster::UpdateHealthBar);

	if (auto aiCon = Cast<AMonsterAIController>(GetController()))
	{
		aiCon->OverrideSightConfig(MonsterDataAsset);
	}

	if (UKismetSystemLibrary::DoesImplementInterface(this, UBossMonsterInterface::StaticClass()))
	{
		OnDead.AddUniqueDynamic(this, &ABaseMonster::OnDeadBossEvent);
	}

	OnEndPlay.AddUniqueDynamic(this, &ABaseMonster::OnEndPlayEvent);
	OnDestroyed.AddUniqueDynamic(this, &ABaseMonster::OnDestroyedEvent);

	DefaultMeshTr = GetMesh()->GetRelativeTransform();
	DefaultHealthBarTr = HealthBarWidgetComponent->GetRelativeTransform();

	InitTransform = GetActorTransform();
}

void ABaseMonster::DetachDroppedItem()
{
	if(DroppedItem != nullptr)
	{
		UE_LOGFMT(LogMonster,Log,"{0} : 붙은 아이템을 때어냅니다.",GetNameSafe(this));
		DroppedItem->K2_DetachFromActor();
		DroppedItem->GetSphereComponent()->SetSimulatePhysics(true);
	}
}

void ABaseMonster::OnEndPlayEvent(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
	/*DetachDroppedItem();*/
	
	if (EndPlayReason == EEndPlayReason::RemovedFromWorld)
	{
		if (const auto gameMode = Cast<ASoulLikeGameMode>(UGameplayStatics::GetGameMode(this)))
		{
			
			//사망 상태가 아닌데 언로드 되었다면, 어트리뷰트 상태를 저장합니다. 다음에 로드되었을 때 복구합니다.
			if (!IsDead())
			{
				gameMode->SaveMonsterAttributeWhenUnload(this);
			}

			if (MonsterDataAsset->bRespawnWhenPlayerRest)
			{
				gameMode->RemoveFromRespawnMonster(this);
			}
		}
	}

	if (EndPlayReason == EEndPlayReason::Destroyed)
	{
		DetachDroppedItem();
		UE_LOGFMT(LogMonster, Warning, "몬스터 파괴됨 : {0}", GetNameSafe(this));
	}
}

void ABaseMonster::OnDestroyedEvent(AActor* DestroyedActor)
{
	if (UKismetSystemLibrary::DoesImplementInterface(this, UBossMonsterInterface::StaticClass()))
	{
		IBossMonsterInterface::Execute_RemoveBossWidget(this, this, UGameplayStatics::GetPlayerCharacter(this, 0));
	}
}

void ABaseMonster::UpdateHealthBar(float Value, float MaxValue)
{
	if (HealthBarWidgetComponent->IsValidLowLevel() == false)
	{
		return;
	}

	if (IsDead())
	{
		return;
	}

	if (HealthBarWidgetComponent->GetUserWidgetObject())
	{
		if (auto widget = Cast<UHealthBarWidget>(HealthBarWidgetComponent->GetUserWidgetObject()))
		{
			widget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			FTimerDelegate hiddenTimerDel = FTimerDelegate::CreateUObject(
				widget, &UUserWidget::SetVisibility, ESlateVisibility::Collapsed);
			GetWorldTimerManager().SetTimer(HealthBarVisibleTimerHandle, hiddenTimerDel, 3.f, false);

			widget->UpdateProgress(Value, MaxValue);
		}
		else
		{
			UE_LOGFMT(LogTemp, Error, "{0} {1}", __FUNCTION__, __LINE__);
		}
	}
	else
	{
		UE_LOGFMT(LogTemp, Error, "{0} {1}", __FUNCTION__, __LINE__);
	}
}

void ABaseMonster::EnableRagdoll() const
{
	if (MonsterDataAsset->RagdollPhysics)
	{
		GetMesh()->SetPhysicsAsset(MonsterDataAsset->RagdollPhysics);
	}

	GetCapsuleComponent()->SetCollisionProfileName("Spectator");

	GetMesh()->SetCollisionProfileName("Ragdoll");
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	GetMesh()->SetAllBodiesSimulatePhysics(true); // 모든 바디를 물리 시뮬레이션으로 활성화합니다.
	GetMesh()->WakeAllRigidBodies();
}

void ABaseMonster::StopAITree() const
{
	if (const auto aiCon = GetController<AMonsterAIController>())
	{
		UE_LOGFMT(LogMonster,Warning,"AI 비활성화 : {0}",GetNameSafe(this));
		aiCon->GetPerceptionComponent()->ForgetAll();
		aiCon->GetPerceptionComponent()->Deactivate();
		if (aiCon->GetBrainComponent() != nullptr)
		{
			if(const auto btTreeComp = Cast<UBehaviorTreeComponent>(aiCon->GetBrainComponent()))
			{
				btTreeComp->StopTree();
				aiCon->ResetBlackboard();
			}
		}
	}

	GetCharacterMovement()->StopMovementImmediately();
}

void ABaseMonster::RunAITree() const
{
	GetController<AMonsterAIController>()->StartBehavior();
}

void ABaseMonster::DeadPresetting() const
{
	StopAITree();

	GetCapsuleComponent()->SetCollisionProfileName("Spectator");
	GetMesh()->SetCollisionProfileName("Spectator");
}

void ABaseMonster::RunDeactivateTimer()
{
	FTimerDelegate deadTimerDel = FTimerDelegate::CreateUObject(this, &ABaseMonster::Deactivate);
	GetWorldTimerManager().SetTimer(DeadTimerHandle, deadTimerDel, DeactivateTime, false);
}

void ABaseMonster::RestoreComponentAttachment() const
{
	GetMesh()->AttachToComponent(RootComponent, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true));
	GetMesh()->SetRelativeTransform(DefaultMeshTr);

	HealthBarWidgetComponent->AttachToComponent(RootComponent,
	                                            FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true));
	HealthBarWidgetComponent->SetRelativeTransform(DefaultHealthBarTr);
}

void ABaseMonster::RestoreFromRagdoll()
{
	GetCapsuleComponent()->SetCollisionProfileName("Pawn");
	GetMesh()->SetCollisionProfileName("MonsterMesh");

	RestoreComponentAttachment();
	
	SetActorEnableCollision(true);
	SetActorHiddenInGame(false);
	GetCharacterMovement()->GravityScale = 1;
	SetCharacterState(ECharacterState::NORMAL);
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
}

void ABaseMonster::Activate()
{
	UE_LOGFMT(LogMonster, Log, "{0} 활성화", GetNameSafe(this));
	GetMesh()->SetAllBodiesSimulatePhysics(false); // 모든 바디를 물리 시뮬레이션으로 활성화합니다.
	GetWorldTimerManager().ClearTimer(DeadTimerHandle);
	SetActorTransform(InitTransform);

	if (const auto gameMode = Cast<ASoulLikeGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		gameMode->AddToRespawnMonster(this);
	}

	if (AttributeComponent)
	{
		AttributeComponent->InitAttributes();
	}

	RestoreFromRagdoll();
	DetachDroppedItem();
	RunAITree();
}

void ABaseMonster::Deactivate()
{
	UE_LOGFMT(LogMonster, Log, "{0} 비활성화", GetNameSafe(this));
	
	StopAITree();
	SetActorHiddenInGame(true);
	GetCharacterMovement()->GravityScale = 0;
	SetActorEnableCollision(false);
}

void ABaseMonster::OnDeadEvent(AActor* Who, AActor* DeadBy)
{
	Super::OnDeadEvent(Who, DeadBy);

	UE_LOGFMT(LogMonster, Warning, "몬스터 사망 이벤트 호출 : {0}", GetNameSafe(this));

	if (ItemDropComponent)
	{
		if (DeadBy != nullptr)
		{
			DeadPresetting();

			if (DeadBy->IsA<ABaseCharacter>())
			{
				if (!UKismetSystemLibrary::DoesImplementInterface(this, UBossMonsterInterface::StaticClass()))
				{
					DroppedItem =  ItemDropComponent->DropItem(Cast<ABaseCharacter>(DeadBy));
				}

				ItemDropComponent->GiveExp(Cast<ABaseCharacter>(DeadBy));
			}

			//캐릭터 특성 적용
			if (DeadBy->IsA<APlayerCharacter>())
			{
				Cast<APlayerCharacter>(DeadBy)->GetAbilityTalentComponent()->BroadcastOnKillMonster(
					Cast<ABaseCharacter>(DeadBy), Cast<ABaseCharacter>(Who));
			}


			switch(DeadAnimationPlayMode)
			{
			case EDeadAnimationPlayMode::None:
				EnableRagdoll();
				RunDeactivateTimer();
				break;
			default: ;
			}
			
		}
	}
}

void ABaseMonster::OnDeadBossEvent(AActor* Who, AActor* DeadBy)
{
	if (UKismetSystemLibrary::DoesImplementInterface(this, UBossMonsterInterface::StaticClass()))
	{
		IBossMonsterInterface::Execute_RemoveBossWidget(this, this, DeadBy);
		UAIConHelperLibrary::ChangePlayerState(Who,DeadBy,EPlayerCharacterState::Peaceful);
		ItemDropComponent->BossDropItem(Cast<ABaseCharacter>(DeadBy));
		USaveGameHelperLibrary::SaveKillBoss(this);
	}
}

void ABaseMonster::PlayDeadAnimationSequence()
{
	Super::PlayDeadAnimationSequence();
	RunDeactivateTimer();
}

void ABaseMonster::PlayDeadAnimationMontage()
{
	Super::PlayDeadAnimationMontage();
	
	if (DeadMontage != nullptr && GetMesh()->GetAnimInstance() != nullptr)
	{
		GetMesh()->GetAnimInstance()->OnMontageEnded.AddUniqueDynamic(
			this, &ABaseMonster::OnDeadMontageEndedEvent);
		PlayAnimMontage(DeadMontage);

		RunDeactivateTimer();
	}
}

void ABaseMonster::OnDeadMontageEndedEvent(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == DeadMontage)
	{
		if (auto interface = Cast<IBossMonsterInterface>(this))
		{
			Destroy();
			return;
		}
		EnableRagdoll();
	}
}

void ABaseMonster::SetMonsterState(EMonsterState NewState)
{
	MonsterState = NewState;
	//블랙보드값 업데이트
	if (auto aiCon = Cast<AAIController>(GetController()))
	{
		if (auto bbComp = UAIBlueprintHelperLibrary::GetBlackboard(this))
		{
			bbComp->SetValueAsEnum("MonsterState", static_cast<uint8>(MonsterState));
		}
	}
}

bool ABaseMonster::ShouldForceCombatState()
{
	if (MonsterDataAsset)
	{
		return MonsterDataAsset->bForceCombatStateWhenPerceptedTarget;
	}
	return false;
}

void ABaseMonster::IncreaseStunIntensity(const FAttributeEffect& Effect,
                                         UAbilityEffectAdditionalInformation* AdditionalInformation)
{
	if (!MonsterDataAsset->bUseStun)
	{
		return;
	}

	if (auto damagedBy = Cast<ABaseCharacter>(AdditionalInformation->HitBy))
	{
		//경직치를 누적시킵니다. 피해를 준 대상의 의지에 따라 추가증가시킵니다.
		StunIntensity += Effect.ApplyValue * 0.1f * (damagedBy->GetAttributeComponent()->GetWillpower());
		if (StunIntensity >= MonsterDataAsset->MaxStunIntensity)
		{
			StunIntensity = 0;
			if (auto aiCon = GetController<AMonsterAIController>())
			{
				UE_LOGFMT(LogAICon, Warning, "{0}가 스턴됨!!!", GetActorNameOrLabel());
				CharacterState = ECharacterState::STUN;

				FTimerHandle aiResumeTimerHandle;
				FTimerDelegate aiResumeTimerDel = FTimerDelegate::CreateLambda([this]()
				{
					CharacterState = ECharacterState::NORMAL;
				});

				const auto& time = GetMesh()->GetAnimInstance()->Montage_Play(
					MonsterDataAsset->StunMontage, 1.f, EMontagePlayReturnType::Duration);
				GetWorldTimerManager().SetTimer(aiResumeTimerHandle, aiResumeTimerDel, time, false);
			}
		}
	}
}


void ABaseMonster::OnRemoveAttributeEffectAdditionalInformationEvent_Implementation(const FAttributeEffect& Effect,
	UAbilityEffectAdditionalInformation*
	AdditionalInformation, float DeltaTime)
{
	if (AdditionalInformation != nullptr)
	{
		UE_LOGFMT(LogMonster, Log, "{0}이(가) {1}에게 입은 피해량 : {2}", GetActorNameOrLabel(),
		          AdditionalInformation->HitBy->GetActorNameOrLabel(), Effect.ApplyValue * DeltaTime);

		if (Effect.Attribute == EAttributeType::HP)
		{
			//사망에 이르렀니?
			if (AttributeComponent->GetHP() <= 0)
			{
				if (IsDead() == false)
				{
					/*CharacterState = ECharacterState::DEAD;*/
					OnDead.Broadcast(this, AdditionalInformation->HitBy.Get());
				}
				return;
			}


			if (auto aiCon = GetController<AMonsterAIController>())
			{
				UAISense_Damage::ReportDamageEvent(this, this, AdditionalInformation->HitBy.Get(), Effect.ApplyValue,
				                                   GetActorLocation(), AdditionalInformation->Hit.Location);
			}

			IncreaseStunIntensity(Effect, AdditionalInformation);
			TriggerHitAnimation(AdditionalInformation);
		}
	}
}

void ABaseMonster::SetRandomRotationYaw()
{
	AddActorLocalRotation(FRotator(0, FMath::RandRange(-360, 360), 0));
}

void ABaseMonster::TriggerHitAnimation_Implementation(UAbilityEffectAdditionalInformation* AdditionalInformation)
{
	Super::TriggerHitAnimation_Implementation(AdditionalInformation);

	if (IsMighty() == false)
	{
		if (auto aicon = Cast<AMonsterAIController>(GetController()))
		{
			UE_LOGFMT(LogTemp, Warning, "트리거 히트 애니메이션 : {0}", GetActorNameOrLabel());
			if (aicon->IsBehaviorTreeRunning())
			{
				Cast<UBehaviorTreeComponent>(aicon->GetBrainComponent())->StopTree();
			}
		}
	}
}

void ABaseMonster::PlayMusic(USoundBase* Music)
{
	if (UKismetSystemLibrary::DoesImplementInterface(this, UBossMonsterInterface::StaticClass()))
	{
		if (MusicComponent == nullptr)
		{
			MusicComponent = NewObject<UAudioComponent>(this);
			MusicComponent->RegisterComponent();
			MusicComponent->SetUISound(true);
			MusicComponent->SetSound(Music);
			MusicComponent->Play();
		}
	}
}

void ABaseMonster::StopMusic(float AdjustVolumeDuration)
{
	if (MusicComponent != nullptr)
	{
		MusicComponent->AdjustVolume(AdjustVolumeDuration, 0.f);

		const FTimerDelegate destroyDel = FTimerDelegate::CreateUObject(
			MusicComponent, &UAudioComponent::DestroyComponent, false);
		FTimerHandle destroyTimerHandle;
		GetWorldTimerManager().SetTimer(destroyTimerHandle, destroyDel, AdjustVolumeDuration, false);
	}
}

FString ABaseMonster::GetSafeName()
{
	return GetNameSafe(this);
}
