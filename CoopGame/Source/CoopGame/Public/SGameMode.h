// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SGameMode.generated.h"

/**
 * 
 */
UCLASS()
class COOPGAME_API ASGameMode : public AGameModeBase
{
	GENERATED_BODY()

protected:

	// Hook For BP to Spawn A Single BOT
	UFUNCTION(BlueprintImplementableEvent, Category = "GameMode")
	void SpawnNewBot();

	// Start Spawning Bots
	void startWave();

	// Stop Spawning Bots
	void EndWave();

	// Set Timer For Next StartWave()
	void PrepareForNextWave();

	void SpawnBotTimerElapsed();

	FTimerHandle TimerHandle_BotSpawner;
	FTimerHandle TimerHandle_NextWaveStart;

	// Bots To Spawn In Current Wave
	int32 NrOfBotsToSpawn;
	int32 WaveCount;

	UPROPERTY(EditDefaultsOnly, Category = "Wave")
	float TimeBetweenWaves;

	void CheckWaveState();

	void CheckAnyPlayerAlive();

	void GameOver();

public:

	ASGameMode();

	virtual void StartPlay() override;

	virtual void Tick(float DeltaSeconds) override;
};
