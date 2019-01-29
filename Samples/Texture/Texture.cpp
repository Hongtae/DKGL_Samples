
#include "app.h"
#include "util.h"


class TextureDemo : public SampleApp
{
    DKObject<DKWindow> window;
	DKObject<DKThread> renderThread;
	DKAtomicNumber32 runningRenderThread;

public:
	void RenderThread(void)
	{
		DKObject<DKData> vertData = resourcePool.LoadResourceData("triangle.vert.spv");
		DKObject<DKData> fragData = resourcePool.LoadResourceData("triangle.frag.spv");
		DKShader vertShader(vertData);
		DKShader fragShader(fragData);

		DKObject<DKGraphicsDevice> device = DKGraphicsDevice::SharedInstance();
		DKObject<DKShaderModule> vertShaderModule = device->CreateShaderModule(&vertShader);
		DKObject<DKShaderModule> fragShaderModule = device->CreateShaderModule(&fragShader);

		DKObject<DKShaderFunction> vertShaderFunction = vertShaderModule->CreateFunction(vertShaderModule->FunctionNames().Value(0));
		DKObject<DKShaderFunction> fragShaderFunction = fragShaderModule->CreateFunction(fragShaderModule->FunctionNames().Value(0));

		DKObject<DKCommandQueue> queue = device->CreateCommandQueue(DKCommandQueue::Graphics);
		DKObject<DKSwapChain> swapChain = queue->CreateSwapChain(window);

		DKLog("VertexFunction.VertexAttributes: %d", vertShaderFunction->StageInputAttributes().Count());
		for (int i = 0; i < vertShaderFunction->StageInputAttributes().Count(); ++i)
		{
			const DKShaderAttribute& attr = vertShaderFunction->StageInputAttributes().Value(i);
			DKLog("  --> VertexAttribute[%d]: \"%ls\" (location:%u)", i, (const wchar_t*)attr.name, attr.location);
		}

		struct Vertex
		{
			DKVector3 position;
			DKVector3 color;
		};
		DKArray<Vertex> vertexData =
		{
			{ {  0.0f, -0.5f, 0.0f },{ 1.0f, 1.0f, 1.0f } },
			{ {  0.5f,  0.5f, 0.0f },{ 0.0f, 1.0f, 0.0f } },
			{ { -0.5f,  0.5f, 0.0f },{ 0.0f, 0.0f, 1.0f } }
		};
		uint32_t vertexBufferSize = static_cast<uint32_t>(vertexData.Count()) * sizeof(Vertex);
		DKArray<uint32_t> indexData = { 0, 1, 2 };
		uint32_t indexBufferSize = indexData.Count() * sizeof(uint32_t);

		DKObject<DKGpuBuffer> vertexBuffer = device->CreateBuffer(vertexBufferSize, DKGpuBuffer::StorageModeShared, DKCpuCacheModeDefault);
		memcpy(vertexBuffer->Lock(), vertexData, vertexBufferSize);
		vertexBuffer->Unlock();

		DKObject<DKGpuBuffer> indexBuffer = device->CreateBuffer(indexBufferSize, DKGpuBuffer::StorageModeShared, DKCpuCacheModeDefault);
		memcpy(indexBuffer->Lock(), indexData, indexBufferSize);
		indexBuffer->Unlock();

		DKRenderPipelineDescriptor pipelineDescriptor;
		pipelineDescriptor.vertexFunction = vertShaderFunction;
		pipelineDescriptor.fragmentFunction = fragShaderFunction;
		pipelineDescriptor.colorAttachments.Resize(1);
		pipelineDescriptor.colorAttachments.Value(0).pixelFormat = swapChain->ColorPixelFormat();
		pipelineDescriptor.depthStencilAttachmentPixelFormat = DKPixelFormat::Invalid; // no depth buffer
		pipelineDescriptor.vertexDescriptor.attributes = {
			{ DKVertexFormat::Float3, 0, 0, 0 },
			{ DKVertexFormat::Float3, sizeof(DKVector3), 0, 1 },
		};
		pipelineDescriptor.vertexDescriptor.layouts = {
			{ DKVertexStepRate::Vertex, sizeof(Vertex), 0 },
		};
		pipelineDescriptor.primitiveTopology = DKPrimitiveType::Triangle;
		pipelineDescriptor.frontFace = DKFrontFace::CCW;
		pipelineDescriptor.triangleFillMode = DKTriangleFillMode::Fill;
		pipelineDescriptor.depthClipMode = DKDepthClipMode::Clip;
		pipelineDescriptor.cullMode = DKCullMode::None;
		pipelineDescriptor.rasterizationEnabled = true;

		DKPipelineReflection reflection;
		DKObject<DKRenderPipelineState> pipelineState = device->CreateRenderPipeline(pipelineDescriptor, &reflection);
		if (pipelineState)
		{
            PrintPipelineReflection(&reflection, DKLogCategory::Verbose);
		}

        DKShaderBindingSetLayout layout;
        if (1)
        {
            DKShaderBinding binding = {
                0,
                DKShader::DescriptorTypeUniformBuffer,
                1,
                nullptr
            };
            layout.bindings.Add(binding);
        }
        DKObject<DKShaderBindingSet> bindSet = device->CreateShaderBindingSet(layout);
        if (bindSet)
        {
            struct
            {
                DKMatrix4 projectionMatrix;
                DKMatrix4 modelMatrix;
                DKMatrix4 viewMatrix;
            } ubo;

            DKObject<DKGpuBuffer> uboBuffer = device->CreateBuffer(sizeof(ubo), DKGpuBuffer::StorageModeShared, DKCpuCacheModeDefault);
            if (uboBuffer)
            {
                ubo.projectionMatrix = DKMatrix4::identity;
                ubo.modelMatrix = DKMatrix4::identity;
                ubo.viewMatrix = DKMatrix4::identity;

                void* p = uboBuffer->Lock(0);
                if (p)
                {
                    memcpy(p, &ubo, sizeof(ubo));
                    uboBuffer->Unlock();

                    bindSet->SetBuffer(0, uboBuffer, 0, sizeof(ubo));
                }
                else
                {
                    DKLogE("GpuBuffer Lock failed!");
                }
            }
        }

		DKTimer timer;
		timer.Reset();

		DKLog("Render thread begin");
		while (!runningRenderThread.CompareAndSet(0, 0))
		{
			DKRenderPassDescriptor rpd = swapChain->CurrentRenderPassDescriptor();
			double t = timer.Elapsed();
			t = (cos(t) + 1.0) * 0.5;
			rpd.colorAttachments.Value(0).clearColor = DKColor(t, 0.0, 0.0, 0.0);

			DKObject<DKCommandBuffer> buffer = queue->CreateCommandBuffer();
			DKObject<DKRenderCommandEncoder> encoder = buffer->CreateRenderCommandEncoder(rpd);
			if (encoder)
			{
				encoder->SetRenderPipelineState(pipelineState);
				encoder->SetVertexBuffer(vertexBuffer, 0, 0);
				encoder->SetIndexBuffer(indexBuffer, 0, DKIndexType::UInt32);
                encoder->SetResources(0, bindSet);
				// draw scene!
				encoder->DrawIndexed(indexData.Count(), 1, 0, 0, 0);
				encoder->EndEncoding();
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

		runningRenderThread = 1;
		renderThread = DKThread::Create(DKFunction(this, &TextureDemo::RenderThread)->Invocation());
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
    TextureDemo app;
	DKPropertySet::SystemConfig().SetValue("AppDelegate", "AppDelegate");
	DKPropertySet::SystemConfig().SetValue("GraphicsAPI", "Vulkan");
	return app.Run();
}
