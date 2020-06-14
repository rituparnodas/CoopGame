// Fill out your copyright notice in the Description page of Project Settings.


#include "SPickupActor.h"
#include "SCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/DecalComponent.h"

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
}

// Called when the game starts or when spawned
void ASPickupActor::BeginPlay()
{
	Super::BeginPlay();
	
}

void ASPickupActor::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	ASCharacter* PlayerPawn = Cast<ASCharacter>(OtherActor);
	if (PlayerPawn)
	{
		// TODO Grant PwerUp To Player If Available
	}
}