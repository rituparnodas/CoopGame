// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "STrackerBot.generated.h"

UCLASS()
class COOPGAME_API ASTrackerBot : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ASTrackerBot();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components")
	class USphereComponent* SphereComp;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components")
	class UStaticMeshComponent* MeshComp;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components")
	class USHealthComponent* HealthComp;

	FVector GetNextPathPoint();

	// Next Point In Navigation Path
	FVector NextPathPoint;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
	float MovementForce;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
	bool bUseVelocityChange;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
	float RequiredDistanceToTarget;

	UFUNCTION()
		void HandleTakeDamage(USHealthComponent* HealthComponent, float Health, float HealthDelta,
			const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

	FTimerHandle TimerHandle_RefreshPath;

	void RefreshPath();

	// Dynamic Material To Pulse On Damage
	UMaterialInstanceDynamic* MatInst;

	//======Destruct Setting======
	void SelfDestruct();

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
	class UParticleSystem* ExplosionEffect;

	bool bExploded;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
	float ExplosionRadius;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
	float ExplosionDamage;

	FTimerHandle TimerHandle_SelfDamage;

	void DamageSelf();

	bool bStartedSelfDestruction;

	//======Sound Effect======
	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
	class USoundCue* SelfDestructSound;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
	class USoundCue* ExplodeSound;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
	float SelfDamageInterval;

	int32 PowerLevel;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void OnCheckNearbyBots();

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
};