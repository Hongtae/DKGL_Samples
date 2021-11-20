#include <cstddef>
#include "app.h"
#include "util.h"

class MainFrame : public DKFrame
{
    DKObject<DKFont> font;
    DKObject<DKFont> fontOutline;
    float frameDelta;
public:
    void OnDraw(DKCanvas* canvas) const override
    {
        canvas->Clear(DKColor(0.1, 0.1, 0.5));
        canvas->DrawRect(DKRect(10, 10, 100, 50), DKMatrix3::identity, DKColor(1, 1, 0), DKBlendState::defaultAlpha);
        canvas->DrawEllipseOutline(DKRect(50, 50, 120, 120), DKMatrix3::identity, DKColor(0, 0, 1), DKBlendState::defaultAlpha, DKSize(10, 10));

        DKPoint line[] = { DKPoint(10, 150), DKPoint(450, 150) };
        canvas->DrawLines(line, 2, DKColor(0, 0, 0), DKBlendState::defaultOpaque, 4.0);
        canvas->DrawText(line[0], line[1], "Lorem ipsum dolor sit amet", fontOutline, DKColor(0, 0, 0));
        canvas->DrawText(line[0], line[1], "Lorem ipsum dolor sit amet", font, DKColor(1, 1, 1));

        if (frameDelta > 0.0)
        {
            DKString fps = DKString::Format("%.1f fps (%.4fs)", 1.0 / frameDelta, frameDelta);
            auto width = font->LineWidth(fps);
            canvas->DrawText(DKPoint(0, 200), DKPoint(width, 200), fps, font, DKColor(1, 1, 1));
        }
    }

    void OnDrawOverlay(DKCanvas* canvas) const override
    {
    }
    
    void OnUpdate(double delta, DKTimeTick tick, DKDateTime date) override
    {
        frameDelta = delta;
        SetRedraw();
    }
    
    void OnLoaded() override
    {
        DKResourcePool& resourcePool = ((SampleApp*)DKApplication::Instance())->resourcePool;
        DKObject<DKData> fontData = resourcePool.LoadResourceData("fonts/NanumGothic.ttf");
        if (fontData)
        {
            float pt = 14.0;
            int dpi = 144;

            font = DKFont::Create(fontData, Screen()->GraphicsDevice());
            if (font)
                font->SetStyle(pt, dpi, dpi);
            fontOutline = DKFont::Create(fontData, Screen()->GraphicsDevice());
            if (fontOutline)
                fontOutline->SetStyle(pt, dpi, dpi, 0, 2, true, true);
        }
    }

    void OnUnload() override
    {
    }
    
    void OnContentResized() override 
    {
        DKSize res = this->ContentResolution();
        float scale = this->Screen()->ContentScaleFactor();
        this->SetContentScale(res / scale);
    }
};

class CanvasDemo : public SampleApp
{
    DKObject<MainFrame> mainFrame;
    DKObject<DKScreen> screen;
    DKObject<DKWindow> window;

public:
	void OnInitialize(void) override
	{
        SampleApp::OnInitialize();
		DKLogD("%s", DKGL_FUNCTION_NAME);

        mainFrame = DKOBJECT_NEW MainFrame();

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

        screen = DKOBJECT_NEW DKScreen();
        screen->SetRootFrame(mainFrame);
        screen->SetWindow(window);
	}
	void OnTerminate(void) override
	{
		DKLogD("%s", DKGL_FUNCTION_NAME);

        screen = nullptr;
        window = nullptr;
        mainFrame = nullptr;

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
