#include "BenchmarkingApplication.h"
#include "DefaultApplication.h"
#include "Macros.h"
#include "ModelInstance.h"
#include "RenderingEngineD3D12.h"
#include "SimulationEngine.h"

bool g_running = false;

LRESULT CALLBACK WndProc(HWND hWnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam);

bool InitializeWindow(HINSTANCE hInstance,
	int ShowWnd,
	RenderWindow& renderWindow)
{
	if (renderWindow.FullScreen)
	{
		HMONITOR hmon = MonitorFromWindow(renderWindow.WindowHandle,
			MONITOR_DEFAULTTONEAREST);
		MONITORINFO mi = { sizeof(mi) };
		GetMonitorInfo(hmon, &mi);

		/*Width = mi.rcMonitor.right - mi.rcMonitor.left;
		Height = mi.rcMonitor.bottom - mi.rcMonitor.top;*/
	}

	WNDCLASSEX wc;

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = NULL;
	wc.cbWndExtra = NULL;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = renderWindow.WindowName;
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wc))
	{
		MessageBoxA(NULL, "Error registering class",
			"Error", MB_OK | MB_ICONERROR);
		return false;
	}

	renderWindow.WindowHandle = CreateWindowEx(NULL,
		renderWindow.WindowName,
		renderWindow.WindowTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		renderWindow.Width,
		renderWindow.Height,
		NULL,
		NULL,
		hInstance,
		NULL);

	if (!renderWindow.WindowHandle)
	{
		MessageBoxA(NULL, "Error creating window",
			"Error", MB_OK | MB_ICONERROR);
		return false;
	}

	if (renderWindow.FullScreen)
	{
		SetWindowLong(renderWindow.WindowHandle, GWL_STYLE, 0);
	}

	ShowWindow(renderWindow.WindowHandle, ShowWnd);
	UpdateWindow(renderWindow.WindowHandle);

	return true;
}

LRESULT CALLBACK WndProc(HWND hwnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam)
{
	switch (msg)
	{
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE) {
			if (MessageBoxA(0, "Are you sure you want to exit?",
				"Really?", MB_YESNO | MB_ICONQUESTION) == IDYES)
			{
				g_running = false;
				DestroyWindow(hwnd);
			}
		}
		return 0;

	case WM_DESTROY: // x button on top right corner of window was pressed
		g_running = false;
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd,
		msg,
		wParam,
		lParam);
}

void InitConsole()
{
	AllocConsole();

	FILE* f;
	freopen_s(&f, "CONOUT$", "w", stdout);
	freopen_s(&f, "CONOUT$", "w", stderr);
	freopen_s(&f, "CONIN$", "r", stdin);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nShowCmd)
{
	InitConsole();

	// Parse command line arguments to see if we are benchmarking (--benchmark)
	int argc = 0;
	LPWSTR* argv = CommandLineToArgvW(lpCmdLine, &argc);
	if (argv == nullptr) { return false; }

	std::vector<std::wstring> arguments(argc);
	for (int i = 0; i < argc; ++i) { arguments[i] = argv[i]; }

	bool isBenchmarking = (arguments[0] == L"--benchmark");
	LocalFree(argv);
	// ----------------------------------------------------------------------------------------------------------
	// Initialization -----------------------------------------------------------------------------------------
	HWND hwnd = NULL;
	RenderWindow renderWindow{ L"BouncyRoomDX12", L"Bounce Room Direct3D12", 1920, 1080, false, hwnd };
	std::unique_ptr<IApplication> application;
	std::unique_ptr<ISimulationEngine> simulationEngine;
	std::unique_ptr<IRenderingEngine> renderingEngine;
	std::shared_ptr<Benchmarker> benchmarker;

	if (isBenchmarking)
	{
		constexpr int stabilizationFrameCount = 300;
		constexpr int measurementFrameCount = 300;

		std::cout << "----------------- BENCHMARKING ENABLED ------------------------";
		benchmarker = std::make_shared<Benchmarker>(stabilizationFrameCount, measurementFrameCount);
		application = std::make_unique<BenchmarkingApplication>(benchmarker);
		simulationEngine = std::make_unique<SimulationEngine>(benchmarker);
		renderingEngine = std::make_unique<RenderingEngineD3D12>(renderWindow, benchmarker);
	}
	else
	{
		std::cout << "----------------- BENCHMARKING DISABLED ------------------------";
		application = std::make_unique<DefaultApplication>();
		simulationEngine = std::make_unique<SimulationEngine>();
		renderingEngine = std::make_unique<RenderingEngineD3D12>(renderWindow);
	}


	Scene mainScene{ renderWindow };

	if (!InitializeWindow(hInstance, nShowCmd, renderWindow))
	{
		MessageBoxA(0, "Window Initialization - Failed",
			"Error", MB_OK);
		return 1;
	}

	if (!application->Init(mainScene))
	{
		MessageBoxA(0, "Failed to initialize Benchmarker",
			"Error", MB_OK);
		g_running = false;
		application->Shutdown();
		return 1;
	}

	if (!simulationEngine->Init(mainScene))
	{
		MessageBoxA(0, "Failed to initialize Simulation Engine",
			"Error", MB_OK);
		g_running = false;
		simulationEngine->Shutdown();
		return 1;
	}

	if (!renderingEngine->Init(mainScene))
	{
		MessageBoxA(0, "Failed to initialize Rendering Engine",
			"Error", MB_OK);
		g_running = false;
		renderingEngine->Shutdown();
		return 1;
	}

	// -------------------------------------------------------------------------------------------------------------
	// Main Loop ---------------------------------------------------------------------------------------------------
	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));

	g_running = true;
	while (g_running)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			//PIXBeginEvent(PIX_COLOR(255, 0, 0), "ApplicationUpdate");
			BREAK_IF_FAIL(application->Update(mainScene), "\nApplication triggered shutdown...");
			//PIXEndEvent();

			//PIXBeginEvent(PIX_COLOR(0, 255, 0), "SimulationUpdate");
			BREAK_IF_FAIL(simulationEngine->Update(mainScene), "\nSimulationEngine triggered shutdown...");
			//PIXEndEvent();

			// PIXBeginEvent(PIX_COLOR(0, 0, 255), "Rendering");
			BREAK_IF_FAIL(renderingEngine->Render(mainScene), "\nRenderingEngine triggered shutdown...");
			// PIXEndEvent();
		}
	}

	renderingEngine->Shutdown();
	simulationEngine->Shutdown();
	application->Shutdown();
	//mainScene.ClearScene();

	std::cout << "\nPress Enter to exit...";
	std::cin.get();
	std::cin.get();

	std::cout << "\nShutting Down...";

	return 0;
}