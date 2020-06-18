// Fill out your copyright notice in the Description page of Project Settings.

#include "SPowerupActor.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

// Sets default values
ASPowerupActor::ASPowerupActor()
{
	PowerupInterval = 0.f;
	TotalNrOfTicks = 0.f;

	bIsPowerupActive = false;

	SetReplicates(true);
}

void ASPowerupActor::OnTickPowerup()
{
	TickProcessed++;

	OnPowerupTicked(); // BlueprintCallable

	if (TickProcessed >= TotalNrOfTicks)
	{
		OnExpired();

		bIsPowerupActive = false;
		OnRep_PowerupActive(); // Because Its Not Automatically Call On Server So We Need To Manually Control It

		// Delete Timer
		GetWorldTimerManager().ClearTimer(TimerHandle_PowerupTick);
	}
}

void ASPowerupActor::OnRep_PowerupActive()
{
	OnPowerupStateChanged(bIsPowerupActive); // BlueprintCallable (Base Only)
}

void ASPowerupActor::ActivatePowerup(AActor* ActivateFor)
{
	OnActivated(ActivateFor); // BlueprintCallable

	bIsPowerupActive = true;
	OnRep_PowerupActive(); // Because Its Not Automatically Call On Server So We Need To Manually Control It

	if (PowerupInterval > 0.f)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_PowerupTick, this, &ASPowerupActor::OnTickPowerup, PowerupInterval, true);
	}
	else
	{
		OnTickPowerup();
	}
}

void ASPowerupActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASPowerupActor, bIsPowerupActive);
}