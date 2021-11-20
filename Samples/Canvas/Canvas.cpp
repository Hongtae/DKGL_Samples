#include <cstddef>
#include "app.h"
#include "util.h"



class CanvasDemo : public SampleApp
{
    DKObject<DKWindow> window;
	DKObject<DKThread> renderThread;
	DKAtomicNumber32 runningRenderThread;
    DKObject<DKCanvas> canvas;

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
        auto compressFile = [](const wchar_t* spvPath)
        {
            if (spvPath && spvPath[0])
            {
                DKString file = spvPath;
                DKString enc = file + ".enc";
                DKString dec = enc + ".dec";

                DKLogI("Compressing %ls...", (const wchar_t*)file);

                DKObject<DKFile> f1 = DKFile::Create(file, DKFile::ModeOpenExisting, DKFile::ModeShareRead);
                DKObject<DKFile> f2 = DKFile::Create(enc, DKFile::ModeOpenNew, DKFile::ModeShareExclusive);
                DKObject<DKFile> f3 = DKFile::Create(dec, DKFile::ModeOpenNew, DKFile::ModeShareExclusive);

                DKCompressor(DKCompressor::BestRatio).Compress(f1, f2);
                f2->SetCurrentPosition(0);
                DKObject<DKBuffer> b64buff = f2->Read(f2->RemainLength());

                f1->SetCurrentPosition(0);
                f2->SetCurrentPosition(0);

                DKStringU8 base64str;
                b64buff->Base64Encode(base64str);
                DKLog("CompressedSize: %llu -> %llu (base64 bytes:%llu)",
                      f1->TotalLength(),
                      f2->TotalLength(),
                      base64str.Length());

                DKLogI("base64: \n%s\n", (const char*)base64str);

                //DKBuffer b64Buffer;
                //DKDataStream b64Stream(b64Buffer);
                //(f2->* (size_t(DKFile::*)(DKStream*, size_t) const)&DKFile::Read)   ((DKStream*)&b64Stream, (size_t)f2->RemainLength());

                DKCompressor().Decompress(f2, f3);

                for (DKFile* f : { f1.Ptr(), f2.Ptr(), f3.Ptr() })
                {
                    DKString path = f->Path();
                    f->SetCurrentPosition(0);
                    size_t s = f->TotalLength();
                    DKHashResultSHA1 sha1;
                    if (DKHashSHA1(f, sha1))
                    {
                        DKLog("SHA1(%ls):\n --> %ls (%llu bytes)",
                              (const wchar_t*)path, (const wchar_t*)sha1.String(), s);
                    }
                    else
                        DKLog("SHA1(%ls) failed!", (const wchar_t*)path);
                }
            }
        };
        compressFile(resourcePool.ResourceFilePath("canvas/canvas.vert.spv"));
        compressFile(resourcePool.ResourceFilePath("canvas/varying_color.frag.spv"));
        compressFile(resourcePool.ResourceFilePath("canvas/varying_color_texture.frag.spv"));
        compressFile(resourcePool.ResourceFilePath("canvas/solid_ellipse.frag.spv"));
        compressFile(resourcePool.ResourceFilePath("canvas/solid_ellipse_inner_hole.frag.spv"));
        compressFile(resourcePool.ResourceFilePath("canvas/texture_ellipse.frag.spv"));
        compressFile(resourcePool.ResourceFilePath("canvas/alpha_texture.frag.spv"));

        DKObject<DKGraphicsDevice> device = DKGraphicsDevice::SharedInstance();
        DKObject<DKGraphicsDeviceContext> deviceContext = DKOBJECT_NEW DKGraphicsDeviceContext(device);
        DKObject<DKCommandQueue> queue = device->CreateCommandQueue(DKCommandQueue::Graphics);

        DKCanvas::CachePipelineContext(deviceContext);

		DKObject<DKData> vertData = resourcePool.LoadResourceData("canvas/canvas.vert.spv");
		DKObject<DKData> fragData = resourcePool.LoadResourceData("canvas/varying_color.frag.spv");
        DKObject<DKShader> vertShader = DKOBJECT_NEW DKShader(vertData);
        DKObject<DKShader> fragShader = DKOBJECT_NEW DKShader(fragData);


		DKObject<DKSwapChain> swapChain = queue->CreateSwapChain(window);


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
        DKObject<DKTexture> texture = LoadTexture2D(queue, resourcePool.LoadResourceData("textures/deathstar3.png"));

        DKObject<DKTexture> depthBuffer = nullptr;

        DKCamera camera;
        DKVector3 cameraPosition = { 0, 5, 10 };
        DKVector3 cameraTartget = { 0, 0, 0 };

        DKAffineTransform3 tm(DKLinearTransform3().Scale(5).Rotate(DKVector3(-1, 0, 0), DKGL_PI * 0.5));

        DKTimer timer;
        timer.Reset();

        DKLog("Render thread begin");
        while (!runningRenderThread.CompareAndSet(0, 0))
        {
            DKRenderPassDescriptor rpd = swapChain->CurrentRenderPassDescriptor();
            double t = timer.Elapsed();
            double waveT = (cos(t) + 1.0) * 0.5;

            DKTexture* renderTarget = rpd.colorAttachments.Value(0).renderTarget;
            int width = renderTarget->Width();
            int height = renderTarget->Height();
            const DKRect viewport(0, 0, width, height);
            if (true)
            {
                DKObject<DKCanvas> canvas = DKOBJECT_NEW DKCanvas(queue->CreateCommandBuffer(),
                                                                  renderTarget);
                canvas->SetViewport(viewport);
                canvas->Clear(DKColor(0.0, 0.0, waveT, 0.0));
                canvas->DrawRect(DKRect(0.0, 0.0, 1.0, 1.0), DKMatrix3::identity, DKColor(0, 1, 0, 0.5), DKBlendState::defaultOpaque);
                //canvas->DrawRect(DKRect(0.25, 0.25, 0.5, 0.5), DKMatrix3::identity, DKColor(1, 1, 1, 1), DKBlendState::defaultOpaque);

                DKCanvas::TexturedVertex verts[4] = {
                    { DKPoint(0.25, 0.25), DKPoint(0, 0), DKColor(1,1,1,1)},
                    { DKPoint(0.25, 0.75), DKPoint(0, 1), DKColor(1,1,1,1)},
                    { DKPoint(0.75, 0.25), DKPoint(1, 0), DKColor(1,1,1,1)},
                    { DKPoint(0.75, 0.75), DKPoint(1, 1), DKColor(1,1,1,1)},
                };
                canvas->DrawTriangleStrip(verts, 4, texture, DKBlendState::defaultOpaque);
                //canvas->DrawRect(DKRect(0.25, 0.25, 0.5, 0.5), DKMatrix3::identity, DKColor(1, 1, 1, 1), DKBlendState::defaultOpaque);
                canvas->Commit();
            }
            if (true)
            {
                DKObject<DKCanvas> canvas = DKOBJECT_NEW DKCanvas(queue->CreateCommandBuffer(),
                                                                  renderTarget);
                canvas->SetViewport(viewport);
                canvas->DrawEllipseOutline(DKRect(0.0, 0.0, 0.5, 0.5), DKMatrix3::identity, DKColor(1, 0, 0, 1),
                                           DKBlendState::defaultOpaque,
                                           DKSize(0.1, 0.1));
                canvas->Commit();
            }

            swapChain->Present();
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
		renderThread = DKThread::Create(DKFunction(this, &CanvasDemo::RenderThread)->Invocation());
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
    CanvasDemo app;
	DKPropertySet::SystemConfig().SetValue("AppDelegate", "AppDelegate");
	DKPropertySet::SystemConfig().SetValue("GraphicsAPI", "Vulkan");
	return app.Run();
}
