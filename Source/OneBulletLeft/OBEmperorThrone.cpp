#include "OBEmperorThrone.h"

#include "Animation/AnimationAsset.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "OBGameState.h"

#include <initializer_list>

namespace
{
UAnimationAsset* LoadFirstAnimationAsset(std::initializer_list<const TCHAR*> AssetPaths)
{
	for (const TCHAR* AssetPath : AssetPaths)
	{
		if (UAnimationAsset* Animation = LoadObject<UAnimationAsset>(nullptr, AssetPath))
		{
			return Animation;
		}
	}

	return nullptr;
}

bool IsBossAnimFallback(const UAnimationAsset* Animation)
{
	return Animation && Animation->GetName() == TEXT("Boss_Anim");
}
}

AOBEmperorThrone::AOBEmperorThrone()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	ThroneBase = MakeMeshComponent(TEXT("ThroneBase"), SceneRoot);
	ThroneBack = MakeMeshComponent(TEXT("ThroneBack"), SceneRoot);
	ThroneSeat = MakeMeshComponent(TEXT("ThroneSeat"), SceneRoot);
	LeftArmRest = MakeMeshComponent(TEXT("LeftArmRest"), SceneRoot);
	RightArmRest = MakeMeshComponent(TEXT("RightArmRest"), SceneRoot);
	BossMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BossMesh"));
	BossMesh->SetupAttachment(SceneRoot);
	BossMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BossMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);

	ConfigureMeshes();
	ConfigureBossMesh();
}

void AOBEmperorThrone::BeginPlay()
{
	Super::BeginPlay();
	ConfigureBossMesh();
	ConfigureMaterials();

	if (const AOBGameState* OneBulletState = GetWorld() ? GetWorld()->GetGameState<AOBGameState>() : nullptr)
	{
		bWasGameOver = OneBulletState->bGameOver;
		if (bWasGameOver && bAutoClapOnPlayerDeath)
		{
			PlayClap();
		}
	}
	else
	{
		PlayCurrentAnimation();
	}
}

void AOBEmperorThrone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ConfigureBossMesh();
	PlayCurrentAnimation();
}

void AOBEmperorThrone::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bAutoClapOnPlayerDeath)
	{
		if (const AOBGameState* OneBulletState = GetWorld() ? GetWorld()->GetGameState<AOBGameState>() : nullptr)
		{
			if (OneBulletState->bGameOver && !bWasGameOver)
			{
				PlayClap();
			}
			else if (!OneBulletState->bGameOver && bWasGameOver)
			{
				PlayIdle();
			}
			bWasGameOver = OneBulletState->bGameOver;
		}
	}
}

void AOBEmperorThrone::PlayIdle()
{
	AnimationState = EOBEmperorAnimationState::Idle;
	PlayCurrentAnimation();
}

void AOBEmperorThrone::PlayClap()
{
	AnimationState = EOBEmperorAnimationState::Clapping;
	PlayCurrentAnimation();
}

void AOBEmperorThrone::ConfigureMeshes()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	for (UStaticMeshComponent* MeshComponent : {
		ThroneBase.Get(), ThroneBack.Get(), ThroneSeat.Get(), LeftArmRest.Get(), RightArmRest.Get() })
	{
		if (MeshComponent && CubeMesh.Succeeded())
		{
			MeshComponent->SetStaticMesh(CubeMesh.Object);
			MeshComponent->SetCollisionProfileName(TEXT("BlockAll"));
		}
	}

	ThroneBase->SetRelativeLocation(FVector(0.0f, 0.0f, 35.0f));
	ThroneBase->SetRelativeScale3D(FVector(2.2f, 1.75f, 0.7f));
	ThroneSeat->SetRelativeLocation(FVector(0.0f, 0.0f, 115.0f));
	ThroneSeat->SetRelativeScale3D(FVector(1.65f, 1.45f, 0.35f));
	ThroneBack->SetRelativeLocation(FVector(-70.0f, 0.0f, 220.0f));
	ThroneBack->SetRelativeScale3D(FVector(0.35f, 1.75f, 2.75f));
	LeftArmRest->SetRelativeLocation(FVector(10.0f, -95.0f, 135.0f));
	LeftArmRest->SetRelativeScale3D(FVector(1.65f, 0.32f, 0.85f));
	RightArmRest->SetRelativeLocation(FVector(10.0f, 95.0f, 135.0f));
	RightArmRest->SetRelativeScale3D(FVector(1.65f, 0.32f, 0.85f));
}

void AOBEmperorThrone::ConfigureBossMesh()
{
	if (!BossMesh)
	{
		return;
	}

	if (!BossModel)
	{
		BossModel = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Assets/Boss.Boss"));
	}

	if (!SittingAnimation || IsBossAnimFallback(SittingAnimation))
	{
		SittingAnimation = LoadFirstAnimationAsset({
			TEXT("/Game/Assets/Animations/Sitting_Anim.Sitting_Anim"),
			TEXT("/Game/Assets/Sitting_Anim.Sitting_Anim"),
			TEXT("/Game/Assets/Boss_Anim.Sitting_Anim"),
			TEXT("/Game/Assets/Boss_Anim.Boss_Anim")
		});
	}

	if (!SittingClapAnimation || IsBossAnimFallback(SittingClapAnimation))
	{
		SittingClapAnimation = LoadFirstAnimationAsset({
			TEXT("/Game/Assets/Animations/Sitting_Clap_Anim.Sitting_Clap_Anim"),
			TEXT("/Game/Assets/Sitting_Clap_Anim.Sitting_Clap_Anim"),
			TEXT("/Game/Assets/Boss_Anim.Sitting_Clap_Anim")
		});
	}

	if (BossModel)
	{
		BossMesh->SetSkeletalMesh(BossModel);
	}
	BossMesh->SetRelativeTransform(BossRelativeTransform);
	BossMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BossMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode, true);
	BossMesh->bEnableAnimation = true;
}

void AOBEmperorThrone::ConfigureMaterials()
{
	SetMeshColor(ThroneBase, FLinearColor(0.55f, 0.05f, 0.04f));
	SetMeshColor(ThroneSeat, FLinearColor(0.75f, 0.06f, 0.05f));
	SetMeshColor(ThroneBack, FLinearColor(0.48f, 0.03f, 0.04f));
	SetMeshColor(LeftArmRest, FLinearColor(0.94f, 0.66f, 0.18f));
	SetMeshColor(RightArmRest, FLinearColor(0.94f, 0.66f, 0.18f));
}

void AOBEmperorThrone::PlayCurrentAnimation()
{
	if (!BossMesh)
	{
		return;
	}

	UAnimationAsset* Animation = AnimationState == EOBEmperorAnimationState::Clapping
		? SittingClapAnimation.Get()
		: SittingAnimation.Get();
	if (!Animation)
	{
		UE_LOG(LogTemp, Warning, TEXT("Emperor throne has no %s animation assigned."),
			AnimationState == EOBEmperorAnimationState::Clapping ? TEXT("clapping") : TEXT("sitting"));
		return;
	}

	if (!BossMesh->GetSkeletalMeshAsset())
	{
		UE_LOG(LogTemp, Warning, TEXT("Emperor throne has no Boss skeletal mesh assigned."));
		return;
	}

	if (Animation->GetSkeleton() && BossMesh->GetSkeletalMeshAsset()->GetSkeleton() && Animation->GetSkeleton() != BossMesh->GetSkeletalMeshAsset()->GetSkeleton())
	{
		UE_LOG(LogTemp, Warning, TEXT("Emperor throne animation %s uses skeleton %s, but Boss mesh uses %s."),
			*GetNameSafe(Animation),
			*GetNameSafe(Animation->GetSkeleton()),
			*GetNameSafe(BossMesh->GetSkeletalMeshAsset()->GetSkeleton()));
		return;
	}

	BossMesh->bEnableAnimation = true;
	BossMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode, true);
	BossMesh->OverrideAnimationData(Animation, true, true, 0.0f, 1.0f);
	BossMesh->InitAnim(true);
	if (UAnimSingleNodeInstance* SingleNodeInstance = BossMesh->GetSingleNodeInstance())
	{
		SingleNodeInstance->SetAnimationAsset(Animation, true, 1.0f);
		SingleNodeInstance->SetLooping(true);
		SingleNodeInstance->SetPlaying(true);
		SingleNodeInstance->SetPosition(0.0f, false);
	}
	BossMesh->RefreshBoneTransforms();
}

void AOBEmperorThrone::SetMeshColor(UStaticMeshComponent* MeshComponent, const FLinearColor& Color)
{
	if (!MeshComponent)
	{
		return;
	}

	UMaterialInstanceDynamic* Material = MeshComponent->CreateAndSetMaterialInstanceDynamic(0);
	if (Material)
	{
		Material->SetVectorParameterValue(TEXT("Color"), Color);
		Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
	}
}

UStaticMeshComponent* AOBEmperorThrone::MakeMeshComponent(const FName Name, USceneComponent* Parent)
{
	UStaticMeshComponent* MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	MeshComponent->SetupAttachment(Parent);
	MeshComponent->SetMobility(EComponentMobility::Movable);
	return MeshComponent;
}
