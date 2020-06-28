// Fill out your copyright notice in the Description page of Project Settings.

#include "SHealthComponent.h"
#include "SGameMode.h"
#include "Net/UnrealNetwork.h"

USHealthComponent::USHealthComponent()
{
	DefaultHealth = 100.f;
	bIsDead = false;

	TeamNum = 255;

	SetIsReplicated(true);
}

void USHealthComponent::Heal(float HealAmount)
{
	if (HealAmount <= 0.f || Health <= 0.f) return;
	Health = FMath::Clamp(Health + HealAmount, 0.f, DefaultHealth);
	UE_LOG(LogTemp, Warning, TEXT("Health Changed : %s , (+ %s)"), *FString::SanitizeFloat(Health), *FString::SanitizeFloat(HealAmount))

	OnHealthChanged.Broadcast(this, Health, HealAmount, nullptr, nullptr, nullptr);
}

void USHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	// Only Hook If We Are The Sever
	if (GetOwnerRole() == ROLE_Authority)
	{
		AActor* MyOwner = GetOwner();
		if (MyOwner)
		{
			MyOwner->OnTakeAnyDamage.AddDynamic(this, &USHealthComponent::HandleTakeAnyDamage);
		}
	}
	Health = DefaultHealth;
}

void USHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage,
	const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	if (Damage <= 0.f || bIsDead) return;

	if (DamageCauser != DamagedActor && IsFriendly(DamagedActor, DamageCauser))
	{
		return;
	}

	// Update Health Clamped
	Health = FMath::Clamp(Health - Damage, 0.f, DefaultHealth);

	UE_LOG(LogTemp, Error, TEXT("Health : %s"), *FString::SanitizeFloat(Health))

	bIsDead = Health <= 0.f;

	OnHealthChanged.Broadcast(this, Health, Damage, DamageType, InstigatedBy, DamageCauser);

	if (bIsDead)
	{
		ASGameMode* GM = Cast<ASGameMode>(GetWorld()->GetAuthGameMode());
		if (GM)
		{
			GM->OnActorKilled.Broadcast(GetOwner(), DamageCauser, InstigatedBy);
		}
	}
}

bool USHealthComponent::IsFriendly(AActor* ActorA, AActor* ActorB)
{
	if (ActorA == nullptr || ActorB == nullptr) return true;

	USHealthComponent* HealthCompA = Cast<USHealthComponent>(ActorA->GetComponentByClass(USHealthComponent::StaticClass()));
	USHealthComponent* HealthCompB = Cast<USHealthComponent>(ActorB->GetComponentByClass(USHealthComponent::StaticClass()));

	if (HealthCompA == nullptr || HealthCompB == nullptr) return true; // Assume Friendly

	return HealthCompA->TeamNum == HealthCompB->TeamNum;
}

// Trigger On Clients
void USHealthComponent::OnRep_Health(float OldHealth)
{
	float Damage = Health - OldHealth;
	OnHealthChanged.Broadcast(this, Health, Damage, nullptr, nullptr, nullptr);
}

void USHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USHealthComponent, Health);
}