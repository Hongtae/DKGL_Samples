
#include "app.h"

DKString ShaderStageNames(uint32_t s)
{
    DKArray<const char*> stages;
    if (s & (uint32_t)DKShaderStage::Vertex)
        stages.Add("Vertex");
    if (s & (uint32_t)DKShaderStage::TessellationControl)
        stages.Add("TessCtrl");
    if (s & (uint32_t)DKShaderStage::TessellationEvaluation)
        stages.Add("TessEval");
    if (s & (uint32_t)DKShaderStage::Geometry)
        stages.Add("Geometry");
    if (s & (uint32_t)DKShaderStage::Fragment)
        stages.Add("Fragment");
    if (s & (uint32_t)DKShaderStage::Compute)
        stages.Add("Compute");

    if (stages.IsEmpty())
        return "";

    DKString str = stages.Value(0);
    for (int i = 1; i < stages.Count(); ++i)
        str += DKString::Format(", %ls", stages.Value(i));
    return str;
}

const char* ShaderDataTypeStr(DKShaderDataType t)
{
    switch (t) {
    case DKShaderDataType::Unknown:				return "Unknown";
    case DKShaderDataType::None:				return "None";

    case DKShaderDataType::Struct:				return "Struct";
    case DKShaderDataType::Texture:				return "Texture";
    case DKShaderDataType::Sampler:				return "Sampler";

    case DKShaderDataType::Float:				return "Float";
    case DKShaderDataType::Float2:				return "Float2";
    case DKShaderDataType::Float3:				return "Float3";
    case DKShaderDataType::Float4:				return "Float4";

    case DKShaderDataType::Float2x2:			return "Float2x2";
    case DKShaderDataType::Float2x3:			return "Float2x3";
    case DKShaderDataType::Float2x4:			return "Float2x4";

    case DKShaderDataType::Float3x2:			return "Float3x2";
    case DKShaderDataType::Float3x3:			return "Float3x3";
    case DKShaderDataType::Float3x4:			return "Float3x4";

    case DKShaderDataType::Float4x2:			return "Float4x2";
    case DKShaderDataType::Float4x3:			return "Float4x3";
    case DKShaderDataType::Float4x4:			return "Float4x4";

    case DKShaderDataType::Half:				return "Half";
    case DKShaderDataType::Half2:				return "Half2";
    case DKShaderDataType::Half3:				return "Half3";
    case DKShaderDataType::Half4:				return "Half4";

    case DKShaderDataType::Half2x2:				return "Half2x2";
    case DKShaderDataType::Half2x3:				return "Half2x3";
    case DKShaderDataType::Half2x4:				return "Half2x4";

    case DKShaderDataType::Half3x2:				return "Half3x2";
    case DKShaderDataType::Half3x3:				return "Half3x3";
    case DKShaderDataType::Half3x4:				return "Half3x4";

    case DKShaderDataType::Half4x2:				return "Half4x2";
    case DKShaderDataType::Half4x3:				return "Half4x3";
    case DKShaderDataType::Half4x4:				return "Half4x4";

    case DKShaderDataType::Int:					return "Int";
    case DKShaderDataType::Int2:				return "Int2";
    case DKShaderDataType::Int3:				return "Int3";
    case DKShaderDataType::Int4:				return "Int4";

    case DKShaderDataType::UInt:				return "UInt";
    case DKShaderDataType::UInt2:				return "UInt2";
    case DKShaderDataType::UInt3:				return "UInt3";
    case DKShaderDataType::UInt4:				return "UInt4";

    case DKShaderDataType::Short:				return "Short";
    case DKShaderDataType::Short2:				return "Short2";
    case DKShaderDataType::Short3:				return "Short3";
    case DKShaderDataType::Short4:				return "Short4";

    case DKShaderDataType::UShort:				return "UShort";
    case DKShaderDataType::UShort2:				return "UShort2";
    case DKShaderDataType::UShort3:				return "UShort3";
    case DKShaderDataType::UShort4:				return "UShort4";

    case DKShaderDataType::Char:				return "Char";
    case DKShaderDataType::Char2:				return "Char2";
    case DKShaderDataType::Char3:				return "Char3";
    case DKShaderDataType::Char4:				return "Char4";

    case DKShaderDataType::UChar:				return "UChar";
    case DKShaderDataType::UChar2:				return "UChar2";
    case DKShaderDataType::UChar3:				return "UChar3";
    case DKShaderDataType::UChar4:				return "UChar4";

    case DKShaderDataType::Bool:				return "Bool";
    case DKShaderDataType::Bool2:				return "Bool2";
    case DKShaderDataType::Bool3:				return "Bool3";
    case DKShaderDataType::Bool4:				return "Bool4";
    }
    return "Error";
}

void PrintShaderResource(const DKShaderResource& res, DKLogCategory c = DKLogCategory::Info)
{
    struct MemberPrinter
    {
        const DKShaderResource& res;
        int indent;
        DKLogCategory c;
        void operator()(const DKShaderResourceStruct& str) const
        {
            DKString indentStr = "";
            for (int i = 0; i < indent; ++i)
            {
                indentStr += "    ";
            }
            for (const DKShaderResourceStructMember& mem : str.members)
            {
                if (mem.count > 1)
                {
                    DKLog(c, " %ls+ %ls[%d] (%s, Offset: %d, Stride: %d)",
                        (const wchar_t*)indentStr,
                          (const wchar_t*)mem.name,
                          mem.count,
                          ShaderDataTypeStr(mem.dataType),
                          mem.offset, mem.stride);

                }
                else
                {
                    DKLog(c, " %ls+ %ls (%s, Offset: %d)",
                        (const wchar_t*)indentStr,
                          (const wchar_t*)mem.name,
                          ShaderDataTypeStr(mem.dataType),
                          mem.offset);
                }

                auto* p = res.structTypeMemberMap.Find(mem.typeInfoKey);
                if (p)
                {
                    DKLog(c, " %ls  Struct \"%ls\"",
                        (const wchar_t*)indentStr,
                          (const wchar_t*)mem.typeInfoKey);
                    MemberPrinter{ res, indent + 1, c}.operator()(p->value);
                }
            }
        }
    };


    if (res.count > 1)
        DKLog(c, "ShaderResource: %ls[%d] (set=%d, binding=%d, stages=%ls)",
        (const wchar_t*)res.name, res.count, res.set, res.binding,
              (const wchar_t*)ShaderStageNames(res.stages));
    else
        DKLog(c, "ShaderResource: %ls (set=%d, binding=%d, stages=%ls)",
        (const wchar_t*)res.name, res.set, res.binding,
              (const wchar_t*)ShaderStageNames(res.stages));

    const char* type = "Unknown (ERROR)";
    switch (res.type)
    {
    case DKShaderResource::TypeBuffer: type = "Buffer"; break;
    case DKShaderResource::TypeTexture:	type = "Texture"; break;
    case DKShaderResource::TypeSampler:	type = "Sampler"; break;
    case DKShaderResource::TypeTextureSampler: type = "SampledTexture"; break;
    }
    const char* access = "Unknown (ERROR)";
    switch (res.access)
    {
    case DKShaderResource::AccessReadOnly:	access = "ReadOnly"; break;
    case DKShaderResource::AccessWriteOnly:	access = "WriteOnly"; break;
    case DKShaderResource::AccessReadWrite:	access = "ReadWrite"; break;
    }

    if (res.type == DKShaderResource::TypeBuffer)
    {
        DKLog(c, " Type:%s, Access:%s, Enabled:%d, Size:%d",
              type,
              access,
              int(res.enabled),
              res.typeInfo.buffer.size);
    }
    else
    {
        DKLog(c, " Type:%s, Access:%s, Enabled:%d",
              type,
              access,
              int(res.enabled));
    }
    if (res.typeInfoKey.Length() > 0)
        DKLog(c, " Struct \"%ls\"", (const wchar_t*)res.typeInfoKey);
    if (res.type == DKShaderResource::TypeBuffer)
    {
        auto p = res.structTypeMemberMap.Find(res.typeInfoKey);
        if (p)
            MemberPrinter{ res, 1 , c}.operator()(p->value);
    }
}

void PrintPipelineReflection(const DKPipelineReflection* reflection, DKLogCategory c = DKLogCategory::Error)
{
    DKLog(c, "=========================================================");
    DKLog(c, "PipelineReflection.InputAttributes: %d", reflection->inputAttributes.Count());
    for (int i = 0; i < reflection->inputAttributes.Count(); ++i)
    {
        const DKShaderAttribute& attr = reflection->inputAttributes.Value(i);
        DKLog(c, "  [in] ShaderAttribute[%d]: \"%ls\" (location:%u)",
              i, (const wchar_t*)attr.name, attr.location);
    }
    DKLog(c, "---------------------------------------------------------");
    DKLog(c, "PipelineReflection.Resources: %d", reflection->resources.Count());
    for (auto& arg : reflection->resources)
        PrintShaderResource(arg, c);
    for (int i = 0; i < reflection->pushConstantLayouts.Count(); ++i)
    {
        const DKShaderPushConstantLayout& layout = reflection->pushConstantLayouts.Value(i);
        DKLog(c, " PushConstant:%d \"%ls\" (offset:%u, size:%u, stages:%ls)",
              i, (const wchar_t*)layout.name, layout.offset, layout.size,
              (const wchar_t*)ShaderStageNames(layout.stages));
    }
    DKLog(c, "=========================================================");
}


class TestApp1 : public DKApplication
{
	DKObject<DKWindow> window;
	DKObject<DKThread> renderThread;
	DKResourcePool resourcePool;

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
				encoder->DrawIndexed(indexData.Count(), 1, 0, 0, 1);
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
		DKLogD("%s", DKGL_FUNCTION_NAME);

		DKString resPath = DefaultPath(SystemPath::AppResource);
		resPath = resPath.FilePathStringByAppendingPath("Data");
		DKLog("resPath: %ls", (const wchar_t*)resPath);
		resourcePool.AddLocatorForPath(resPath);

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
		renderThread = DKThread::Create(DKFunction(this, &TestApp1::RenderThread)->Invocation());
	}
	void OnTerminate(void) override
	{
		DKLogD("%s", DKGL_FUNCTION_NAME);

		runningRenderThread = 0;
		renderThread->WaitTerminate();
		renderThread = NULL;
		window = NULL;

		DKLogI("Memory Pool Statistics");
		size_t numBuckets = DKMemoryPoolNumberOfBuckets();
		DKMemoryPoolBucketStatus* buckets = new DKMemoryPoolBucketStatus[numBuckets];
		DKMemoryPoolQueryAllocationStatus(buckets, numBuckets);
		size_t usedBytes = 0;
		for (int i = 0; i < numBuckets; ++i)
		{
			if (buckets[i].totalChunks > 0)
			{
				DKLogI("--> %5lu:  %5lu/%5lu, usage: %.1f%%, used: %.1fKB, total: %.1fKB",
					   buckets[i].chunkSize,
					   buckets[i].usedChunks, buckets[i].totalChunks,
					   double(buckets[i].usedChunks) / double(buckets[i].totalChunks) * 100.0,
					   double(buckets[i].chunkSize * buckets[i].usedChunks) / 1024.0,
					   double(buckets[i].chunkSize * buckets[i].totalChunks) / 1024.0
				);
				usedBytes += buckets[i].chunkSize * buckets[i].usedChunks;
			}
		}
		DKLogI("MemoryPool Usage: %.1fMB / %.1fMB", double(usedBytes) / (1024 * 1024), double(DKMemoryPoolSize()) / (1024 * 1024));
		delete[] buckets;
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
	TestApp1 app;
	DKPropertySet::SystemConfig().SetValue("AppDelegate", "AppDelegate");
	DKPropertySet::SystemConfig().SetValue("GraphicsAPI", "Vulkan");
	return app.Run();
}
