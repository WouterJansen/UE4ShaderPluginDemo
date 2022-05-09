// Copyright 2016-2020 Cadic AB. All Rights Reserved.
// @Author	Fredrik Lindh [Temaran] (temaran@gmail.com) {https://github.com/Temaran}
///////////////////////////////////////////////////////////////////////////////////////

#include "ShaderDeclarationDemoModule.h"

#include "ComputeShaderExample.h"

#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "RHI.h"
#include "GlobalShader.h"
#include "RHICommandList.h"
#include "RenderGraphBuilder.h"
#include "RenderTargetPool.h"
#include "Runtime/Core/Public/Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"

IMPLEMENT_MODULE(FShaderDeclarationDemoModule, ShaderDeclarationDemo)

// Declare some GPU stats so we can track them later
DECLARE_GPU_STAT_NAMED(ShaderPlugin_Render, TEXT("ShaderPlugin: Root Render"));
DECLARE_GPU_STAT_NAMED(ShaderPlugin_Compute, TEXT("ShaderPlugin: Render Compute Shader"));

void FShaderDeclarationDemoModule::StartupModule()
{
	OnPostResolvedSceneColorHandle.Reset();
	bCachedParametersValid = false;

	// Maps virtual shader source directory to the plugin's actual shaders directory.
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("TemaranShaderTutorial"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/TutorialShaders"), PluginShaderDir);
}

void FShaderDeclarationDemoModule::ShutdownModule()
{
	EndRendering();
}

void FShaderDeclarationDemoModule::BeginRendering()
{
	if (OnPostResolvedSceneColorHandle.IsValid())
	{
		return;
	}

	bCachedParametersValid = false;

	const FName RendererModuleName("Renderer");
	IRendererModule* RendererModule = FModuleManager::GetModulePtr<IRendererModule>(RendererModuleName);
	if (RendererModule)
	{
		OnPostResolvedSceneColorHandle = RendererModule->GetResolvedSceneColorCallbacks().AddRaw(this, &FShaderDeclarationDemoModule::PostResolveSceneColor_RenderThread);
	}
}

void FShaderDeclarationDemoModule::EndRendering()
{
	if (!OnPostResolvedSceneColorHandle.IsValid())
	{
		return;
	}

	const FName RendererModuleName("Renderer");
	IRendererModule* RendererModule = FModuleManager::GetModulePtr<IRendererModule>(RendererModuleName);
	if (RendererModule)
	{
		RendererModule->GetResolvedSceneColorCallbacks().Remove(OnPostResolvedSceneColorHandle);
	}

	OnPostResolvedSceneColorHandle.Reset();
}

void FShaderDeclarationDemoModule::UpdateParameters(FShaderUsageExampleParameters& DrawParameters)
{
	RenderEveryFrameLock.Lock();
	CachedShaderUsageExampleParameters = DrawParameters;
	bCachedParametersValid = true;
	RenderEveryFrameLock.Unlock();
}

void FShaderDeclarationDemoModule::PostResolveSceneColor_RenderThread(FRHICommandListImmediate& RHICmdList, class FSceneRenderTargets& SceneContext)
{
	if (!bCachedParametersValid)
	{
		return;
	}

	// Depending on your data, you might not have to lock here, just added this code to show how you can do it if you have to.
	RenderEveryFrameLock.Lock();
	FShaderUsageExampleParameters Copy = CachedShaderUsageExampleParameters;
	RenderEveryFrameLock.Unlock();

	Draw_RenderThread(Copy);

	char* lockXYZI = (char*)RHICmdList.LockStructuredBuffer(OutputBufferXYZI_buffer_, 0, sizeof(FVector4) * 32, EResourceLockMode::RLM_ReadOnly);
	const TArray<FVector4>* outputBufferXYZI_temp = (TArray<FVector4>*)RHICmdList.LockStructuredBuffer(OutputBufferXYZI_buffer_, 0, sizeof(FVector4) * 32, EResourceLockMode::RLM_ReadOnly);
	FMemory::Memcpy(Copy.OutputBufferXYZI.GetData(), outputBufferXYZI_temp, sizeof(FVector4) * 32);
	RHICmdList.UnlockStructuredBuffer(OutputBufferXYZI_buffer_);
}

void FShaderDeclarationDemoModule::Draw_RenderThread(const FShaderUsageExampleParameters& DrawParameters)
{
	check(IsInRenderingThread());

	if (!DrawParameters.RenderTargetDepth)
	{
		return;
	}

	if (!DrawParameters.RenderTargetSemantics)
	{
		return;
	}

	if (!DrawParameters.RenderTargetIntensity)
	{
		return;
	}

	FRHICommandListImmediate& RHICmdList = GRHICommandList.GetImmediateCommandList();

	QUICK_SCOPE_CYCLE_COUNTER(STAT_ShaderPlugin_Render); // Used to gather CPU profiling data for the UE4 session frontend
	SCOPED_DRAW_EVENT(RHICmdList, ShaderPlugin_Render); // Used to profile GPU activity and add metadata to be consumed by for example RenderDoc

	OutputBufferXYZI_array_.SetNum(32);
	OutputBufferSemantics_array_.SetNum(32);
	OutputBufferXYZI_resource_.ResourceArray = &OutputBufferXYZI_array_;
	OutputBufferSemantics_resource_.ResourceArray = &OutputBufferSemantics_array_;
	OutputBufferXYZI_buffer_ = RHICreateStructuredBuffer(sizeof(float), sizeof(FVector4) * 32, BUF_ShaderResource | BUF_UnorderedAccess, OutputBufferXYZI_resource_);
	OutputBufferSemantics_buffer_ = RHICreateStructuredBuffer(sizeof(float), sizeof(FVector4) * 32, BUF_ShaderResource | BUF_UnorderedAccess, OutputBufferSemantics_resource_);
	OutputBufferXYZI_UAV_ = RHICreateUnorderedAccessView(OutputBufferXYZI_buffer_, /* bool bUseUAVCounter */ false, /* bool bAppendBuffer */ false);
	OutputBufferSemantics_UAV_ = RHICreateUnorderedAccessView(OutputBufferSemantics_buffer_, /* bool bUseUAVCounter */ false, /* bool bAppendBuffer */ false);

	FComputeShaderExample::RunComputeShader_RenderThread(RHICmdList, DrawParameters, DrawParameters.RenderTargetDepth->GetRenderTargetResource()->TextureRHI, DrawParameters.RenderTargetSemantics->GetRenderTargetResource()->TextureRHI, DrawParameters.RenderTargetIntensity->GetRenderTargetResource()->TextureRHI, OutputBufferXYZI_UAV_, OutputBufferSemantics_UAV_);
}

void FShaderDeclarationDemoModule::Get_Data(TArray<FVector4> outputBufferXYZI, TArray<FVector4> outputBufferSemantics)
{
	FRHICommandListImmediate& RHICmdList = GRHICommandList.GetImmediateCommandList();
	char* lockXYZI = (char*)RHICmdList.LockStructuredBuffer(OutputBufferXYZI_buffer_, 0, sizeof(FVector4) * 32, EResourceLockMode::RLM_ReadOnly);
	const TArray<FVector4>* outputBufferXYZI_temp = (TArray<FVector4>*)RHICmdList.LockStructuredBuffer(OutputBufferXYZI_buffer_, 0, sizeof(FVector4) * 32, EResourceLockMode::RLM_ReadOnly);
	FMemory::Memcpy(outputBufferXYZI.GetData(), outputBufferXYZI_temp, sizeof(FVector4) * 32);
	RHICmdList.UnlockStructuredBuffer(OutputBufferXYZI_buffer_);
}
