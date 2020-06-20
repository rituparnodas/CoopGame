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

	HealthComp->OnHealthChanged.AddDynamic(this, &ASTrackerBot::HandleTakeDamage); // OnHealthChanged Is OnRep_HealthChanged() 
																				   //So This Will Work in Client And Server

	FTimerHandle TimerHandle_CheckPowerLevel;
	GetWorldTimerManager().SetTimer(TimerHandle_CheckPowerLevel, this, &ASTrackerBot::OnCheckNearbyBots, 1.f, true);
}

FVector ASTrackerBot::GetNextPathPoint()
{
	// Hack, To Get Player Location
	ACharacter* PlayerPawn = UGameplayStatics::GetPlayerCharacter(this, 0);

	UNavigationPath* NavPath =  UNavigationSystemV1::FindPathToActorSynchronously(this, GetActorLocation(), PlayerPawn);
	
	if (NavPath && NavPath->PathPoints.Num() > 1)
	{
		return NavPath->PathPoints[1]; // Return Next Point In The Path
	}
	else
	{
		return GetActorLocation(); // Failed To Find Path
	}
}

// This Is Working For Both Server And Client
void ASTrackerBot::SelfDestruct()
{
	if (bExploded) return;
	bExploded = true;

	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());

	UGameplayStatics::PlaySoundAtLocation(this, ExplodeSound, GetActorLocation());

	MeshComp->SetVisibility(false, true);
	MeshComp->SetSimulatePhysics(false);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (GetLocalRole() == ROLE_Authority)
	{
		TArray<AActor*> IgnoredActors;
		IgnoredActors.Add(this);

		// Increase damage Based On The Power Level (Challenge Code)
		float ActualDamage = ExplosionDamage + (ExplosionDamage * PowerLevel);

		// This Working On Client Because Here is ["this"] inside Code And Which Is Replicated
		UGameplayStatics::ApplyRadialDamage(
			this,
			ActualDamage,
			GetActorLocation(),
			ExplosionRadius,
			nullptr,
			IgnoredActors,
			this,
			GetInstigatorController(),
			true);

		SetLifeSpan(2.f);
		//DrawDebugSphere(GetWorld(), GetActorLocation(), ExplosionRadius, 20, FColor::Purple, false, 1.f, 1.f);
	}
}

void ASTrackerBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetLocalRole() == ROLE_Authority && !bExploded)
	{
		float DistanceToTarget = (GetActorLocation() - NextPathPoint).Size();
		if (DistanceToTarget <= RequiredDistanceToTarget)
		{
			NextPathPoint = GetNextPathPoint();

			//DrawDebugString(GetWorld(), GetActorLocation(), "Target Reached");
		}
		else
		{
			//Keep Moving Towards Next Target
			FVector ForceDirection = NextPathPoint - GetActorLocation();
			ForceDirection.Normalize();
			ForceDirection *= MovementForce;

			MeshComp->AddForce(ForceDirection, NAME_None, bUseVelocityChange);

			//DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation() + ForceDirection, 32, FColor::Blue, false, 0.f, 0, 1.f);
		}
		//DrawDebugSphere(GetWorld(), NextPathPoint, 20, 12, FColor::Blue, false, 0.f, 1.f);
	}
}

void ASTrackerBot::OnCheckNearbyBots()
{
	// distance To Check Nearby Bots
	const float Radius = 600;

	// create Temporary Collision Shape For Overlaps
	FCollisionShape CollShape;
	CollShape.SetSphere(Radius);

	// only Find Pawns (Player And AI Bots)
	FCollisionObjectQueryParams QueryParams;

	// Our TrackerBot's  Mesh Comp is Set To Physics Body In Blueprint(Default)
	QueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	QueryParams.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, QueryParams, CollShape);

	//DrawDebugSphere(GetWorld(), GetActorLocation(), Radius, 12, FColor::Red, false, 2.f);

	int32 NrOfBots = 0;

	// Loop Over the Result Using A Range Based For Loop
	for (FOverlapResult Result : Overlaps)
	{
		ASTrackerBot* Bot = Cast<ASTrackerBot>(Result.GetActor());

		if (Bot && Bot != this)
		{
			NrOfBots++;
		}
	}

	const int32 MaxPowerLevel = 4;

	// Clamp Between Min = 0 And Max = 4
	PowerLevel = FMath::Clamp(NrOfBots, 0, MaxPowerLevel);
	
	// Update The Material Color
	if (MatInst == nullptr)
	{
		MatInst = MeshComp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, MeshComp->GetMaterial(0));
	}
	if (MatInst)
	{
		float Alpha = PowerLevel / (float)MaxPowerLevel;

		MatInst->SetScalarParameterValue("PowerLevelAlpha", Alpha);
	}
	DrawDebugString(GetWorld(), FVector(0,0,0), FString::FromInt(PowerLevel),this, FColor::White, 1.f, true);
}

// This Is Working For Both Server And Client
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
	Super::NotifyActorBeginOverlap(OtherActor);

	ASCharacter* PlayerPawn = Cast<ASCharacter>(OtherActor);
	if (PlayerPawn && !bStartedSelfDestruction)
	{
		if (GetLocalRole() == ROLE_Authority && !bExploded)
		{
			GetWorldTimerManager().SetTimer(TimerHandle_SelfDamage, this, &ASTrackerBot::DamageSelf, SelfDamageInterval, true, 0.f);
		}

		bStartedSelfDestruction = true;

		UGameplayStatics::SpawnSoundAttached(SelfDestructSound, RootComponent);
	}
}

void ASTrackerBot::DamageSelf()
{
	UGameplayStatics::ApplyDamage(this, 20, GetInstigatorController(), this, nullptr);
}