// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "00_Character/BaseCharacter.h"
#include "97_Interface/BoneTransformInterface.h"
#include "97_Interface/LockOnInterface.h"
#include "97_Interface/01_Monster/AIInterface.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "Engine/DataAsset.h"
#include "BaseMonster.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMonster, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFinishSpawnAlly, class UBehaviorTreeComponent*, OwnerComp,
                                             EBTNodeResult::Type, Result);

/**
 * 
 */

UENUM(BlueprintType)
enum class EMonsterState : uint8
{
	// 평화 상태
	Peaceful UMETA(DisplayName = "평화"),
	// 적을 인식하고, 전투로 진입한 상태
	Aggressive UMETA(DisplayName = "전투"),
	//적을 인식했지만 전투는 아닌 상태
	Beware UMETA(DisplayName = "경계"),
	MAX
};

UENUM(BlueprintType)
enum class EMonsterType :uint8
{
	//야수
	BEAST,
	//언데드
	UNDEAD,
	//트롤
	TROLL
};


UCLASS(BlueprintType)
class SOULLIKE_API UMonsterDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	//몬스터를 구분짓는 유일한 태그입니다.
	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	FGameplayTag MonsterTag;
	//몬스터 이름
	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	FName MonsterName;
	//AI에 사용할 행동트리
	UPROPERTY(EditAnywhere)
	class UBehaviorTree* BehaviorTree;
	UPROPERTY(EditAnywhere)
	bool bStartBehaviorTreeImmediately;
	//이 값이 참이면 플레이어가 화톳불에서 휴식할 때, 부활합니다.
	UPROPERTY(EditAnywhere)
	bool bRespawnWhenPlayerRest = true;
	UPROPERTY(EditAnywhere)
	EMonsterType MonsterType;

	//참이면 경직시스템을 사용합니다.
	UPROPERTY(EditAnywhere)
	bool bUseStun = false;
	//이 수치를 초과하면, 누적된 경직수치를 초기화 하고, 피격애니메이션을 강제 플레이합니다. 보스타입 몬스터들에게 경직을 발생시키기 좋습니다.
	UPROPERTY(EditAnywhere, Category="Stun", meta=(EditCondition = "bUseStun"))
	float MaxStunIntensity;
	UPROPERTY(EditAnywhere, Category="Stun", meta=(EditCondition = "bUseStun"))
	UAnimMontage* StunMontage;
	
	//시야 범위
	UPROPERTY(EditAnywhere, Category="Perception")
	float SightRadius = 1200.f;
	//시야범위에 들어와 인지하는 도중 이 거리 밖으로 빠져나가면 시야에서 사라진 것으로 봅니다.
	//덮어 쓸 컨피그
	UPROPERTY(EditAnywhere, Category="Perception")
	float LoseSightRadius = 1500.f;
	//정면에서 이 수치만큼을 시야라고 생각합니다.
	UPROPERTY(EditAnywhere, Category="Perception")
	float PeripheralVisionAngleDegrees = 60.f;
	//시야 퍼셉션에서 기억할 최대 시간입니다
	UPROPERTY(EditAnywhere, Category="Perception")
	float MaxAge = 5.f;
	UPROPERTY(EditAnywhere, Category="Perception")
	bool bForceCombatStateWhenPerceptedTarget = true;
	//레그돌용
	UPROPERTY(EditAnywhere)
	class UPhysicsAsset* RagdollPhysics;

	
};


/* 
 *	모든 몬스터의 기본 클래스
 *	IBossMonsterInterface를 상속받으면 보스몬스터 취급 합니다.
 */
UCLASS()
class SOULLIKE_API ABaseMonster : public ABaseCharacter, public ILockOnInterface,
                                  public IAIInterface
{
	GENERATED_BODY()

	friend class AMonsterAIController;
	/**********************************************기본 상속 함수*********************************************************/
protected:
	ABaseMonster();

	virtual void BeginPlay() override;

	virtual void PostInitializeComponents() override;
	
	UFUNCTION()
	void OnDestroyedEvent(AActor* DestroyedActor);
	/**********************************************컴포넌트*********************************************************/
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	class UWidgetComponent* HealthBarWidgetComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	class UItemDropComponent* ItemDropComponent;
public:
	class UWidgetComponent* GetHealthBarWidgetComponent() const { return HealthBarWidgetComponent; }


protected:
	FTimerHandle HealthBarVisibleTimerHandle;
	
	UFUNCTION()
	void UpdateHealthBar(float Value, float MaxValue);

	/**********************************************락온*********************************************************/
public:
	virtual bool IsLockOnAble_Implementation() override { return bLockOnAble; }
	virtual void SetLockOnAble_Implementation(bool newVal) override { bLockOnAble = newVal; }
	/**********************************************아군 스폰기능*********************************************************/
protected:
	//아군 스폰이 끝난 후 호출하세요.
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnFinishSpawnAlly OnFinishSpawnAllyEvent;

	//몬스터 사망시 n초후에 몬스터를 제거하는 타이머 핸들
	UPROPERTY()
	FTimerHandle DeadTimerHandle;
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, DisplayName= "SpawnAlly")
	void K2_SpawnAlly(int32 SpawnCount);
	virtual void K2_SpawnAlly_Implementation(int32 SpawnCount);
public:
	void SpawnAlly(int32 SpawnCount, const FOnFinishSpawnAlly& OnFinishSpawnAlly);
	void ResetMonster(const FTransform& DefaultTr);

protected:
	//이 몬스터가 락온 가능한 상태면 참입니다.
	UPROPERTY(EditAnywhere)
	bool bLockOnAble = true;

	/**********************************************상태*********************************************************/
protected:
	//적을 인식했는지 저장하는 변수입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,Transient)
	EMonsterState MonsterState;
	//몬스터 기본정보를 저장하는 데이터 에셋입니다.
	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	class UMonsterDataAsset* MonsterDataAsset;
	//이 수치가 DataAsset에 저장된 수치보다 높아지면 이 수치를 0으로 초기화 하고, 강제로 피격 애니메이션을 재생하도록 합니다.
	UPROPERTY(EditAnywhere,Transient)
	float StunIntensity;
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	EMonsterType GetMonsterType() { return MonsterDataAsset->MonsterType; }
public:
	UFUNCTION(BlueprintCallable)
	void SetMonsterState(EMonsterState NewState);
	UFUNCTION(BlueprintCallable,BlueprintPure)
	EMonsterState GetMonsterState(){return MonsterState;}
	UFUNCTION(BlueprintCallable,BlueprintPure)
	bool ShouldForceCombatState();
protected:
	void IncreaseStunIntensity(const FAttributeEffect& Effect,
							   UAbilityEffectAdditionalInformation* AdditionalInformation);
	/**********************************************리스폰*********************************************************/
protected:
	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool IsRespawnWhenPlayerRest() { return MonsterDataAsset->bRespawnWhenPlayerRest; }


	/**********************************************AI*********************************************************/
protected:
	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool IsStartBehaviorTreeImmediately() { return MonsterDataAsset->bStartBehaviorTreeImmediately; }
	/**********************************************기본정보*********************************************************/
public:
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FName GetMonsterName() { return MonsterDataAsset->MonsterName; }
	const FGameplayTag& GetMonsterTag() const { return MonsterDataAsset->MonsterTag; }
	/**********************************************애니메이션*********************************************************/

	
	/**********************************************사망처리*********************************************************/

protected:
	virtual void OnDeadEvent(AActor* Who, AActor* DeadBy) override;
	UFUNCTION()
	void OnDeadBossEvent(AActor* Who, AActor* DeadBy);
public:
	UFUNCTION()
	virtual void OnDeadMontageEndedEvent(UAnimMontage* Montage, bool bInterrupted);

	/**********************************************어빌리티 이팩트*********************************************************/
protected:
	virtual void OnRemoveAttributeEffectAdditionalInformationEvent_Implementation(const FAttributeEffect& Effect,
		UAbilityEffectAdditionalInformation*
		AdditionalInformation,float DeltaTime = 1) override;

	//레벨에서 호출하면 렌덤하게 회전을 틉니다.
	UFUNCTION(BlueprintCallable,CallInEditor)
	void SetRandomRotationYaw();

	virtual void TriggerHitAnimation_Implementation(UAbilityEffectAdditionalInformation* AdditionalInformation) override;
};