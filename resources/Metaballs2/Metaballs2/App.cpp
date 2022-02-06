
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

#include "App.h"
#include "MarchingCubesLookup.h"
#include "Sky.pipeline.h"

App g_App;
DemoApp *app = &g_App;

App::App()
{
	m_DeviceParams.m_Title = L"Metaballs 2";

	// Setup features we need
	m_DeviceParams.m_MeshShaders = Enable;
	m_DeviceParams.m_8bitStorage = Require;

	// A little frame-to-frame upload buffer use.
	m_UploadBufferSize = 64 * 1024;
	// A large chunk of data upload on first frame, mostly the 2048x2048 cubemap with mips
	m_InitialUploadBufferSize = 129 * 1024 * 1024;

	m_CamSpeed = 1.5f;
}

void App::ResetCamera()
{
	DemoApp::ResetCamera();

	m_CamPos = float3(0.5f, 0.5f, -0.2f);
	m_AngleX = 0.0f;
	m_AngleY = 0.0f;
}

void App::OnDropDownChanged(DropDownList* list)
{
	DemoApp::OnDropDownChanged(list);

	if (list == m_BallCountList || list == m_GridSize)
	{
		Finish(m_Device);
		DestroyPipelines();

		m_BallCount = atoi(m_BallCountList->GetSelectedText());
		m_Shift = m_GridSize->GetSelectedItem() + 3;

		CreatePipelines();
	}
}

void App::CreateRenderSetups()
{
	m_RenderPass = CreateRenderPass(m_Device, GetBackBufferFormat(m_Device), IMGFMT_D16, CLEAR_COLOR | CLEAR_DEPTH, m_DeviceParams.m_MSAA);

	const bool use_msaa = (m_DeviceParams.m_MSAA > 1);
	if (use_msaa)
	{
		float4 clear_color(0, 0, 0, 0);

		STextureParams cb_params;
		cb_params.m_Width  = m_DeviceParams.m_Width;
		cb_params.m_Height = m_DeviceParams.m_Height;
		cb_params.m_Format = GetBackBufferFormat(m_Device);
		cb_params.m_MSAASampleCount = m_DeviceParams.m_MSAA;
		cb_params.m_RenderTarget = true;
		cb_params.m_ClearValue = clear_color;
		m_ColorBuffer = CreateTexture(m_Device, cb_params);
	}

	STextureParams db_params;
	db_params.m_Width  = m_DeviceParams.m_Width;
	db_params.m_Height = m_DeviceParams.m_Height;
	db_params.m_Format = IMGFMT_D16;
	db_params.m_MSAASampleCount = m_DeviceParams.m_MSAA;
	db_params.m_DepthTarget = true;
	m_DepthBuffer = CreateTexture(m_Device, db_params);

	// A rendersetup for each buffered frame
	for (uint i = 0; i < BUFFER_FRAMES; i++)
	{
		Texture back_buffer = GetBackBuffer(m_Device, i);
		m_RenderSetup[i] = CreateRenderSetup(m_Device, m_RenderPass, use_msaa? &m_ColorBuffer : &back_buffer, 1, m_DepthBuffer, use_msaa? back_buffer : nullptr);
	}
}

void App::DestroyRenderSetups()
{
	for (uint i = 0; i < BUFFER_FRAMES; i++)
		DestroyRenderSetup(m_Device, m_RenderSetup[i]);

	DestroyTexture(m_Device, m_DepthBuffer);

	DestroyRenderPass(m_Device, m_RenderPass);
}

bool App::CreatePipelines()
{
	// Graphics pipelines
	{
		using namespace NSky;

		SPipelineParams p_params;
		p_params.m_Name = "Sky";
		p_params.m_RootSignature = m_SkyRoot;
		p_params.m_VS = VertexShader;
		p_params.m_PS = PixelShader;
		p_params.m_RenderPass = m_RenderPass;
		p_params.m_DepthTest = true;
		p_params.m_DepthWrite = false;
		p_params.m_DepthFunc = EQUAL;
		if ((m_SkyPipeline = CreatePipeline(m_Device, p_params)) == nullptr)
			return false;
	}

	uint specialization_constants[] = { m_BallCount, m_Shift };

	{
		using namespace NMarchingCubes;

		SPipelineParams p_params;
		p_params.m_Name = "MarchingCubes";
		p_params.m_RootSignature = m_MarchingCubesRoot;
		p_params.m_SpecializationConstants = specialization_constants;
		p_params.m_NumSpecializationConstants = elementsof(specialization_constants);
		p_params.m_MS = MeshShader;
		p_params.m_PS = PixelShader;
		p_params.m_CullMode = CULL_BACK;
		p_params.m_RenderPass = m_RenderPass;
		p_params.m_DepthTest = true;
		p_params.m_DepthWrite = true;

		if (m_DeviceParams.m_MeshShaders)
		{
			p_params.m_TS = TaskShader;
			if ((m_MarchinCubes = CreatePipeline(m_Device, p_params)) == nullptr)
				return false;

			p_params.m_TS = TaskShaderPrepass;
			if ((m_MarchinCubesPrepass = CreatePipeline(m_Device, p_params)) == nullptr)
				return false;
		}

		const AttribDesc vertex_format[] =
		{
			{ 0, VF_FLOAT3, "POSITION" },
			{ 0, VF_FLOAT3, "NORMAL"   },
		};

		p_params.m_Name = "VS_PS";
		p_params.m_Attribs = vertex_format;
		p_params.m_AttribCount = elementsof(vertex_format);
		p_params.m_VS = VertexShader;
		p_params.m_TS = { nullptr, 0 };
		p_params.m_MS = { nullptr, 0 };
		if ((m_VS_PS = CreatePipeline(m_Device, p_params)) == nullptr)
			return false;
	}

	// Compute pipelines
	{
		using namespace NMarchingCubes;

		SComputePipelineParams p_params;
		p_params.m_RootSignature = m_MarchingCubesRoot;
		p_params.m_SpecializationConstants = specialization_constants;
		p_params.m_NumSpecializationConstants = elementsof(specialization_constants);

		p_params.m_Name = "InitCounters";
		p_params.m_CS = InitCounters;
		if ((m_InitCounters = CreatePipeline(m_Device, p_params)) == nullptr)
			return false;

		p_params.m_Name = "TaskCompute";
		p_params.m_CS = TaskShaderCompute;
		if ((m_TaskShaderCompute = CreatePipeline(m_Device, p_params)) == nullptr)
			return false;

		p_params.m_Name = "TaskComputePrepass";
		p_params.m_CS = TaskShaderComputePrepass;
		if ((m_TaskShaderComputePrepass = CreatePipeline(m_Device, p_params)) == nullptr)
			return false;

		p_params.m_Name = "MeshCompute";
		p_params.m_CS = MeshShaderCompute;
		if ((m_MeshShaderCompute = CreatePipeline(m_Device, p_params)) == nullptr)
			return false;

		p_params.m_Name = "AdjustMeshletCount";
		p_params.m_CS = AdjustMeshletCount;
		if ((m_AdjustMeshletCount = CreatePipeline(m_Device, p_params)) == nullptr)
			return false;

		p_params.m_Name = "EvaluateField";
		p_params.m_CS = EvaluateField;
		if ((m_EvaluateField = CreatePipeline(m_Device, p_params)) == nullptr)
			return false;
	}

	return true;
}

void App::DestroyPipelines()
{
	DestroyPipeline(m_Device, m_EvaluateField);

	DestroyPipeline(m_Device, m_VS_PS);
	DestroyPipeline(m_Device, m_MeshShaderCompute);
	DestroyPipeline(m_Device, m_AdjustMeshletCount);
	DestroyPipeline(m_Device, m_TaskShaderComputePrepass);
	DestroyPipeline(m_Device, m_TaskShaderCompute);
	DestroyPipeline(m_Device, m_InitCounters);

	DestroyPipeline(m_Device, m_MarchinCubesPrepass);
	DestroyPipeline(m_Device, m_MarchinCubes);
	DestroyPipeline(m_Device, m_SkyPipeline);
}

bool App::Init()
{
	// Setup up some user options
	int tab = m_ConfigDialog->AddTab("Metaballs");

	m_ConfigDialog->AddWidget(tab, new Label(0, 0, 160, 36, "Technique"));
	m_Techniques = new DropDownList(0, 40, 235, 36);
	int item = m_Techniques->AddItem("Compute shader");
	if (m_DeviceParams.m_MeshShaders)
		item = m_Techniques->AddItem("Mesh shader");
	m_Techniques->SelectItem(item);
	m_ConfigDialog->AddWidget(tab, m_Techniques);

	m_Prepass = new CheckBox(250, 40, 250, 36, "Prepass", true);
	m_ConfigDialog->AddWidget(tab, m_Prepass);

	m_ConfigDialog->AddWidget(tab, new Label(0, 80, 120, 36, "Grid size"));
	m_GridSize = new DropDownList(0, 120, 100, 36);
	m_GridSize->AddItem("8");
	m_GridSize->AddItem("16");
	m_GridSize->AddItem("32");
	m_GridSize->AddItem("64");
	m_GridSize->AddItem("128");
	m_GridSize->AddItem("256");
	m_GridSize->AddItem("512");
	m_GridSize->SelectItem(4);
	m_GridSize->SetListener(this);
	m_ConfigDialog->AddWidget(tab, m_GridSize);

	m_ConfigDialog->AddWidget(tab, new Label(140, 80, 220, 36, "Animation speed"));
	m_AnimationSpeed = new Slider(140, 120, 220, 36, 0.0f, 1.5f, 1.0f);
	m_ConfigDialog->AddWidget(tab, m_AnimationSpeed);

	m_ConfigDialog->AddWidget(tab, new Label(140, 160, 220, 36, "Ball size"));
	m_BallSize = new Slider(140, 200, 220, 36, -1.0f, 1.5f, 0.0f);
	m_ConfigDialog->AddWidget(tab, m_BallSize);

	m_ConfigDialog->AddWidget(tab, new Label(0, 160, 150, 36, "Ball count"));
	m_BallCountList = new DropDownList(0, 200, 100, 36);
	m_BallCountList->AddItem("1");
	m_BallCountList->AddItem("2");
	m_BallCountList->AddItem("4");
	m_BallCountList->AddItem("8");
	m_BallCountList->AddItem("16");
	m_BallCountList->AddItem("32");
	m_BallCountList->AddItem("64");
	m_BallCountList->AddItem("128");
	m_BallCountList->SelectItem(5);
	m_BallCountList->SetListener(this);
	m_ConfigDialog->AddWidget(tab, m_BallCountList);


	CreateRenderSetups();

	Context context = GetMainContext(m_Device);

	{
		// Load Sky resources
		using namespace NSky;

		if ((m_SkyRoot = CreateRootSignature(m_Device, RootSig)) == nullptr)
			return false;

		static const char *files[] =
		{
			"../Textures/CubeMaps/LancellottiChapel/posx.jpg",
			"../Textures/CubeMaps/LancellottiChapel/negx.jpg",
			"../Textures/CubeMaps/LancellottiChapel/posy.jpg",
			"../Textures/CubeMaps/LancellottiChapel/negy.jpg",
			"../Textures/CubeMaps/LancellottiChapel/posz.jpg",
			"../Textures/CubeMaps/LancellottiChapel/negz.jpg",
		};
		if ((m_Skybox = CreateTexture(m_Device, files, elementsof(files), 15)) == nullptr)
			return false;

		if ((m_SkyboxResources = CreateResourceTable(m_Device, m_SkyRoot, Resources, { m_Skybox } )) == nullptr)
			return false;

		if ((m_Samplers = CreateSamplerTable(m_Device, m_SkyRoot, Samplers, {{ FILTER_TRILINEAR, 1, AM_CLAMP, AM_CLAMP, AM_CLAMP }} )) == nullptr)
			return false;
	}

	{
		// Load Marching Cubes resources
		using namespace NMarchingCubes;

		if ((m_MarchingCubesRoot = CreateRootSignature(m_Device, RootSig)) == nullptr)
			return false;

		if ((m_Constants = CreateBuffer(m_Device, SBufferParams(sizeof(SConstants), HEAP_DEFAULT, CONSTANT_BUFFER, "Constants"))) == nullptr)
			return false;

		if ((m_MarchingCubesLookup = CreateBuffer(m_Device, SBufferParams(sizeof(g_MarchingCubesLookup), HEAP_DEFAULT, SHADER_RESOURCE, "MarchingCubesLookup"))) == nullptr)
			return false;
		SetBufferData(context, m_MarchingCubesLookup, g_MarchingCubesLookup, sizeof(g_MarchingCubesLookup));

		if ((m_Counters = CreateBuffer(m_Device, SBufferParams(sizeof(SCounters), HEAP_DEFAULT, SHADER_RESOURCE | INDIRECT_PARAM, "Counters"))) == nullptr)
			return false;

		// Conservative observed max size needed. Theoretical max allocation would be 512x512x512 * sizeof(SComputeMeshletData) = 1.5GB.
		if ((m_MeshletData = CreateBuffer(m_Device, SBufferParams(sizeof(SComputeMeshletData), 2 * 1024 * 1024, HEAP_DEFAULT, SHADER_RESOURCE, "MeshletData"))) == nullptr)
			return false;

		// Conservative observed max size needed. Theoretical max allocation would be 512x512x512 * 12 vertices * sizeof(SVertex) = 18GB.
		if ((m_VertexBuffer = CreateBuffer(m_Device, SBufferParams(sizeof(SVertex), 8 * 1024 * 1024, HEAP_DEFAULT, SHADER_RESOURCE | VERTEX_BUFFER, "VB"))) == nullptr)
			return false;

		// Conservative observed max size needed. Theoretical max allocation would be 512x512x512 * 15 indices * sizeof(uint32) = 7.5GB.
		if ((m_IndexBuffer = CreateBuffer(m_Device, SBufferParams(sizeof(uint), 16 * 1024 * 1024, HEAP_DEFAULT, SHADER_RESOURCE | INDEX_BUFFER, "IB"))) == nullptr)
			return false;

		// Allocated for the largest size needed. 8 bits store the sign bit for the field() function for a 2x2x2 cube.
		STextureParams ft_params;
		ft_params.m_Type   = TEX_3D;
		ft_params.m_Width  = 512 / 2 + 1;
		ft_params.m_Height = 512 / 2 + 1;
		ft_params.m_Depth  = 512 / 2 + 1;
		ft_params.m_Format = IMGFMT_R8UI;
		ft_params.m_UnorderedAccess = true;
		if ((m_Field = CreateTexture(m_Device, ft_params)) == nullptr)
			return false;

		Barrier(context, {{ m_Field, RS_COMMON, RS_UNORDERED_ACCESS }} );

		// Main resource table used for all shaders (except skybox)
		SResourceDesc resources[] = { m_MarchingCubesLookup, m_Skybox, m_Counters, m_MeshletData, m_VertexBuffer, m_IndexBuffer, m_Field };
		if ((m_MarchingCubesResourceTable = CreateResourceTable(m_Device, m_MarchingCubesRoot, Resources, resources)) == nullptr)
			return false;

		if ((m_VertexSetup = CreateVertexSetup(m_Device, m_VertexBuffer, sizeof(SVertex), m_IndexBuffer, sizeof(uint))) == nullptr)
			return false;
	}

	if (!CreatePipelines())
		return false;


	// Initialize metaballs
	srand((uint) __rdtsc());
	for (uint i = 0; i < NMarchingCubes::MAX_BALL_COUNT; i++)
	{
		// Random positions in [0.25, 0.75]
		m_Balls[i].pos.x = float(rand() % RAND_MAX) / float(RAND_MAX - 1) * 0.5f + 0.25f;
		m_Balls[i].pos.y = float(rand() % RAND_MAX) / float(RAND_MAX - 1) * 0.5f + 0.25f;
		m_Balls[i].pos.z = float(rand() % RAND_MAX) / float(RAND_MAX - 1) * 0.5f + 0.25f;

		// Random directions in [-0.6, 0.6]
		m_Balls[i].dir.x = (float(rand() % RAND_MAX) / float(RAND_MAX - 1) - 0.5f) * 1.2f;
		m_Balls[i].dir.y = (float(rand() % RAND_MAX) / float(RAND_MAX - 1) - 0.5f) * 1.2f;
		m_Balls[i].dir.z = (float(rand() % RAND_MAX) / float(RAND_MAX - 1) - 0.5f) * 1.2f;

		// Random radius in [0.02, 0.06]
		m_Balls[i].radius = float(rand() % RAND_MAX) / float(RAND_MAX - 1) * 0.04f + 0.02f;
	}

	return true;
}

void App::Exit()
{
	Finish(m_Device);

	DestroyPipelines();

	DestroySamplerTable(m_Device, m_Samplers);

	DestroyTexture(m_Device, m_Field);

	DestroyVertexSetup(m_Device, m_VertexSetup);
	DestroyBuffer(m_Device, m_MarchingCubesLookup);
	DestroyBuffer(m_Device, m_MeshletData);
	DestroyBuffer(m_Device, m_Counters);
	DestroyBuffer(m_Device, m_VertexBuffer);
	DestroyBuffer(m_Device, m_IndexBuffer);

	DestroyBuffer(m_Device, m_Constants);

	DestroyResourceTable(m_Device, m_MarchingCubesResourceTable);
	DestroyRootSignature(m_Device, m_MarchingCubesRoot);

	DestroyResourceTable(m_Device, m_SkyboxResources);
	DestroyTexture(m_Device, m_Skybox);
	DestroyRootSignature(m_Device, m_SkyRoot);

	DestroyRenderSetups();
}

void App::DrawFrame(Context context, uint buffer_index)
{
	const float near_plane = 0.01f;
	const float far_plane = 40.0f;
	const float ratio = (float) m_DeviceParams.m_Height / (float) m_DeviceParams.m_Width;

	// Setup transforms
	float4x4 proj     = PerspectiveMatrix(       1.5f, ratio, far_plane, near_plane);
	float4x4 proj_inv = PerspectiveMatrixInverse(1.5f, ratio, far_plane, near_plane);

	float4x4 view = mul(mul(RotateX(-m_AngleY), RotateY(-m_AngleX)), translate(-m_CamPos));
	float4x4 view_inv = mul(RotateY(m_AngleX), RotateX(m_AngleY)); // For skybox, without translation

	float4x4 view_proj     = mul(proj, view);
	float4x4 view_proj_inv = mul(view_inv, proj_inv);

	// Update the metaballs
	float animation_speed = m_AnimationSpeed->GetValue();
	animation_speed *= animation_speed;
	const float frame_time = animation_speed * min(m_Timer.GetFrameTime(), 0.1f);
	for (uint i = 0; i < m_BallCount; i++)
	{
		vec3 d = vec3(0.5f, 0.5f, 0.5f) - m_Balls[i].pos;
		m_Balls[i].dir += d * (5.0f * frame_time / (2.0f + dot(d, d)));
		m_Balls[i].pos += m_Balls[i].dir * frame_time;
	}

	// Setup global constants
	SMapBufferParams cb_params(context, m_Constants);
	auto* constants = (NMarchingCubes::SConstants*) MapBuffer(cb_params);
		constants->ViewProj = view_proj;
		constants->CamPos = m_CamPos;
		constants->Padding = 0;
		for (int i = 0; i < NMarchingCubes::MAX_BALL_COUNT; i++)
		{
			float radius = m_Balls[i].radius * exp2f(m_BallSize->GetValue());
			constants->Balls[i] = float4(m_Balls[i].pos, radius * radius);
		}
	UnmapBuffer(cb_params);


	const bool use_mesh_shaders = (m_Techniques->GetSelectedItem() == 1);
	const bool use_prepass = m_Prepass->IsChecked();
	const uint grid_size = (1 << m_Shift);


	if (use_prepass)
	{
		using namespace NMarchingCubes;

		ScopeMarker(context, "Prepass");

		SetRootSignature(context, m_MarchingCubesRoot);
		SetComputeConstantBuffer(context, Constants, m_Constants);
		SetComputeResourceTable(context, Resources, m_MarchingCubesResourceTable);
		SetComputeSamplerTable(context, Samplers, m_Samplers);

		SetPipeline(context, m_EvaluateField);
		Dispatch(context, grid_size / 8 + 1, grid_size / 8 + 1, grid_size / 8 + 1);

		UAVBarrier(context);
	}


	if (!use_mesh_shaders)
	{
		// Compute metaballs using compute shaders
		using namespace NMarchingCubes;

		{
			ScopeMarker(context, "Compute Task");

			// 1st pass - Initialize counters buffer
			SetRootSignature(context, m_MarchingCubesRoot);
			SetComputeConstantBuffer(context, Constants, m_Constants);
			SetComputeResourceTable(context, Resources, m_MarchingCubesResourceTable);
			SetComputeSamplerTable(context, Samplers, m_Samplers);

			SetPipeline(context, m_InitCounters);
			Dispatch(context, 1, 1, 1);

			UAVBarrier(context);

			// 2nd pass - corresponding to the task shader. Works on 4x4x4 chunks.
			SetPipeline(context, use_prepass? m_TaskShaderComputePrepass : m_TaskShaderCompute);
			Dispatch(context, (grid_size / 4) * (grid_size / 4) * (grid_size / 4), 1, 1);

			UAVBarrier(context);

			// 3rd pass - pads the meshlet data if needed and divides the work for the specific workgroup size, 64 by default, which gives two meshlets per workgroup.
			SetPipeline(context, m_AdjustMeshletCount);
			Dispatch(context, 1, 1, 1);

			Barrier(context, {
				{ m_Counters, RS_UNORDERED_ACCESS, RS_INDIRECT_ARGUMENT } });
		}
		
		{
			ScopeMarker(context, "Compute Mesh");

			// 4th pass - corresponding to mesh shader. Works on N meshlets per workgroup. Default workgroup size of 64 processes two meshlets per workgroup.
			SetPipeline(context, m_MeshShaderCompute);
			DispatchIndirect(context, m_Counters, 0);

			Barrier(context, {
				{ m_VertexBuffer, RS_UNORDERED_ACCESS, RS_VERTEX_BUFFER },
				{ m_IndexBuffer,  RS_UNORDERED_ACCESS, RS_INDEX_BUFFER  } });
		}
	}

	BeginRenderPass(context, "Backbuffer", m_RenderPass, m_RenderSetup[buffer_index], float4(0, 0, 0, 0));
	{
		// Draw metaballs
		using namespace NMarchingCubes;
		ScopeMarker(context, "Metaballs");

		SetRootSignature(context, m_MarchingCubesRoot);
		SetGraphicsConstantBuffer(context, Constants, m_Constants);
		SetGraphicsResourceTable(context, Resources, m_MarchingCubesResourceTable);
		SetGraphicsSamplerTable(context, Samplers, m_Samplers);

		if (use_mesh_shaders)
		{
			SetPipeline(context, use_prepass? m_MarchinCubesPrepass : m_MarchinCubes);
			// The task shader works on 4x4x4 chunks
			DrawMeshTask(context, 0, (grid_size / 4) * (grid_size / 4) * (grid_size / 4));
		}
		else
		{
			SetPipeline(context, m_VS_PS);
			SetVertexSetup(context, m_VertexSetup);
			DrawIndexedIndirect(context, m_Counters, offsetof(SCounters, IndexCount));
		}
	}
	{
		// Draw skybox
		using namespace NSky;
		ScopeMarker(context, "Skybox");

		SetRootSignature(context, m_SkyRoot);
		SetGraphicsResourceTable(context, Resources, m_SkyboxResources);
		SetGraphicsSamplerTable(context, Samplers, m_Samplers);

		SConstants* consts = (SConstants*) SetGraphicsConstantBuffer(context, Constants, sizeof(SConstants));
		consts->InvViewProj = view_proj_inv;

		SetPipeline(context, m_SkyPipeline);
		Draw(context, 0, 3);
	}
	EndRenderPass(context, m_RenderSetup[buffer_index]);
}
