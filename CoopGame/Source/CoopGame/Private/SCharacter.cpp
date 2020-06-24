// Fill out your copyright notice in the Description page of Project Settings.


#include "SCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "SWeapon.h"
#include "Components/CapsuleComponent.h"
#include "CoopGame/CoopGame.h"
#include "SHealthComponent.h"
#include "Engine/EngineTypes.h"
#include "Net/UnrealNetwork.h"

ASCharacter::ASCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(FName("SpringArmComp"));
	SpringArmComp->SetupAttachment(RootComponent);
	SpringArmComp->bUsePawnControlRotation = true;

	TPPCameraComp = CreateDefaultSubobject<UCameraComponent>(FName("TPPCameraComp"));
	TPPCameraComp->SetupAttachment(SpringArmComp);

	GetMovementComponent()->GetNavAgentPropertiesRef().bCanCrouch = true;

	FPPCameraComp = CreateDefaultSubobject<UCameraComponent>(FName("FPPCameraComp"));
	HeadSocket = "head";
	FPPCameraComp->AttachTo(GetMesh(), HeadSocket);
	FPPCameraComp->bUsePawnControlRotation = true;

	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);

	HealthComp = CreateDefaultSubobject<USHealthComponent>(FName("HealthComp"));

	ZoomedFOV = 65.f;
	ZoomInterpSpeed = 16.f;

	WeaponAttachSocket = "WeaponSocket";

	bADS = false;
}

void ASCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	DefaultFOV = TPPCameraComp->FieldOfView;
	HealthComp->OnHealthChanged.AddDynamic(this, &ASCharacter::OnHealthChanged);

	if (GetLocalRole() == ROLE_Authority)
	{
		// Spawn A Default Weapon
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		CurrentWeapon = GetWorld()->SpawnActor<ASWeapon>(StarterWeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		if (CurrentWeapon)
		{
			CurrentWeapon->SetOwner(this);
			CurrentWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponAttachSocket);
		}
	}
}

void ASCharacter::OnHealthChanged(USHealthComponent* HealthComponent, float Health, float HealthDelta, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (Health <= 0.f && !bDied) // If We Don't Use bDied Then The Function Will Keep Calling This
	{
		// Die
		bDied = true;

		GetMovementComponent()->StopMovementImmediately();
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		DetachFromControllerPendingDestroy(); 
		SetLifeSpan(10.f);
		CurrentWeapon->SetLifeSpan(10.f);
	}
}

void ASCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// (Condition ? A : B) if Condition is True Then A otherwise B
	float TargetFOV = bWantsToZoom ? ZoomedFOV : DefaultFOV;
	float NewFOV = FMath::FInterpTo(TPPCameraComp->FieldOfView, TargetFOV, DeltaTime, ZoomInterpSpeed);

	TPPCameraComp->SetFieldOfView(NewFOV);
}

void ASCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Player Movement Function
	PlayerInputComponent->BindAxis("MoveForward", this, &ASCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASCharacter::MoveRight);

	// These Are Built-in Function
	PlayerInputComponent->BindAxis("LookUp", this, &ASCharacter::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Turn", this, &ASCharacter::AddControllerYawInput);

	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ASCharacter::BeginCrouch);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &ASCharacter::EndCrouch);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// ADS
	PlayerInputComponent->BindAction("ADS", IE_Pressed, this, &ASCharacter::BeginZoom);
	PlayerInputComponent->BindAction("ADS", IE_Released, this, &ASCharacter::EndZoom);

	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ASCharacter::StartFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ASCharacter::StopFire);
}

void ASCharacter::MoveForward(float Value)
{
	AddMovementInput(GetActorForwardVector() * Value);
}

void ASCharacter::MoveRight(float Value)
{
	AddMovementInput(GetActorRightVector() * Value);
}

void ASCharacter::BeginCrouch()
{
	Crouch();
}

void ASCharacter::EndCrouch()
{
	UnCrouch();
}

FVector ASCharacter::GetPawnViewLocation() const
{
	// We Don't Need Base Implementation So We Don't Need Super
	if (TPPCameraComp)
	{
		return TPPCameraComp->GetComponentLocation();
	}
	else
	{
		return Super::GetPawnViewLocation(); // If SomeHow Fails Then Return The Original State
	}
}

// Calling It From Blueprint
void ASCharacter::SwapCamera()
{
	Select = TPPCameraComp;
	TPPCameraComp = FPPCameraComp;
	FPPCameraComp = Select;
}

void ASCharacter::BeginZoom()
{
	bWantsToZoom = true;

	if (GetLocalRole() < ROLE_Authority )
	{
		ServerADSOn();
		
	}

	bADS = true;
	
}

void ASCharacter::EndZoom()
{
	bWantsToZoom = false;

	if (GetLocalRole() < ROLE_Authority)
	{
		ServerADSOff();
		
	}

	bADS = false;
}

void ASCharacter::ServerADSOn_Implementation()
{
	BeginZoom();
}

void ASCharacter::ServerADSOff_Implementation()
{
	EndZoom();
}

bool ASCharacter::ServerADSOn_Validate() { return true; }
bool ASCharacter::ServerADSOff_Validate() { return true; }

void ASCharacter::StartFire()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StartFire();
	}
}

void ASCharacter::StopFire()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StopFire();
	}
}

void ASCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCharacter, CurrentWeapon);
	DOREPLIFETIME(ASCharacter, bDied);
	DOREPLIFETIME(ASCharacter, bADS);
}