#include "windows_interface.h"

static int windowsTitleHeight = 0;
static int windowsTitleButtonWidth = 46;

#define IDT_HOVER_MONITOR 2233

static int Win32DpiScale(int value, UINT dpi) {
  return MulDiv(value, dpi, 96);
}

static int GetTitleHeight(HWND hWnd) {
  UINT dpi = GetDpiForWindow(hWnd);
  if (windowsTitleHeight != 0) {
    return Win32DpiScale(windowsTitleHeight, dpi);
  }
  int standardTitleHeight = GetSystemMetrics(SM_CYCAPTION);
  return Win32DpiScale(standardTitleHeight, dpi);
}

static int GetTitleButtonWidth(HWND hWnd) {
  UINT dpi = GetDpiForWindow(hWnd);
  return Win32DpiScale(windowsTitleButtonWidth, dpi);
}

static CustomTitleBarButtonRects GetTitleButtonRects(HWND hWnd) {
  RECT clientRect;
  GetClientRect(hWnd, &clientRect);

  // Calculate button areas
  int titleHeight = GetTitleHeight(hWnd);
  int buttonWidth = GetTitleButtonWidth(hWnd);

  int buttonSpacing = 0;

  CustomTitleBarButtonRects buttonRects {};
  buttonRects.minimize = {
      clientRect.right - 3 * buttonWidth - 2 * buttonSpacing,
      0,
      clientRect.right - 2 * buttonWidth - 2 * buttonSpacing,
      titleHeight
  };
  buttonRects.maximize = {
      clientRect.right - 2 * buttonWidth - buttonSpacing,
      0,
      clientRect.right - buttonWidth - buttonSpacing,
      titleHeight
  };
  buttonRects.close = {
      clientRect.right - buttonWidth,
      0,
      clientRect.right,
      titleHeight
  };
  return buttonRects;
}

// Looks for |key| in |map|, returning the associated value if it is present, or
// a nullptr if not.
static const flutter::EncodableValue* ValueOrNull(const flutter::EncodableMap& map, const char* key) {
  auto it = map.find(flutter::EncodableValue(key));
  if (it == map.end()) {
    return nullptr;
  }
  return &(it->second);
}

// Looks for |key| in |map|, returning the associated int64 value if it is
// present, or std::nullopt if not.
static std::optional<int64_t> GetInt64ValueOrNull(const flutter::EncodableMap& map,
  const char* key) {
  auto value = ValueOrNull(map, key);
  if (!value) {
    return std::nullopt;
  }
  if (std::holds_alternative<int32_t>(*value)) {
    return static_cast<int64_t>(std::get<int32_t>(*value));
  }
  auto val64 = std::get_if<int64_t>(value);
  if (!val64) {
    return std::nullopt;
  }
  return *val64;
}

static const std::string* GetStringValueOrNull(const flutter::EncodableMap& map, const char* key) {
  return std::get_if<std::string>(ValueOrNull(map, key));
}

static const flutter::EncodableMap* GetArgsMap(const flutter::EncodableValue* args) {
  return std::get_if<flutter::EncodableMap>(args);
}

static bool GetMouseDown() {
  return (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
}

static LRESULT CALLBACK WndProcView(HWND const hWnd, UINT const message, WPARAM const wParam, LPARAM const lParam) noexcept {
  if (WindowsInterface::instance) {
    auto customResult = WindowsInterface::instance->HandleWindowProcView(hWnd, message, wParam, lParam);
    if (customResult) {
      return customResult.value();
    }
  }
  if (WindowsInterface::originalWndProc) {
    LRESULT result = CallWindowProc(WindowsInterface::originalWndProc, hWnd, message, wParam, lParam);
    return result;
  }
  return DefWindowProc(hWnd, message, wParam, lParam);
}

void CALLBACK OnTitleButtonTimerProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
  if (WindowsInterface::instance) {
    WindowsInterface::instance->OnTitleButtonTimer(hWnd, uMsg, idEvent, dwTime);
  }
}

WindowsInterface* WindowsInterface::instance = nullptr;
WNDPROC WindowsInterface::originalWndProc = nullptr;
std::unique_ptr<
  flutter::MethodChannel<flutter::EncodableValue>,
  std::default_delete<flutter::MethodChannel<flutter::EncodableValue>>> channel = nullptr;

void WindowsInterface::RegisterPlugin(flutter::PluginRegistry* registry, HWND view) {
  auto registrar = flutter::PluginRegistrarManager::GetInstance()->GetRegistrar<flutter::PluginRegistrarWindows>(
      registry->GetRegistrarForPlugin("WindowsInterface"));

  channel = std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
      registrar->messenger(), "windows_interface",
      &flutter::StandardMethodCodec::GetInstance());

  auto plugin = std::make_unique<WindowsInterface>(registrar, view);

  channel->SetMethodCallHandler(
      [plugin_pointer = plugin.get()](const auto &call, auto result) {
        plugin_pointer->HandleMethodCall(call, std::move(result));
      });

  registrar->AddPlugin(std::move(plugin));
}

WindowsInterface::WindowsInterface(flutter::PluginRegistrarWindows* registrar, HWND view) : registrar(registrar), view(view)  {
  window_proc_id = registrar->RegisterTopLevelWindowProcDelegate(
    [this](HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
      return HandleWindowProc(hWnd, message, wParam, lParam);
    });
  instance = this;
  originalWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(view, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProcView)));
  titleBarHoveredButton = CustomTitleBarHoveredButton_None;
  titleBarDownButton = CustomTitleBarHoveredButton_None;
}

WindowsInterface::~WindowsInterface() {
  registrar->UnregisterTopLevelWindowProcDelegate(window_proc_id);
  channel = nullptr;
}

void WindowsInterface::Invoke(std::string method, std::optional<std::string> params) {
  if (channel == nullptr)
    return;
  flutter::EncodableMap args = flutter::EncodableMap();
  args[flutter::EncodableValue("method")] = flutter::EncodableValue(method);
  if (params.has_value()) {
    args[flutter::EncodableValue("params")] = flutter::EncodableValue(params.value());
  }
  channel->InvokeMethod("invoke", std::make_unique<flutter::EncodableValue>(args));
}

void WindowsInterface::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue> &method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  if (method_call.method_name().compare("releaseMouse") == 0) {
    ReleaseMouse();
    result->Success();
  } else if (method_call.method_name().compare("setWindowsTitleHeight") == 0) {
    const auto* args = GetArgsMap(method_call.arguments());
    auto height = GetInt64ValueOrNull(*args, "height");
    if (height.has_value()) {
      SetWindowsTitleHeight(height.value());
      result->Success();
    } else {
      result->Error("setWindowsTitleHeight(int height) arguments is null");
    }
  } else if (method_call.method_name().compare("setWindowsTitleButtonWidth") == 0) {
    const auto* args = GetArgsMap(method_call.arguments());
    auto width = GetInt64ValueOrNull(*args, "width");
    if (width.has_value()) {
      SetWindowsTitleButtonWidth(width.value());
      result->Success();
    }
    else {
      result->Error("setWindowsTitleButtonWidth(int width) arguments is null");
    }
  } else {
    result->NotImplemented();
  }
}

HWND WindowsInterface::GetMainWindow() {
  return GetActiveWindow();
}

HWND WindowsInterface::GetViewWindow() const {
  return view;
}

CustomTitleBarHoveredButton WindowsInterface::GetTitleButtonStatus(HWND hWnd) {
  POINT cursorPoint;
  GetCursorPos(&cursorPoint);
  ScreenToClient(hWnd, &cursorPoint);

  auto buttonRects = GetTitleButtonRects(hWnd);
  CustomTitleBarHoveredButton newHoveredButton = CustomTitleBarHoveredButton_None;
  if (PtInRect(&buttonRects.close, cursorPoint)) {
    newHoveredButton = CustomTitleBarHoveredButton_Close;
  }
  else if (PtInRect(&buttonRects.minimize, cursorPoint)) {
    newHoveredButton = CustomTitleBarHoveredButton_Minimize;
  }
  else if (PtInRect(&buttonRects.maximize, cursorPoint)) {
    newHoveredButton = CustomTitleBarHoveredButton_Maximize;
  }
  return newHoveredButton;
}

void WindowsInterface::UpdateTitleButtonStatus(HWND hWnd) {
  CustomTitleBarHoveredButton newHoveredButton = GetTitleButtonStatus(hWnd);
  if (newHoveredButton != CustomTitleBarHoveredButton_None) {
    if (newHoveredButton != titleBarHoveredButton) {
      SetTitleButtonHover(hWnd, newHoveredButton);
    }
  }
}

const std::string WindowsInterface::GetTitleBarHoverdButtonTypeStr() {
  if (titleBarHoveredButton == CustomTitleBarHoveredButton_Maximize) {
    return "maximize";
  } else if (titleBarHoveredButton == CustomTitleBarHoveredButton_Minimize) {
    return "minimize";
  } else if (titleBarHoveredButton == CustomTitleBarHoveredButton_Close) {
    return "close";
  } else {
    return "none";
  }
}

void WindowsInterface::SetTitleButtonHover(HWND hWnd, CustomTitleBarHoveredButton targetHover) {
  if (titleBarHoveredButton == targetHover) {
    return;
  }
  titleBarHoveredButton = targetHover;
  OnTitleButtonHovered();

  if (titleBarHoveredButton != CustomTitleBarHoveredButton_None) {
    if (isTitleButtonTimerRunning) {
      KillTimer(hWnd, IDT_HOVER_MONITOR);
    }
    SetTimer(hWnd, IDT_HOVER_MONITOR, 24, OnTitleButtonTimerProc);
    isTitleButtonTimerRunning = true;
  } else {
    KillTimer(hWnd, IDT_HOVER_MONITOR);
    isTitleButtonTimerRunning = false;
  }
}

void WindowsInterface::OnTitleButtonTimer(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
  CustomTitleBarHoveredButton newHoveredButton = GetTitleButtonStatus(hWnd);
  if (newHoveredButton == CustomTitleBarHoveredButton_None) {
    SetTitleButtonHover(hWnd, newHoveredButton);
  }
}

void WindowsInterface::InvalidateTitleButtons(HWND hWnd) {
  auto buttonRects = GetTitleButtonRects(hWnd);
  InvalidateRect(hWnd, &buttonRects.minimize, FALSE);
  InvalidateRect(hWnd, &buttonRects.maximize, FALSE);
  InvalidateRect(hWnd, &buttonRects.close, FALSE);
}

std::optional<LRESULT> WindowsInterface::HandleWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  std::optional<LRESULT> result = std::nullopt;
  return result;
}

std::optional<LRESULT> WindowsInterface::HandleWindowProcView(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  std::optional<LRESULT> result = std::nullopt;
  switch (message)
  {
    case WM_ACTIVATE: {
      InvalidateTitleButtons(hWnd);
      break;
    }
    case WM_SIZE:
    {
      InvalidateTitleButtons(hWnd);
      break;
    }
    case WM_NCHITTEST:
    {
      if (titleBarHoveredButton == CustomTitleBarHoveredButton_Maximize) {
        return HTMAXBUTTON;
      } else if (titleBarHoveredButton == CustomTitleBarHoveredButton_Minimize) {
        return HTMINBUTTON;
      } else if (titleBarHoveredButton == CustomTitleBarHoveredButton_Close) {
        return HTCLOSE;
      }
      return HTCLIENT;
      break;
    }
    case WM_NCMOUSEMOVE: {
      UpdateTitleButtonStatus(hWnd);
      break;
    }
    case WM_MOUSEMOVE: {
      UpdateTitleButtonStatus(hWnd);
      break;
    }
    case WM_NCLBUTTONDOWN: {
      if (titleBarHoveredButton != CustomTitleBarHoveredButton_None) {
        OnTitleButtonDown();
        titleBarDownButton = titleBarHoveredButton;
        return HTNOWHERE;
      }
      break;
    }
    case WM_NCLBUTTONUP:
    {
      if (titleBarHoveredButton != CustomTitleBarHoveredButton_None) {
        OnTitleButtonUp();
        if (titleBarHoveredButton == titleBarDownButton) {
          OnTitleButtonClick();
        }
        titleBarDownButton = CustomTitleBarHoveredButton_None;
      }
      break;
    }
  }
  return result;
}

void WindowsInterface::ReleaseMouse() {
  HWND hwnd = GetMainWindow();
  ReleaseCapture();
  SendMessage(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(0, 0));
}

void WindowsInterface::SetWindowsTitleHeight(int64_t height) {
  windowsTitleHeight = static_cast<int32_t>(height);
}

void WindowsInterface::SetWindowsTitleButtonWidth(int64_t width) {
  windowsTitleButtonWidth = static_cast<int32_t>(width);
}

void WindowsInterface::OnTitleButtonHovered() {
  if (titleBarHoveredButton != CustomTitleBarHoveredButton_None) {
    SetCursor(LoadCursor(NULL, IDC_ARROW));
  }
  auto type = GetTitleBarHoverdButtonTypeStr();
  Invoke("onTitleButtonHoverd", type);
}

void WindowsInterface::OnTitleButtonDown() {
  auto type = GetTitleBarHoverdButtonTypeStr();
  Invoke("onTitleButtonDown", type);
}

void WindowsInterface::OnTitleButtonUp() {
  auto type = GetTitleBarHoverdButtonTypeStr();
  Invoke("onTitleButtonUp", type);
}

void WindowsInterface::OnTitleButtonClick() {
  auto type = GetTitleBarHoverdButtonTypeStr();
  Invoke("onTitleButtonClick", type);
}
