// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/STrackerBot.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationPath.h"
#include "GameFramework/Character.h"
#include "DrawDebugHelpers.h"
#include "Particles/ParticleSystem.h"
#include "SHealthComponent.h"
#include "SCharacter.h"
#include "TimerManager.h"
#include "Sound/SoundCue.h"

ASTrackerBot::ASTrackerBot()
{
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(FName("MeshComp"));
	RootComponent = MeshComp;
	MeshComp->SetCanEverAffectNavigation(false);
	MeshComp->SetSimulatePhysics(true);

	SphereComp = CreateDefaultSubobject<USphereComponent>(FName("SphereComp"));
	SphereComp->SetSphereRadius(200.f);
	SphereComp->SetupAttachment(RootComponent);
	SphereComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereComp->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	SphereComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn,ECollisionResponse::ECR_Overlap);

	HealthComp = CreateDefaultSubobject<USHealthComponent>(FName("HealthComp"));

	bUseVelocityChange = false;
	MovementForce = 1000.f;
	RequiredDistanceToTarget = 100.f;

	bExploded = false;
	ExplosionRadius = 200.f;
	ExplosionDamage = 40.f;

	SelfDamageInterval = 0.25f;
}

void ASTrackerBot::BeginPlay()
{
	Super::BeginPlay();

	if (GetLocalRole() == ROLE_Authority)
	{
		// Find Initial Move To
		NextPathPoint = GetNextPathPoint();
	}

	HealthComp->OnHealthChanged.AddDynamic(this, &ASTrackerBot::HandleTakeDamage);
}

FVector ASTrackerBot::GetNextPathPoint()
{
	// Hack, To Get Player Location
	ACharacter* PlayerPawn = UGameplayStatics::GetPlayerCharacter(this, 0);

	UNavigationPath* NavPath =  UNavigationSystemV1::FindPathToActorSynchronously(this, GetActorLocation(), PlayerPawn);

	if (NavPath->PathPoints.Num() > 1)
	{
		return NavPath->PathPoints[1]; // Return Next Point In The Path
	}
	else
	{
		return GetActorLocation(); // Failed To Find Path
	}
}

void ASTrackerBot::SelfDestruct()
{
	if (bExploded) return;

	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());
	
	TArray<AActor*> IgnoredActors;
	IgnoredActors.Add(this);

	UGameplayStatics::ApplyRadialDamage(
		this, 
		ExplosionDamage, 
		GetActorLocation(), 
		ExplosionRadius,
		nullptr, 
		IgnoredActors, 
		this, 
		GetInstigatorController(), 
		true);

	DrawDebugSphere(GetWorld(), GetActorLocation(), ExplosionRadius, 20, FColor::Purple, false, 9.f, 5.f);

	Destroy();
	bExploded = true;

	UGameplayStatics::PlaySoundAtLocation(this, ExplodeSound, GetActorLocation());
}

void ASTrackerBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetLocalRole() == ROLE_Authority)
	{
		float DistanceToTarget = (GetActorLocation() - NextPathPoint).Size();
		if (DistanceToTarget <= RequiredDistanceToTarget)
		{
			NextPathPoint = GetNextPathPoint();

			DrawDebugString(GetWorld(), GetActorLocation(), "Target Reached");
		}
		else
		{
			//Keep Moving Towards Next Target
			FVector ForceDirection = NextPathPoint - GetActorLocation();
			ForceDirection.Normalize();
			ForceDirection *= MovementForce;

			MeshComp->AddForce(ForceDirection, NAME_None, bUseVelocityChange);

			DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation() + ForceDirection, 32, FColor::Blue, false, 0.f, 0, 1.f);
		}
		DrawDebugSphere(GetWorld(), NextPathPoint, 20, 12, FColor::Blue, false, 0.f, 1.f);
	}
}

void ASTrackerBot::HandleTakeDamage(USHealthComponent* HealthComponent, float Health, float HealthDelta,
	const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (MatInst == nullptr)
	{
		MatInst = MeshComp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, MeshComp->GetMaterial(0));
	}

	if (MatInst) 
	{
		MatInst->SetScalarParameterValue("LastTimeDamageTaken", GetWorld()->TimeSeconds);
	}

	if (Health <= 0)
	{
		SelfDestruct();
	}

	UE_LOG(LogTemp, Warning, TEXT("Health : %s of %s"), *FString::SanitizeFloat(Health), *GetName())
}

void ASTrackerBot::NotifyActorBeginOverlap(AActor* OtherActor)
{
	ASCharacter* PlayerPawn = Cast<ASCharacter>(OtherActor);
	if (PlayerPawn && !bStartedSelfDestruction)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_SelfDamage, this, &ASTrackerBot::DamageSelf, SelfDamageInterval, true, 0.f);

		bStartedSelfDestruction = true;

		UGameplayStatics::SpawnSoundAttached(SelfDestructSound, RootComponent);
	}
}

void ASTrackerBot::DamageSelf()
{
	UGameplayStatics::ApplyDamage(this, 20, GetInstigatorController(), this, nullptr);
}