#include <cstddef>
#include "app.h"
#include "util.h"

#define TINYOBJLOADER_IMPLMENTATION
#include "tiny_obj_loader.h"
#include <unordered_map>


class SampleObjMesh
{
public:
	struct Vertex
	{
		DKVector3 inPos;
		DKVector3 inColor;
		DKVector2 intexCoord;

		int Compare(const Vertex& Other) const
		{

			if ( this == &Other 
				|| inPos == Other.inPos && inColor == Other.inColor && intexCoord == Other.intexCoord)
			{
				return 0;
			}
			else
			{
				return static_cast<int>(this - &Other);
			}
		}

		bool operator > (const Vertex& Other) const
		{
			return Compare(Other) > 0;
		}

		bool operator < (const Vertex& Other) const
		{
			return Compare(Other) < 0;
		}
	};

	SampleObjMesh() 
	{
		vertices.Reserve(100);
		indices.Reserve(100);
	}

	void LoadFromObjFile(const char* InPath)
	{
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string err;
		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, InPath)) {
			throw std::runtime_error(err);
		}
		
		DKMap<Vertex, uint32_t> uniqueVertices;
		DKLog("Save to Container");
		for (const auto& shape : shapes)
		{
			for (const auto& index : shape.mesh.indices)
			{
				Vertex vertex = {};

				vertex.inPos = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};

				if (attrib.texcoords.size())
				{
					vertex.intexCoord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
					};
				}				

				vertex.inColor = { 1.0f, 1.0f, 1.0f };

				if (uniqueVertices.Find(vertex) == nullptr)
				{
					uniqueVertices.Insert(vertex, static_cast<uint32_t>(vertices.Count()));
					vertices.Add(vertex);
				}

				indices.Add(uniqueVertices.Value(vertex));
			}
		}
	}

	uint32_t GetVerticesCount() const { 
		return static_cast<uint32_t>(vertices.Count()); };
	uint32_t GetIndicesCount() const { 
		return static_cast<uint32_t>(indices.Count()); };
	const Vertex* GetVerticesData() const { 
		return vertices; }
	const uint32_t* GetIndicesData() const { 
		return indices; }

private:
	DKArray<Vertex> vertices;
	DKArray<uint32_t> indices;
	DKSpinLock                  MeshLock;
};

///// Template Spealization for DKString. (for DKMap, DKSet)
//template <> struct DKMapKeyComparator<SampleObjMesh::Vertex>
//{
//	int operator () (const DKStringW& lhs, const DKStringW& rhs) const
//	{
//		return lhs.Compare(rhs);
//	}
//};


class MeshDemo : public SampleApp
{
    DKObject<DKWindow> window;
	DKObject<DKThread> renderThread;
	DKAtomicNumber32 runningRenderThread;
	DKObject<SampleObjMesh> SampleMesh;

public:
	void LoadMesh()
	{
		
		DKLog("Loading Mesh");
		SampleMesh->LoadFromObjFile("..\\Data\\meshes\\car.obj");
	}

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
            texDesc.usage = DKTexture::UsageCopyDestination | DKTexture::UsageSampled;
            DKObject<DKTexture> tex = device->CreateTexture(texDesc);
            if (tex)
            {
                size_t bytesPerPixel = image->BytesPerPixel();
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
		DKObject<DKData> vertData = resourcePool.LoadResourceData("shaders/mesh.vert.spv");
		DKObject<DKData> fragData = resourcePool.LoadResourceData("shaders/mesh.frag.spv");
		DKShader vertShader(vertData);
		DKShader fragShader(fragData);

		DKObject<DKGraphicsDevice> device = DKGraphicsDevice::SharedInstance();
        DKObject<DKCommandQueue> queue = device->CreateCommandQueue(DKCommandQueue::Graphics);

		// create texture
		DKObject<DKTexture> texture = LoadTexture2D(queue, resourcePool.LoadResourceData("textures/deathstar3.png"));
		// create sampler
		DKSamplerDescriptor samplerDesc = {};
		samplerDesc.magFilter = DKSamplerDescriptor::MinMagFilterLinear;
		samplerDesc.minFilter = DKSamplerDescriptor::MinMagFilterLinear;
		samplerDesc.addressModeU = DKSamplerDescriptor::AddressModeRepeat;
		samplerDesc.addressModeV = DKSamplerDescriptor::AddressModeRepeat;
		samplerDesc.addressModeW = DKSamplerDescriptor::AddressModeRepeat;
		samplerDesc.maxAnisotropy = 16;
		
		DKObject<DKSamplerState> sampler = device->CreateSamplerState(samplerDesc);

        // create shaders
		DKObject<DKShaderModule> vertShaderModule = device->CreateShaderModule(&vertShader);
		DKObject<DKShaderModule> fragShaderModule = device->CreateShaderModule(&fragShader);

		DKObject<DKShaderFunction> vertShaderFunction = vertShaderModule->CreateFunction(vertShaderModule->FunctionNames().Value(0));
		DKObject<DKShaderFunction> fragShaderFunction = fragShaderModule->CreateFunction(fragShaderModule->FunctionNames().Value(0));

		DKObject<DKSwapChain> swapChain = queue->CreateSwapChain(window);

		DKLog("VertexFunction.VertexAttributes: %d", vertShaderFunction->StageInputAttributes().Count());
		for (int i = 0; i < vertShaderFunction->StageInputAttributes().Count(); ++i)
		{
			const DKShaderAttribute& attr = vertShaderFunction->StageInputAttributes().Value(i);
			DKLog("  --> VertexAttribute[%d]: \"%ls\" (location:%u)", i, (const wchar_t*)attr.name, attr.location);
		}

	
		uint32_t vertexBufferSize = static_cast<uint32_t>(SampleMesh->GetVerticesCount()) * sizeof(SampleObjMesh::Vertex);
		uint32_t indexBufferSize = SampleMesh->GetIndicesCount() * sizeof(uint32_t);

		DKObject<DKGpuBuffer> vertexBuffer = device->CreateBuffer(vertexBufferSize, DKGpuBuffer::StorageModeShared, DKCpuCacheModeReadWrite);
		memcpy(vertexBuffer->Contents(), SampleMesh->GetVerticesData(), vertexBufferSize);
        vertexBuffer->Flush();

		DKObject<DKGpuBuffer> indexBuffer = device->CreateBuffer(indexBufferSize, DKGpuBuffer::StorageModeShared, DKCpuCacheModeReadWrite);
		memcpy(indexBuffer->Contents(), SampleMesh->GetIndicesData(), indexBufferSize);
        indexBuffer->Flush();

		DKRenderPipelineDescriptor pipelineDescriptor;
		pipelineDescriptor.vertexFunction = vertShaderFunction;
		pipelineDescriptor.fragmentFunction = fragShaderFunction;
		pipelineDescriptor.colorAttachments.Resize(1);
		pipelineDescriptor.colorAttachments.Value(0).pixelFormat = swapChain->ColorPixelFormat();
        pipelineDescriptor.colorAttachments.Value(0).blendingEnabled = true;
        pipelineDescriptor.colorAttachments.Value(0).sourceRGBBlendFactor = DKBlendFactor::SourceAlpha;
        pipelineDescriptor.colorAttachments.Value(0).destinationRGBBlendFactor = DKBlendFactor::OneMinusSourceAlpha;
		pipelineDescriptor.depthStencilAttachmentPixelFormat = DKPixelFormat::Invalid; // no depth buffer
		pipelineDescriptor.vertexDescriptor.attributes = {
			{ DKVertexFormat::Float3, offsetof(SampleObjMesh::Vertex, inPos), 0, 0 },
            { DKVertexFormat::Float3, offsetof(SampleObjMesh::Vertex, inColor), 0, 1 },
			{ DKVertexFormat::Float2, offsetof(SampleObjMesh::Vertex, intexCoord), 0, 2 },
		};
		pipelineDescriptor.vertexDescriptor.layouts = {
			{ DKVertexStepRate::Vertex, sizeof(SampleObjMesh::Vertex), 0 },
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
            layout.bindings.Add(bindings, 2);
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

            DKObject<DKGpuBuffer> uboBuffer = device->CreateBuffer(sizeof(ubo), DKGpuBuffer::StorageModeShared, DKCpuCacheModeReadWrite);
            if (uboBuffer)
            {
                ubo.projectionMatrix = DKMatrix4::identity;
                ubo.modelMatrix = DKMatrix4::identity;
                ubo.viewMatrix = DKMatrix4::identity;

                memcpy(uboBuffer->Contents(), &ubo, sizeof(ubo));
                bindSet->SetBuffer(0, uboBuffer, 0, sizeof(ubo));
                uboBuffer->Flush();
            }
			         

            bindSet->SetTexture(1, texture);
            bindSet->SetSamplerState(1, sampler);
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
				encoder->DrawIndexed(SampleMesh->GetIndicesCount(), 1, 0, 0, 0);
				encoder->EndEncoding();
				buffer->Commit();
				swapChain->Present();
			}
			else
			{
			}
			DKThread::Sleep(0.5);
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

		if (SampleMesh == nullptr)
		{
			SampleMesh = new SampleObjMesh;
		}

		LoadMesh();

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
