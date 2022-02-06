#include "demoContextOGL.h"
#include "shadersGL.h"

DemoContext* CreateDemoContextOGL()
{
	return new DemoContextOGL;
}

bool DemoContextOGL::initialize(const RenderInitOptions& options)
{
	OGL_Renderer::InitRender(options);
	return true;
}

void DemoContextOGL::startFrame(Vec4 colorIn)
{
	OGL_Renderer::StartFrame(colorIn);
}

void DemoContextOGL::endFrame()
{
	OGL_Renderer::EndFrame();
}

void DemoContextOGL::presentFrame(bool fullsync)
{
	OGL_Renderer::PresentFrame(fullsync);
}

void DemoContextOGL::readFrame(int* buffer, int width, int height)
{
	OGL_Renderer::ReadFrame(buffer, width, height);
}

void DemoContextOGL::getViewRay(int x, int y, Vec3& origin, Vec3& dir)
{
	OGL_Renderer::GetViewRay(x, y, origin, dir);
}

void DemoContextOGL::setView(Matrix44 view, Matrix44 projection)
{
	OGL_Renderer::SetView(view, projection);
}

void DemoContextOGL::renderEllipsoids(FluidRenderer* renderer, FluidRenderBuffers* buffers, int n, int offset, float radius, float screenWidth, float screenAspect, float fov, Vec3 lightPos, Vec3 lightTarget, Matrix44 lightTransform, ::ShadowMap* shadowMap, Vec4 color, float blur, float ior, bool debug)
{
	OGL_Renderer::RenderEllipsoids(renderer, buffers, n, offset, radius, screenWidth, screenAspect, fov, lightPos, lightTarget, lightTransform, shadowMap, color, blur, ior, debug);
}

void DemoContextOGL::drawMesh(const Mesh* m, Vec3 color)
{
	OGL_Renderer::DrawMesh(m, color);
}

void DemoContextOGL::drawCloth(const Vec4* positions, const Vec4* normals, const float* uvs, const int* indices, int numTris, int numPositions, int colorIndex, float expand, bool twosided, bool smooth)
{
	OGL_Renderer::DrawCloth(positions, normals, uvs, indices, numTris, numPositions, colorIndex, expand, twosided, smooth);
}

void DemoContextOGL::drawRope(Vec4* positions, int* indices, int numIndices, float radius, int color)
{
	OGL_Renderer::DrawRope(positions, indices, numIndices, radius, color);
}

void DemoContextOGL::drawPlane(const Vec4& p, bool color)
{
	OGL_Renderer::DrawPlane(p, color);
}

void DemoContextOGL::drawPlanes(Vec4* planes, int n, float bias)
{
	OGL_Renderer::DrawPlanes(planes, n, bias);
}

void DemoContextOGL::drawPoints(FluidRenderBuffers* buffers, int n, int offset, float radius, float screenWidth, float screenAspect, float fov, Vec3 lightPos, Vec3 lightTarget, Matrix44 lightTransform, ::ShadowMap* shadowTex, bool showDensity)
{
	OGL_Renderer::DrawPoints(buffers, n, offset, radius, screenWidth, screenAspect, fov, lightPos, lightTarget, lightTransform, shadowTex, showDensity);
}

void DemoContextOGL::graphicsTimerBegin()
{
	OGL_Renderer::GraphicsTimerBegin();
}

void DemoContextOGL::graphicsTimerEnd()
{
	OGL_Renderer::GraphicsTimerEnd();
}

float DemoContextOGL::rendererGetDeviceTimestamps(unsigned long long* begin, unsigned long long* end, unsigned long long* freq)
{
	return OGL_Renderer::RendererGetDeviceTimestamps(begin, end, freq);
}

void DemoContextOGL::bindSolidShader(Vec3 lightPos, Vec3 lightTarget, Matrix44 lightTransform, ::ShadowMap* shadowMap, float bias, Vec4 fogColor)
{
	OGL_Renderer::BindSolidShader(lightPos, lightTarget, lightTransform, shadowMap, bias, fogColor);
}

void DemoContextOGL::unbindSolidShader()
{
	OGL_Renderer::UnbindSolidShader();
}

ShadowMap* DemoContextOGL::shadowCreate()
{
	return OGL_Renderer::ShadowCreate();
}

void DemoContextOGL::shadowDestroy(ShadowMap* map)
{
	OGL_Renderer::ShadowDestroy(map);
}

void DemoContextOGL::shadowBegin(ShadowMap* map)
{
	OGL_Renderer::ShadowBegin(map);
}

void DemoContextOGL::shadowEnd()
{
	OGL_Renderer::ShadowEnd();
}

FluidRenderer* DemoContextOGL::createFluidRenderer(uint32_t width, uint32_t height)
{
	return OGL_Renderer::CreateFluidRenderer(width, height);
}

void DemoContextOGL::destroyFluidRenderer(FluidRenderer* renderer)
{
	OGL_Renderer::DestroyFluidRenderer(renderer);
}

FluidRenderBuffers* DemoContextOGL::createFluidRenderBuffers(int numParticles, bool enableInterop)
{
	return OGL_Renderer::CreateFluidRenderBuffers(numParticles, enableInterop);
}

void DemoContextOGL::updateFluidRenderBuffers(FluidRenderBuffers* buffers, NvFlexSolver* flex, bool anisotropy, bool density)
{
	OGL_Renderer::UpdateFluidRenderBuffers(buffers, flex, anisotropy, density);
}

void DemoContextOGL::updateFluidRenderBuffers(FluidRenderBuffers* buffers, Vec4* particles, float* densities, Vec4* anisotropy1, Vec4* anisotropy2, Vec4* anisotropy3, int numParticles, int* indices, int numIndices)
{
	OGL_Renderer::UpdateFluidRenderBuffers(buffers, particles, densities, anisotropy1, anisotropy2, anisotropy3, numParticles, indices, numIndices);
}

void DemoContextOGL::destroyFluidRenderBuffers(FluidRenderBuffers* buffers)
{
	OGL_Renderer::DestroyFluidRenderBuffers(buffers);
}

GpuMesh* DemoContextOGL::createGpuMesh(const Mesh* m)
{
	return OGL_Renderer::CreateGpuMesh(m);
}

void DemoContextOGL::destroyGpuMesh(GpuMesh* mesh)
{
	OGL_Renderer::DestroyGpuMesh(mesh);
}

void DemoContextOGL::drawGpuMesh(GpuMesh* m, const Matrix44& xform, const Vec3& color)
{
	OGL_Renderer::DrawGpuMesh(m, xform, color);
}

void DemoContextOGL::drawGpuMeshInstances(GpuMesh* m, const Matrix44* xforms, int n, const Vec3& color)
{
	OGL_Renderer::DrawGpuMeshInstances(m, xforms, n, color);
}

DiffuseRenderBuffers* DemoContextOGL::createDiffuseRenderBuffers(int numDiffuseParticles, bool& enableInterop)
{
	return OGL_Renderer::CreateDiffuseRenderBuffers(numDiffuseParticles, enableInterop);
}

void DemoContextOGL::destroyDiffuseRenderBuffers(DiffuseRenderBuffers* buffers)
{
	OGL_Renderer::DestroyDiffuseRenderBuffers(buffers);
}

void DemoContextOGL::updateDiffuseRenderBuffers(DiffuseRenderBuffers* buffers, Vec4* diffusePositions, Vec4* diffuseVelocities, int numDiffuseParticles)
{
	OGL_Renderer::UpdateDiffuseRenderBuffers(buffers, diffusePositions, diffuseVelocities, numDiffuseParticles);
}

void DemoContextOGL::updateDiffuseRenderBuffers(DiffuseRenderBuffers* buffers, NvFlexSolver* solver)
{
	OGL_Renderer::UpdateDiffuseRenderBuffers(buffers, solver);
}

void DemoContextOGL::drawDiffuse(FluidRenderer* render, const DiffuseRenderBuffers* buffers, int n, float radius, float screenWidth, float screenAspect, float fov, Vec4 color, Vec3 lightPos, Vec3 lightTarget, Matrix44 lightTransform, ::ShadowMap* shadowMap, float motionBlur, float inscatter, float outscatter, bool shadowEnabled, bool front)
{
	OGL_Renderer::RenderDiffuse(render, (DiffuseRenderBuffers*)buffers, n, radius, screenWidth, screenAspect, fov, color, lightPos, lightTarget, lightTransform, shadowMap, motionBlur, inscatter, outscatter, shadowEnabled, front);
}

int DemoContextOGL::getNumDiffuseRenderParticles(DiffuseRenderBuffers* buffers)
{
	return OGL_Renderer::GetNumDiffuseRenderParticles(buffers);
}

void DemoContextOGL::beginLines()
{
	OGL_Renderer::BeginLines();
}

void DemoContextOGL::drawLine(const Vec3& p, const Vec3& q, const Vec4& color)
{
	OGL_Renderer::DrawLine(p, q, color);
}

void DemoContextOGL::endLines()
{
	OGL_Renderer::EndLines();
}

void DemoContextOGL::onSizeChanged(int width, int height, bool minimized)
{
	OGL_Renderer::ReshapeRender(width, height, minimized);
}

void DemoContextOGL::startGpuWork()
{
	OGL_Renderer::StartGpuWork();
}

void DemoContextOGL::endGpuWork()
{
	OGL_Renderer::EndGpuWork();
}

void DemoContextOGL::flushGraphicsAndWait()
{
}

void DemoContextOGL::setFillMode(bool wire)
{
	OGL_Renderer::SetFillMode(wire);
}

void DemoContextOGL::setCullMode(bool enabled)
{
	OGL_Renderer::SetCullMode(enabled);
}

void DemoContextOGL::drawImguiGraph()
{
	OGL_Renderer::DrawImguiGraph();
}

void* DemoContextOGL::getGraphicsCommandQueue()
{
	return OGL_Renderer::GetGraphicsCommandQueue();
}

void DemoContextOGL::getRenderDevice(void** device, void** context)
{
	OGL_Renderer::GetRenderDevice(device, context);
}
