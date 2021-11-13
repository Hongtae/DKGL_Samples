#include <cstddef>
#include "app.h"
#include "util.h"

//#define TINYOBJLOADER_IMPLMENTATION
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
            const float pp[] = { this->inPos.x, this->inPos.y, this->inPos.z,
                this->inColor.x, this->inColor.y, this->inColor.z,
                this->intexCoord.x, this->intexCoord.y};
            const float rr[] = { Other.inPos.x, Other.inPos.y, Other.inPos.z,
                Other.inColor.x, Other.inColor.y, Other.inColor.z,
                Other.intexCoord.x, Other.intexCoord.y};

            for (int i = 0; i < std::size(pp); ++i)
            {
                auto k = pp[i] - rr[i];
                if (abs(k) < 0.0001)
                    continue;
                if (k > 0)
                    return 1;
                else if (k < 0)
                    return -1;
            }
            return 0;
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

                aabb.Expand(vertex.inPos);
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

    DKAabb aabb;
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


class MaterialDemo : public SampleApp
{
    DKObject<DKWindow> window;
	DKObject<DKThread> renderThread;
	DKAtomicNumber32 runningRenderThread;
	DKObject<SampleObjMesh> SampleMesh;

public:
	void LoadMesh()
	{
		
		DKLog("Loading Mesh");
        DKString path = resourcePool.ResourceFilePath("meshes/VikingRoom/viking_room.obj");
		SampleMesh->LoadFromObjFile(DKStringU8(path));
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
		DKObject<DKData> vertData = resourcePool.LoadResourceData("shaders/mesh.vert.spv");
		DKObject<DKData> fragData = resourcePool.LoadResourceData("shaders/mesh.frag.spv");
        DKObject<DKShader> vertShader = DKOBJECT_NEW DKShader(vertData);
        DKObject<DKShader> fragShader = DKOBJECT_NEW DKShader(fragData);

		DKObject<DKGraphicsDevice> device = DKGraphicsDevice::SharedInstance();
        DKObject<DKCommandQueue> queue = device->CreateCommandQueue(DKCommandQueue::Graphics);

		DKObject<DKSwapChain> swapChain = queue->CreateSwapChain(window);

        DKObject<DKMesh> mesh = DKOBJECT_NEW DKMesh();

        if (true)
        {
            // setup vertex buffer, index buffer
		    uint32_t vertexBufferSize = static_cast<uint32_t>(SampleMesh->GetVerticesCount()) * sizeof(SampleObjMesh::Vertex);
		    uint32_t indexBufferSize = SampleMesh->GetIndicesCount() * sizeof(uint32_t);

		    DKObject<DKGpuBuffer> vertexBuffer = device->CreateBuffer(vertexBufferSize, DKGpuBuffer::StorageModeShared, DKCpuCacheModeReadWrite);
		    memcpy(vertexBuffer->Contents(), SampleMesh->GetVerticesData(), vertexBufferSize);
            vertexBuffer->Flush();

		    DKObject<DKGpuBuffer> indexBuffer = device->CreateBuffer(indexBufferSize, DKGpuBuffer::StorageModeShared, DKCpuCacheModeReadWrite);
		    memcpy(indexBuffer->Contents(), SampleMesh->GetIndicesData(), indexBufferSize);
            indexBuffer->Flush();

            mesh->vertexBuffers.Add({
                {
                    { DKVertexStream::Position, DKVertexFormat::Float3, offsetof(SampleObjMesh::Vertex, inPos), "inPos" },
                    { DKVertexStream::Color,    DKVertexFormat::Float3, offsetof(SampleObjMesh::Vertex, inColor), "inColor" },
                    { DKVertexStream::TexCoord, DKVertexFormat::Float2, offsetof(SampleObjMesh::Vertex, intexCoord), "inTexCoord" }
                },  // DKVertexBuffer.declarations
                vertexBuffer,   // DKVertexBuffer.buffer
                0,
                SampleMesh->GetVerticesCount(),
                sizeof(SampleObjMesh::Vertex)
            });
            mesh->vertexStart = 0;
            mesh->vertexCount = SampleMesh->GetVerticesCount();
            mesh->indexBuffer = indexBuffer;
            mesh->indexCount = SampleMesh->GetIndicesCount();
            mesh->indexType = DKIndexType::UInt32;
            mesh->primitiveType = DKPrimitiveType::Triangle;
        }
        if (true)
        {
            // create shaders
            DKObject<DKShaderModule> vertShaderModule = device->CreateShaderModule(vertShader);
            DKObject<DKShaderModule> fragShaderModule = device->CreateShaderModule(fragShader);

            DKObject<DKShaderFunction> vertShaderFunction = vertShaderModule->CreateFunction(vertShaderModule->FunctionNames().Value(0));
            DKObject<DKShaderFunction> fragShaderFunction = fragShaderModule->CreateFunction(fragShaderModule->FunctionNames().Value(0));

            DKLog("VertexFunction.VertexAttributes: %d", vertShaderFunction->StageInputAttributes().Count());
            for (int i = 0; i < vertShaderFunction->StageInputAttributes().Count(); ++i)
            {
                const DKShaderAttribute& attr = vertShaderFunction->StageInputAttributes().Value(i);
                DKLog("  --> VertexAttribute[%d]: \"%ls\" (location:%u)", i, (const wchar_t*)attr.name, attr.location);
            }

            // create texture
            DKObject<DKTexture> texture = LoadTexture2D(queue, resourcePool.LoadResourceData("meshes/VikingRoom/viking_room.png"));
            // create sampler
            DKSamplerDescriptor samplerDesc = {};
            samplerDesc.magFilter = DKSamplerDescriptor::MinMagFilterLinear;
            samplerDesc.minFilter = DKSamplerDescriptor::MinMagFilterLinear;
            samplerDesc.addressModeU = DKSamplerDescriptor::AddressModeClampToEdge;
            samplerDesc.addressModeV = DKSamplerDescriptor::AddressModeClampToEdge;
            samplerDesc.addressModeW = DKSamplerDescriptor::AddressModeClampToEdge;
            samplerDesc.maxAnisotropy = 16;

            DKObject<DKSamplerState> sampler = device->CreateSamplerState(samplerDesc);


            // setup material, PSO
            mesh->material = DKOBJECT_NEW DKMaterial();
            DKMaterial* material = mesh->material;
            material->shaderTemplates.Update(DKShaderStage::Vertex, {
                vertShader,
                vertShaderFunction,
                {
                }, // resourceTypes
                {
                    {"inPos", DKVertexStream::Position},
                    {"inColor", DKVertexStream::Color},
                    {"inTexCoord", DKVertexStream::TexCoord}
                }  // inputAttributeTypes
            });
            material->shaderTemplates.Update(DKShaderStage::Fragment, {
                fragShader,
                fragShaderFunction,
                // no inputAttributes in fragment-shader
            });

            material->textureProperties.Update("samplerColor", {texture});
            material->samplerProperties.Update("samplerColor", {sampler});
#if 0
            // default values for struct
            material->structElementProperties.Update("ubo.projection", {
                DKShaderDataType::Float4x4, 
                {reinterpret_cast<const uint8_t*>(DKMatrix4::identity.val), sizeof(DKMatrix4) }
            });
            material->structElementProperties.Update("ubo.model", {
                DKShaderDataType::Float4x4,
                {reinterpret_cast<const uint8_t*>(DKMatrix4::identity.val), sizeof(DKMatrix4) }
            });
            material->structElementProperties.Update("ubo.view", {
                DKShaderDataType::Float4x4,
                {reinterpret_cast<const uint8_t*>(DKMatrix4::identity.val), sizeof(DKMatrix4) }
            });
#endif
            material->depthStencilAttachmentPixelFormat = DKPixelFormat::D32Float;
            material->depthStencilDescriptor.depthWriteEnabled = true;
            material->depthStencilDescriptor.depthCompareFunction = DKCompareFunctionLessEqual;
            material->colorAttachments = {
                {
                    0, // render-target (color-attachment) index
                    swapChain->ColorPixelFormat(),
                    DKBlendState::defaultAlpha
                }
            };
        }
        if (mesh->InitResources(device))
        {
            PrintPipelineReflection(mesh->PipelineReflection(), DKLogCategory::Verbose);


            DKObject<DKTexture> depthBuffer = nullptr;

            DKCamera camera;
            DKVector3 cameraPosition = { 0, 5, 10 };
            DKVector3 cameraTartget = { 0, 0, 0 };

            DKAffineTransform3 tm(DKLinearTransform3().Scale(5).Rotate(DKVector3(-1,0,0), DKGL_PI * 0.5));

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

                DKObject<DKCommandBuffer> buffer = queue->CreateCommandBuffer();
                DKObject<DKRenderCommandEncoder> encoder = buffer->CreateRenderCommandEncoder(rpd);
                if (encoder)
                {
                    camera.SetView(cameraPosition, cameraTartget - cameraPosition, DKVector3(0, 1, 0));
                    camera.SetPerspective(DKGL_DEGREE_TO_RADIAN(90), float(width)/float(height), 1, 1000);

                    DKQuaternion quat(DKVector3(0, 1, 0), t);
                    DKAffineTransform3 trans = tm * DKAffineTransform3(quat);

                    struct
                    {
                        DKMatrix4 projectionMatrix;
                        DKMatrix4 modelMatrix;
                        DKMatrix4 viewMatrix;
                    } ubo;
                    ubo.projectionMatrix = camera.ProjectionMatrix();
                    ubo.modelMatrix = trans.Matrix4();
                    ubo.viewMatrix = camera.ViewMatrix();

                    // update shader properties..
                    bool bindstruct = true;
                    if (bindstruct)
                    {
                        // bind struct 
                        mesh->structProperties.Value("ubo").Set(&ubo, sizeof(ubo));
                    }
                    else
                    {
                        // bind struct elements separately
                        mesh->structProperties.Value("ubo.projection").Set(ubo.projectionMatrix.val);
                        mesh->structProperties.Value("ubo.model").Set(ubo.modelMatrix.val);
                        mesh->structProperties.Value("ubo.view").Set(ubo.viewMatrix.val);
                    }

                    mesh->UpdateMaterialProperties(nullptr);
                    mesh->EncodeRenderCommand(encoder, 1, 0);

                    encoder->EndEncoding();
                    buffer->Commit();
                    swapChain->Present();
                }
                else
                {
                }
                DKThread::Sleep(0.01);
            }
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

        SampleMesh = DKOBJECT_NEW SampleObjMesh();

		LoadMesh();

		runningRenderThread = 1;
		renderThread = DKThread::Create(DKFunction(this, &MaterialDemo::RenderThread)->Invocation());
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
    MaterialDemo app;
	DKPropertySet::SystemConfig().SetValue("AppDelegate", "AppDelegate");
	DKPropertySet::SystemConfig().SetValue("GraphicsAPI", "Vulkan");
	return app.Run();
}
