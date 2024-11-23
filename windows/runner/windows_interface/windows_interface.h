#ifndef FLUTTER_PLUGIN_WINDOWS_INTERFACE_H_
#define FLUTTER_PLUGIN_WINDOWS_INTERFACE_H_

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>
#include <flutter/plugin_registry.h>
#include <Windows.h>

typedef enum {
  CustomTitleBarHoveredButton_None,
  CustomTitleBarHoveredButton_Minimize,
  CustomTitleBarHoveredButton_Maximize,
  CustomTitleBarHoveredButton_Close,
} CustomTitleBarHoveredButton;

typedef struct {
  RECT close;
  RECT maximize;
  RECT minimize;
} CustomTitleBarButtonRects;

class WindowsInterface : public flutter::Plugin {
 public:
  static WindowsInterface* instance;
  static WNDPROC originalWndProc;

  static void RegisterPlugin(flutter::PluginRegistry* registry, HWND view);

  WindowsInterface(flutter::PluginRegistrarWindows* registrar, HWND view);
  virtual ~WindowsInterface();

  std::optional<LRESULT> HandleWindowProcView(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
  void OnTitleButtonTimer(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

 private:
  flutter::PluginRegistrarWindows* registrar;
  HWND view;
  int window_proc_id = -1;
  bool isTitleButtonTimerRunning = false;

  CustomTitleBarHoveredButton titleBarHoveredButton;
  CustomTitleBarHoveredButton titleBarDownButton;

  void HandleMethodCall(const flutter::MethodCall<flutter::EncodableValue>& method_call, std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
  void Invoke(std::string method, std::optional<std::string> params = nullptr);
  HWND GetMainWindow();
  HWND GetViewWindow() const;

  std::optional<LRESULT> HandleWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
  CustomTitleBarHoveredButton GetTitleButtonStatus(HWND hWnd);
  void UpdateTitleButtonStatus(HWND hWnd);
  void InvalidateTitleButtons(HWND hWnd);

  const std::string WindowsInterface::GetTitleBarHoverdButtonTypeStr();
  void SetTitleButtonHover(HWND hWnd, CustomTitleBarHoveredButton targetHover);

  void ReleaseMouse();
  void SetWindowsTitleHeight(int64_t height);
  void SetWindowsTitleButtonWidth(int64_t width);
  void OnTitleButtonHovered();
  void OnTitleButtonDown();
  void OnTitleButtonUp();
  void OnTitleButtonClick();
};

#endif  // FLUTTER_PLUGIN_WINDOWS_INTERFACE_H_
