// Fill out your copyright notice in the Description page of Project Settings.


#include "SPickupActor.h"
#include "SCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/DecalComponent.h"
#include "SPowerupActor.h"
#include "TimerManager.h"

// Sets default values
ASPickupActor::ASPickupActor()
{
	SphereComp = CreateDefaultSubobject<USphereComponent>(FName("SphereComp"));
	RootComponent = SphereComp;
	SphereComp->SetSphereRadius(75.f);

	DecalComp = CreateDefaultSubobject<UDecalComponent>(FName("DecalComp"));
	DecalComp->SetupAttachment(RootComponent);
	DecalComp->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	DecalComp->DecalSize.Set(64, 75, 75);

	SetReplicates(true);
}

// Called when the game starts or when spawned
void ASPickupActor::BeginPlay()
{
	Super::BeginPlay();

	if (GetLocalRole() == ROLE_Authority)
	{
		Respawn();
	}
}

void ASPickupActor::Respawn()
{
	if (!PowerUpClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("PowerUpClass is nullptr in %s , Please Update Your Blueprint"), *GetName())
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	PowerUpInstance = GetWorld()->SpawnActor<ASPowerupActor>(PowerUpClass, GetTransform(), SpawnParams);
}

void ASPickupActor::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (GetLocalRole() == ROLE_Authority)
	{
		ASCharacter* PlayerPawn = Cast<ASCharacter>(OtherActor);
		if (PlayerPawn && PowerUpInstance)
		{
			PowerUpInstance->ActivatePowerup();
			PowerUpInstance = nullptr;

			GetWorldTimerManager().SetTimer(TimerHandle_RespawnTimer, this, &ASPickupActor::Respawn, CooldownDuration, false);
		}
	}
}