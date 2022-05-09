// Copyright 2016-2020 Cadic AB. All Rights Reserved.
// @Author	Fredrik Lindh [Temaran] (temaran@gmail.com) {https://github.com/Temaran}
///////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "CoreMinimal.h"

#include "Runtime/Core/Public/PixelFormat.h"
#include "Components/SceneCaptureComponent2D.h"
#include "GameFramework/Character.h"
#include "ShaderUsageDemoCharacter.generated.h"

class UInputComponent;

UCLASS()
class AShaderUsageDemoCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	class USkeletalMeshComponent* Mesh1P;

	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	class USkeletalMeshComponent* FP_Gun;

	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	class USceneComponent* FP_MuzzleLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FirstPersonCameraComponent;	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	class USoundBase* FireSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	class UAnimMontage* FireAnimation;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	float BaseTurnRate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	float BaseLookUpRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	FVector GunOffset;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	class USceneCaptureComponent2D* capture_2D_depth_;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	class USceneCaptureComponent2D* capture_2D_segmentation_;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	class USceneCaptureComponent2D* capture_2D_intensity_;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	class UTextureRenderTarget2D* render_target_2D_depth_;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	class UTextureRenderTarget2D* render_target_2D_segmentation_;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	class UTextureRenderTarget2D* render_target_2D_intensity_;

	TArray<FVector4> outputBufferXYZI;
	TArray<FVector4> outputBufferSemantics;


	/************************************************************************/
	/* Plugin Shader Demo variables!                                        */
	/************************************************************************/
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ShaderDemo)
	FColor StartColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ShaderDemo)
	float ComputeShaderSimulationSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ShaderDemo)
	UMaterialInterface* MaterialToApplyToClickedObject;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ShaderDemo)
	class UTextureRenderTarget2D* RenderTarget;

	
public:
	AShaderUsageDemoCharacter();
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	float EndColorBuildup;
	float EndColorBuildupDirection;
	float ComputeShaderBlend;
	float TotalTimeSecs;

	void OnFire();
	void TurnAtRate(float Rate);
	void LookUpAtRate(float Rate);

	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
};
