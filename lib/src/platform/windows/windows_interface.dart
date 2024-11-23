import 'dart:developer';
import 'dart:io';

import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';

const int windowsTitleHeight = 32;
const int windowsTitleButtonWidth = 46;

abstract mixin class WindowsInterfaceListener {
  /// title button hoverd
  void onTitleButtonHoverd(String type) {}

  /// title button down
  void onTitleButtonDown(String type) {}

  /// title button up
  void onTitleButtonUp(String type) {}

  /// title button click
  void onTitleButtonClick(String type) {}
}

class WindowsInterface {
  WindowsInterface._();
  static final WindowsInterface instance = WindowsInterface._();

  final MethodChannel _channel = const MethodChannel('windows_interface');

  final ObserverList<WindowsInterfaceListener> _listeners =
      ObserverList<WindowsInterfaceListener>();

  Future<void> _callListeners(dynamic call) async {
    final String method = call['method'];
    final String params = call['params'];
    // log("method: $method, params: $params");
    for (final WindowsInterfaceListener listener in listeners) {
      if (!_listeners.contains(listener)) {
        return;
      }
      Map<String, Function> funcMap = {
        'onTitleButtonHoverd': listener.onTitleButtonHoverd,
        'onTitleButtonDown': listener.onTitleButtonDown,
        'onTitleButtonUp': listener.onTitleButtonUp,
        'onTitleButtonClick': listener.onTitleButtonClick,
      };
      funcMap[method]?.call(params);
    }
  }

  List<WindowsInterfaceListener> get listeners {
    final List<WindowsInterfaceListener> localListeners =
        List<WindowsInterfaceListener>.from(_listeners);
    return localListeners;
  }

  bool get hasListeners {
    return _listeners.isNotEmpty;
  }

  void addListener(WindowsInterfaceListener listener) {
    _listeners.add(listener);
  }

  void removeListener(WindowsInterfaceListener listener) {
    _listeners.remove(listener);
  }

  Future<T?> invokeMethod<T>(String method, [dynamic arguments]) async {
    try {
      if (Platform.isWindows) {
        return await _channel.invokeMethod<T>(method, arguments);
      }
    } catch (e) {
      log(e.toString());
    }
    return null;
  }

  /// 释放鼠标
  Future<void> releaseMouse() async {
    await invokeMethod('releaseMouse');
  }

  /// 设置windows的title高度
  Future<void> setWindowsTitleHeight(int height) async {
    await invokeMethod('setWindowsTitleHeight', {'height': height});
  }

  /// 设置windows的title的button宽度
  Future<void> setWindowsTitleButtonWidth(int width) async {
    await invokeMethod('setWindowsTitleButtonWidth', {'width': width});
  }

  Future<void> setupMethodChannel() async {
    _channel.setMethodCallHandler((call) async {
      if (call.method == 'invoke') {
        final args = call.arguments;
        return _callListeners(args);
      }
    });
    await setWindowsTitleHeight(windowsTitleHeight);
    await setWindowsTitleButtonWidth(windowsTitleButtonWidth);
  }
}

final windowsInterface = WindowsInterface.instance;
