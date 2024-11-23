import 'dart:io';

import 'package:flutter/material.dart';
import 'package:window_manager/window_manager.dart';

import 'src/app.dart';
import 'src/platform/windows/windows_interface.dart';

Future<void> main() async {
  await startupWindow();
  await startupChannel();
  runApp(const MyApp());
}

Future<void> startupWindow() async {
  WidgetsFlutterBinding.ensureInitialized();
  if (Platform.isWindows || Platform.isLinux || Platform.isMacOS) {
    const windowSize = Size(1000, 600);
    const minSize = Size(300, 100);
    await windowManager.ensureInitialized();
    WindowManager.instance.setMinimumSize(minSize);
    WindowOptions windowOptions = WindowOptions(
      size: windowSize,
      center: true,
      backgroundColor: Colors.transparent,
      skipTaskbar: false,
      // hide system title bar
      titleBarStyle: TitleBarStyle.hidden,
      windowButtonVisibility: Platform.isMacOS,
    );
    windowManager.waitUntilReadyToShow(windowOptions, () async {
      await windowManager.show();
      await windowManager.focus();
    });
  }
}

Future<void> startupChannel() async {
  await windowsInterface.setupMethodChannel();
}
