#include <cstddef>
#include "app.h"
#include "util.h"

class UVQuad : public GPUGeometry
{
private:
    struct Vertex
    {
        DKVector3 Pos;
        DKVector2 UV;
    };

    DKArray<UVQuad::Vertex> vertices =
    {
        { { 1.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
        { { -1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
        { { -1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f } },
        { { 1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f } }
    };

    DKArray<uint32_t> indices = { 0,1,2,2,3,0 };

public:
    UVQuad() = default;
    
    size_t VerticesCount() const { return vertices.Count(); }
    size_t IndicesCount() const { return indices.Count(); }
    UVQuad::Vertex* VerticesData() { return vertices; }
    uint32_t* IndicesData() { return indices; }

    void InitializeGpuResource(DKCommandQueue* queue)
    {
        DKGraphicsDevice* device = queue->Device();
        uint32_t vertexBufferSize = static_cast<uint32_t>(VerticesCount()) * sizeof(UVQuad::Vertex);
        uint32_t indexBufferSize = IndicesCount() * sizeof(uint32_t);

        vertexBuffer = device->CreateBuffer(vertexBufferSize, DKGpuBuffer::StorageModeShared, DKCpuCacheModeReadWrite);
        memcpy(vertexBuffer->Contents(), VerticesData(), vertexBufferSize);
        vertexBuffer->Flush();

        indexBuffer = device->CreateBuffer(indexBufferSize, DKGpuBuffer::StorageModeShared, DKCpuCacheModeReadWrite);
        memcpy(indexBuffer->Contents(), IndicesData(), indexBufferSize);
        indexBuffer->Flush();

        // setup vertex buffer and attributes
        vertexDesc.attributes = {
            { DKVertexFormat::Float3, offsetof(UVQuad::Vertex, Pos), 0, 0 },
            { DKVertexFormat::Float2, offsetof(UVQuad::Vertex, UV), 0, 1 },
        };
        vertexDesc.layouts = {
            { DKVertexStepRate::Vertex, sizeof(UVQuad::Vertex), 0 },
        };
    }
};

class TextureComputeTarget
{
private:
    DKObject<DKTexture> textureTarget = nullptr;

public:
    TextureComputeTarget() = default;

    DKTexture* ComputeTarget(DKCommandQueue* queue, int w, int h)
    {
        auto device = queue->Device();

        if (textureTarget)
        {
            if (textureTarget->Width() != w ||
                textureTarget->Height() != h)
                textureTarget = nullptr;
        }

        if (textureTarget == nullptr)
        {
            // create depth buffer
            DKTextureDescriptor texDesc = {};
            texDesc.textureType = DKTexture::Type2D;
            texDesc.pixelFormat = DKPixelFormat::BGRA8Unorm;
            texDesc.width = w;
            texDesc.height = h;
            texDesc.depth = 1;
            texDesc.mipmapLevels = 1;
            texDesc.sampleCount = 1;
            texDesc.arrayLength = 1;
            texDesc.usage = DKTexture::UsageShaderRead | DKTexture::UsageShaderWrite | DKTexture::UsageSampled;
            textureTarget = device->CreateTexture(texDesc);
        }

        return textureTarget;
    }
};

class GPUShader
{
private:
    DKObject<DKData> shaderData = nullptr;
    DKObject<DKShaderModule> shaderModule = nullptr;
    DKObject<DKShaderFunction> shaderFunc = nullptr;
public:
    GPUShader(DKData* data) : shaderData(data)
    {
    }

    void InitializeGpuResource(DKCommandQueue* queue)
    {
        if (shaderData)
        {
            DKGraphicsDevice* device = queue->Device();
            DKShader shader(shaderData);
            shaderModule = device->CreateShaderModule(&shader);
            shaderFunc = shaderModule->CreateFunction(shaderModule->FunctionNames().Value(0));
        }
    }

    DKShaderFunction* Function() { return shaderFunc; }
};

class GraphicShaderBindingSet
{
public:
    struct UBO
    {
        DKMatrix4 projectionMatrix;
        DKMatrix4 modelMatrix;
    };
private:
    DKShaderBindingSetLayout descriptorSetLayout;
    DKObject<DKShaderBindingSet> descriptorSetPreCompute;
    DKObject<DKShaderBindingSet> descriptorSetPostCompute;
    DKObject<DKRenderPipelineState> pipelineState;
    DKObject<DKGpuBuffer> uniformBuffer;
    UBO* ubo = nullptr;
public:
    GraphicShaderBindingSet() = default;
    DKShaderBindingSet* PrecomputeDescSet() { return descriptorSetPreCompute; }
    DKShaderBindingSet* PostcomputeDescSet() { return descriptorSetPostCompute; }
    DKRenderPipelineState* GraphicPipelineState() { return pipelineState; }

    void InitializeGpuResource(DKGraphicsDevice* device)
    {
        if (1)
        {
            DKShaderBinding bindings[2] = {
                {
                    0,
                    DKShader::DescriptorTypeUniformBuffer,
                    1,
                    nullptr
                },
                {
                    1,
                    DKShader::DescriptorTypeTextureSampler,
                    1,
                    nullptr
                },
            };
            descriptorSetLayout.bindings.Add(bindings, 2);
        }

        descriptorSetPreCompute = device->CreateShaderBindingSet(descriptorSetLayout);
        descriptorSetPostCompute = device->CreateShaderBindingSet(descriptorSetLayout);

        uniformBuffer = device->CreateBuffer(sizeof(GraphicShaderBindingSet::UBO), DKGpuBuffer::StorageModeShared, DKCpuCacheModeReadWrite);
        
        if (descriptorSetPreCompute)
        {
            if (uniformBuffer)
            {
                ubo = reinterpret_cast<UBO*>(uniformBuffer->Contents());
                ubo->projectionMatrix = DKMatrix4::identity;
                ubo->modelMatrix = DKMatrix4::identity;
                uniformBuffer->Flush();

                descriptorSetPreCompute->SetBuffer(0, uniformBuffer, 0, sizeof(UBO));
            }
        }

        if (descriptorSetPostCompute)
        {
            if (uniformBuffer && ubo)
            {
                descriptorSetPostCompute->SetBuffer(0, uniformBuffer, 0, sizeof(UBO));
            }
        }
    }

    DKGpuBuffer* UniformBuffer() { return uniformBuffer; }
    UBO* UniformBufferO() { return ubo; }
};

class MeshDemo : public SampleApp
{
    DKObject<DKWindow> window;
	DKObject<DKThread> renderThread;
	DKAtomicNumber32 runningRenderThread;

    //Resource
	DKObject<UVQuad> Quad;
    DKObject<DKTexture> textureColorMap;

    DKObject<TextureComputeTarget> ComputeTarget;
    DKObject<DKSamplerState> sampleState = nullptr;;

    DKObject<GraphicShaderBindingSet> graphicShaderBindingSet = nullptr;

public:
	DKObject<DKTexture> LoadTexture2D(DKCommandQueue* queue, DKData* data)
    {
        DKObject<DKImage> image = DKImage::Create(data);
        if (image)
        {
            DKGraphicsDevice* device = queue->Device();
            DKTextureDescriptor texDesc = {};
            texDesc.textureType = DKTexture::Type2D;
            texDesc.pixelFormat = DKPixelFormat::RGBA8Unorm;
            texDesc.width = image->Width();
            texDesc.height = image->Height();
            texDesc.depth = 1;
            texDesc.mipmapLevels = 1;
            texDesc.sampleCount = 1;
            texDesc.arrayLength = 1;
            texDesc.usage = DKTexture::UsageStorage | DKTexture::UsageShaderRead | DKTexture::UsageCopyDestination | DKTexture::UsageSampled;
            DKObject<DKTexture> tex = device->CreateTexture(texDesc);
            if (tex)
            {
                size_t bytesPerPixel = image->BytesPerPixel();
                DKASSERT_DESC(bytesPerPixel == DKPixelFormatBytesPerPixel(texDesc.pixelFormat), "BytesPerPixel mismatch!");

                uint32_t width = image->Width();
                uint32_t height = image->Height();

                size_t bufferLength = bytesPerPixel * width * height;
                DKObject<DKGpuBuffer> stagingBuffer = device->CreateBuffer(bufferLength, DKGpuBuffer::StorageModeShared, DKCpuCacheModeReadWrite);
                
                memcpy(stagingBuffer->Contents(), image->Contents(), bufferLength);
                stagingBuffer->Flush();

                DKObject<DKCommandBuffer> cb = queue->CreateCommandBuffer();
                DKObject<DKCopyCommandEncoder> encoder = cb->CreateCopyCommandEncoder();
                encoder->CopyFromBufferToTexture(stagingBuffer,
                                                 { 0, width, height },
                                                 tex,
                                                 { 0,0, 0,0,0 },
                                                 { width,height,1 });
                encoder->EndEncoding();
                cb->Commit();

                DKLog("Texture created!");
                return tex;
            }
        }
        return nullptr;
    }



    void RenderThread(void)
    {
        // Device and Queue Preperation
        DKObject<DKGraphicsDevice> device = DKGraphicsDevice::SharedInstance();
        DKObject<DKCommandQueue> graphicsQueue = device->CreateCommandQueue(DKCommandQueue::Graphics);
        DKObject<DKCommandQueue> computeQueue = device->CreateCommandQueue(DKCommandQueue::Compute);

        // Geometry Initialzie
        Quad->InitializeGpuResource(graphicsQueue);

        // create shaders
		DKObject<DKData> vertData = resourcePool.LoadResourceData("shaders/ComputeShader/texture.vert.spv");
		DKObject<DKData> fragData = resourcePool.LoadResourceData("shaders/ComputeShader/texture.frag.spv");
        DKObject<DKData> embossData = resourcePool.LoadResourceData("shaders/ComputeShader/emboss.comp.spv");
        DKObject<DKData> edgedetectData = resourcePool.LoadResourceData("shaders/ComputeShader/edgedetect.comp.spv");
        DKObject<DKData> sharpenData = resourcePool.LoadResourceData("shaders/ComputeShader/sharpen.comp.spv");


        DKObject<GPUShader> VS = DKOBJECT_NEW GPUShader(vertData);
        DKObject<GPUShader> FS = DKOBJECT_NEW GPUShader(fragData);

        DKObject<GPUShader> CS_E = DKOBJECT_NEW GPUShader(embossData);
        DKObject<GPUShader> CS_ED = DKOBJECT_NEW GPUShader(edgedetectData);
        DKObject<GPUShader> CS_SH = DKOBJECT_NEW GPUShader(sharpenData);

        VS->InitializeGpuResource(graphicsQueue);
        FS->InitializeGpuResource(graphicsQueue);

        CS_E->InitializeGpuResource(computeQueue);
        CS_ED->InitializeGpuResource(computeQueue);
        CS_SH->InitializeGpuResource(computeQueue);

        auto VSF = VS->Function();
        auto FSF = FS->Function();
        auto CS_EF = CS_E->Function();
        auto CS_EDF = CS_ED->Function();
        auto CS_SHF = CS_SH->Function();



        // Texture Resource Initialize
        
        ComputeTarget = DKOBJECT_NEW TextureComputeTarget();
        
        DKSamplerDescriptor computeSamplerDesc = {};
        computeSamplerDesc.magFilter = DKSamplerDescriptor::MinMagFilterLinear;
        computeSamplerDesc.minFilter = DKSamplerDescriptor::MinMagFilterLinear;
        computeSamplerDesc.mipFilter = DKSamplerDescriptor::MipFilterLinear;
        computeSamplerDesc.addressModeU = DKSamplerDescriptor::AddressModeClampToEdge;
        computeSamplerDesc.addressModeV = DKSamplerDescriptor::AddressModeClampToEdge;
        computeSamplerDesc.addressModeW = DKSamplerDescriptor::AddressModeClampToEdge;
        computeSamplerDesc.maxAnisotropy = 1.0f;
        computeSamplerDesc.compareFunction = DKCompareFunctionNever;
        DKObject<DKSamplerState> computeSampler = device->CreateSamplerState(computeSamplerDesc);


		// create texture
		DKObject<DKTexture> texture = LoadTexture2D(graphicsQueue, resourcePool.LoadResourceData("textures/deathstar3.png"));
		
        // create sampler
		DKSamplerDescriptor samplerDesc = {};
		samplerDesc.magFilter = DKSamplerDescriptor::MinMagFilterLinear;
		samplerDesc.minFilter = DKSamplerDescriptor::MinMagFilterLinear;
		samplerDesc.addressModeU = DKSamplerDescriptor::AddressModeClampToEdge;
		samplerDesc.addressModeV = DKSamplerDescriptor::AddressModeClampToEdge;
		samplerDesc.addressModeW = DKSamplerDescriptor::AddressModeClampToEdge;
		samplerDesc.maxAnisotropy = 1;
        DKObject<DKSamplerState> sampler = device->CreateSamplerState(samplerDesc);

		
		DKObject<DKSwapChain> swapChain = graphicsQueue->CreateSwapChain(window);

		DKLog("VertexFunction.VertexAttributes: %d", VSF->StageInputAttributes().Count());
		for (int i = 0; i < VSF->StageInputAttributes().Count(); ++i)
		{
			const DKShaderAttribute& attr = VSF->StageInputAttributes().Value(i);
			DKLog("  --> VertexAttribute[%d]: \"%ls\" (location:%u)", i, (const wchar_t*)attr.name, attr.location);
		}

		
		DKRenderPipelineDescriptor pipelineDescriptor;
        // setup shader
        pipelineDescriptor.vertexFunction = VSF;
		pipelineDescriptor.fragmentFunction = FSF;
        
        // setup color-attachment render-targets
		pipelineDescriptor.colorAttachments.Resize(1);
		pipelineDescriptor.colorAttachments.Value(0).pixelFormat = swapChain->ColorPixelFormat();
        pipelineDescriptor.colorAttachments.Value(0).blendingEnabled = false;
        pipelineDescriptor.colorAttachments.Value(0).sourceRGBBlendFactor = DKBlendFactor::SourceAlpha;
        pipelineDescriptor.colorAttachments.Value(0).destinationRGBBlendFactor = DKBlendFactor::OneMinusSourceAlpha;
        // setup depth-stencil
		pipelineDescriptor.depthStencilAttachmentPixelFormat = DKPixelFormat::D32Float;
        pipelineDescriptor.depthStencilDescriptor.depthWriteEnabled = true;
        pipelineDescriptor.depthStencilDescriptor.depthCompareFunction = DKCompareFunctionLessEqual;
   
        // setup vertex buffer and attributes
        pipelineDescriptor.vertexDescriptor = Quad->VertexDescriptor();

        // setup topology and rasterization
		pipelineDescriptor.primitiveTopology = DKPrimitiveType::Triangle;
		pipelineDescriptor.frontFace = DKFrontFace::CCW;
		pipelineDescriptor.triangleFillMode = DKTriangleFillMode::Fill;
		pipelineDescriptor.depthClipMode = DKDepthClipMode::Clip;
		pipelineDescriptor.cullMode = DKCullMode::Back;
		pipelineDescriptor.rasterizationEnabled = true;

		DKPipelineReflection reflection;
		DKObject<DKRenderPipelineState> pipelineState = device->CreateRenderPipeline(pipelineDescriptor, &reflection);
		if (pipelineState)
		{
            PrintPipelineReflection(&reflection, DKLogCategory::Verbose);
		}
        ///
        graphicShaderBindingSet = DKOBJECT_NEW GraphicShaderBindingSet();
        graphicShaderBindingSet->InitializeGpuResource(device);
        auto uboBuffer = graphicShaderBindingSet->UniformBuffer();
        auto ubo = graphicShaderBindingSet->UniformBufferO();

        // ComputerBuffer Layout
        DKShaderBindingSetLayout ComputeLayout;
        if (1)
        {
            DKShaderBinding bindings[2] = {
                {
                    0,
                    DKShader::DescriptorTypeStorageTexture,
                    1,
                    nullptr
                }, // Input Image (read-only)
                {
                    1,
                    DKShader::DescriptorTypeStorageTexture,
                    1,
                    nullptr
                }, // Output image (write)
            };
            ComputeLayout.bindings.Add(bindings, 2);
        }
        DKObject<DKShaderBindingSet> computebindSet = device->CreateShaderBindingSet(ComputeLayout);

        //auto CS_EF = CS_E->Function();
        //auto CS_EDF = CS_ED->Function();
        //auto CS_SHF = CS_SH->Function();

        DKComputePipelineDescriptor embossComputePipelineDescriptor;
        embossComputePipelineDescriptor.computeFunction = CS_EF;
        auto Emboss = device->CreateComputePipeline(embossComputePipelineDescriptor);
        
        DKObject<DKTexture> depthBuffer = nullptr;
        DKObject<DKTexture> targettex = nullptr;

        DKTimer timer;
		timer.Reset();

		DKLog("Render thread begin");
		while (!runningRenderThread.CompareAndSet(0, 0))
		{
			DKRenderPassDescriptor rpd = swapChain->CurrentRenderPassDescriptor();
			double t = timer.Elapsed();
			double waveT = (cos(t) + 1.0) * 0.5;
			rpd.colorAttachments.Value(0).clearColor = DKColor(waveT, 0.0, 0.0, 0.0);

            int width = rpd.colorAttachments.Value(0).renderTarget->Width();
            int height = rpd.colorAttachments.Value(0).renderTarget->Height();
            if (depthBuffer)
            {
                if (depthBuffer->Width() !=  width ||
                    depthBuffer->Height() != height )
                    depthBuffer = nullptr;
            }
            if (depthBuffer == nullptr)
            {
                // create depth buffer
                DKTextureDescriptor texDesc = {};
                texDesc.textureType = DKTexture::Type2D;
                texDesc.pixelFormat = DKPixelFormat::D32Float;
                texDesc.width = width;
                texDesc.height = height;
                texDesc.depth = 1;
                texDesc.mipmapLevels = 1;
                texDesc.sampleCount = 1;
                texDesc.arrayLength = 1;
                texDesc.usage = DKTexture::UsageRenderTarget;
                depthBuffer = device->CreateTexture(texDesc);
            }
            rpd.depthStencilAttachment.renderTarget = depthBuffer;
            rpd.depthStencilAttachment.loadAction = DKRenderPassAttachmentDescriptor::LoadActionClear;
            rpd.depthStencilAttachment.storeAction = DKRenderPassAttachmentDescriptor::StoreActionDontCare;

            targettex = ComputeTarget->ComputeTarget(computeQueue, width, height);

            DKObject<DKCommandBuffer> computeCmdbuffer = computeQueue->CreateCommandBuffer();
            DKObject<DKComputeCommandEncoder> computeEncoder = computeCmdbuffer->CreateComputeCommandEncoder();
            if (computeEncoder)
            {
                if (computebindSet)
                {
                    computebindSet->SetTexture(0, texture);
                    //computebindSet->SetSamplerState(0, sampler);
                    computebindSet->SetTexture(1, targettex);
                    //computebindSet->SetSamplerState(1, sampler);
                }
                computeEncoder->SetComputePipelineState(Emboss);
                computeEncoder->SetResources(0, computebindSet);
                computeEncoder->Dispatch(width / 16, height / 16, 1);
                computeEncoder->EndEncoding();
            }

			DKObject<DKCommandBuffer> buffer = graphicsQueue->CreateCommandBuffer();
			DKObject<DKRenderCommandEncoder> encoder = buffer->CreateRenderCommandEncoder(rpd);

			if (encoder)
			{
                if (graphicShaderBindingSet->PostcomputeDescSet() && ubo)
                {
                    graphicShaderBindingSet->PostcomputeDescSet()->SetBuffer(0, uboBuffer, 0, sizeof(GraphicShaderBindingSet::UBO));
                    graphicShaderBindingSet->PostcomputeDescSet()->SetTexture(1, targettex);
                    graphicShaderBindingSet->PostcomputeDescSet()->SetSamplerState(1, sampler);
                }

				encoder->SetRenderPipelineState(pipelineState);
				encoder->SetVertexBuffer(Quad->VertexBuffer(), 0, 0);
				encoder->SetIndexBuffer(Quad->IndexBuffer(), 0, DKIndexType::UInt32);
                encoder->SetResources(0, graphicShaderBindingSet->PostcomputeDescSet());
				// draw scene!
				encoder->DrawIndexed(Quad->IndicesCount(), 1, 0, 0, 0);
                encoder->EndEncoding();

                if (computeCmdbuffer)
                    computeCmdbuffer->Commit();

				buffer->Commit();

				swapChain->Present();
			}
			else
			{
			}
			DKThread::Sleep(0.01);
		}
		DKLog("RenderThread terminating...");
	}

	void OnInitialize(void) override
	{
        SampleApp::OnInitialize();
		DKLogD("%s", DKGL_FUNCTION_NAME);

        // create window
        window = DKWindow::Create("DefaultWindow");
        window->SetOrigin({ 0, 0 });
        window->Resize({ 320, 240 });
        window->Activate();

        window->AddEventHandler(this, DKFunction([this](const DKWindow::WindowEvent& e)
        {
            if (e.type == DKWindow::WindowEvent::WindowClosed)
                DKApplication::Instance()->Terminate(0);
        }), NULL, NULL);

        Quad = DKOBJECT_NEW UVQuad();

		runningRenderThread = 1;
		renderThread = DKThread::Create(DKFunction(this, &MeshDemo::RenderThread)->Invocation());
	}
	void OnTerminate(void) override
	{
		DKLogD("%s", DKGL_FUNCTION_NAME);

		runningRenderThread = 0;
		renderThread->WaitTerminate();
		renderThread = NULL;
        window = NULL;

        SampleApp::OnTerminate();
	}
};


#ifdef _WIN32
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
					  _In_opt_ HINSTANCE hPrevInstance,
					  _In_ LPWSTR    lpCmdLine,
					  _In_ int       nCmdShow)
#else
int main(int argc, const char * argv[])
#endif
{
    MeshDemo app;
	DKPropertySet::SystemConfig().SetValue("AppDelegate", "AppDelegate");
	DKPropertySet::SystemConfig().SetValue("GraphicsAPI", "Vulkan");
	return app.Run();
}
