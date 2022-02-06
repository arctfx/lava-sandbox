
/* * * * * * * * * * * * * Author's note * * * * * * * * * * * *\
*   _       _   _       _   _       _   _       _     _ _ _ _   *
*  |_|     |_| |_|     |_| |_|_   _|_| |_|     |_|  _|_|_|_|_|  *
*  |_|_ _ _|_| |_|     |_| |_|_|_|_|_| |_|     |_| |_|_ _ _     *
*  |_|_|_|_|_| |_|     |_| |_| |_| |_| |_|     |_|   |_|_|_|_   *
*  |_|     |_| |_|_ _ _|_| |_|     |_| |_|_ _ _|_|  _ _ _ _|_|  *
*  |_|     |_|   |_|_|_|   |_|     |_|   |_|_|_|   |_|_|_|_|    *
*                                                               *
*                     http://www.humus.name                     *
*                                                                *
* This file is a part of the work done by Humus. You are free to   *
* use the code in any way you like, modified, unmodified or copied   *
* into your own work. However, I expect you to respect these points:  *
*  - If you use this file and its contents unmodified, or use a major *
*    part of this file, please credit the author and leave this note. *
*  - For use in anything commercial, please request my approval.     *
*  - Share your work and ideas too as much as you can.             *
*                                                                *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#pragma once

#include "../Framework4/Demo/DemoApp.h"
#include "MarchingCubes.pipeline.h"

class App : public DemoApp
{
public:
	App();

	void ResetCamera() override;

	void CreateRenderSetups() override;
	void DestroyRenderSetups() override;
	bool CreatePipelines() override;
	void DestroyPipelines() override;

	void OnDropDownChanged(DropDownList* list) override;

	bool Init() override;
	void Exit() override;

	void DrawFrame(Context context, uint buffer_index) override;

private:

	// Main rendertarget. m_ColorBuffer is only used if MSAA is enabled.
	Texture m_ColorBuffer;
	Texture m_DepthBuffer;

	RenderPass m_RenderPass;
	RenderSetup m_RenderSetup[BUFFER_FRAMES];

	Buffer m_Constants;

	// Resources for Marching Cubes
	RootSignature m_MarchingCubesRoot;
	ResourceTable m_MarchingCubesResourceTable;
	Buffer m_MarchingCubesLookup;

	// Mesh shader based implementation
	Pipeline m_MarchinCubes;
	Pipeline m_MarchinCubesPrepass;
	// Compute shader based implementation
	Pipeline m_InitCounters;
	Pipeline m_TaskShaderCompute;
	Pipeline m_TaskShaderComputePrepass;
	Pipeline m_AdjustMeshletCount;
	Pipeline m_MeshShaderCompute;
	Pipeline m_VS_PS;
	VertexSetup m_VertexSetup;
	Buffer m_MeshletData;
	Buffer m_Counters;
	Buffer m_VertexBuffer;
	Buffer m_IndexBuffer;
	// Prepass implementation
	Pipeline m_EvaluateField;
	Texture m_Field;

	// Resources for the skybox. Skybox texture is also used in Marching Cubes pass.
	RootSignature m_SkyRoot;
	Pipeline m_SkyPipeline;
	Texture m_Skybox;
	ResourceTable m_SkyboxResources;

	// Has a single sampler state used for both passes
	SamplerTable m_Samplers;

	// Metaball data
	struct MetaBall
	{
		float3 pos;
		float3 dir;
		float radius;
	};
	MetaBall m_Balls[NMarchingCubes::MAX_BALL_COUNT];

	// GUI and settings
	DropDownList* m_Techniques;
	DropDownList* m_BallCountList;
	DropDownList* m_GridSize;
	CheckBox* m_Prepass;
	Slider* m_BallSize;
	Slider* m_AnimationSpeed;

	uint m_BallCount = NMarchingCubes::DEFAULT_BALL_COUNT;
	uint m_Shift = NMarchingCubes::DEFAULT_SHIFT;
};
