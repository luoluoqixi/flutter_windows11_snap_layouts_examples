import 'dart:io';

import 'package:flutter/material.dart';

import 'package:window_manager/window_manager.dart';

import '../../platform/windows/windows_interface.dart';
import 'window_caption_button.dart';

final double windowCaptionHeight =
    Platform.isMacOS ? 28 : windowsTitleHeight.toDouble();

class FixDragToMoveArea extends StatefulWidget {
  const FixDragToMoveArea({
    super.key,
    required this.child,
  });

  final Widget child;

  @override
  State<StatefulWidget> createState() => _FixDragToMoveArea();
}

class _FixDragToMoveArea extends State<FixDragToMoveArea> {
  bool _isDoubleTapped = false;
  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      behavior: HitTestBehavior.translucent,
      onPanDown: (details) {
        _isDoubleTapped = false;
      },
      onPanStart: (details) {
        if (_isDoubleTapped) {
          // 如果是双击后触发的拖拽事件，阻止它
          return;
        }
        windowManager.startDragging();
      },
      onDoubleTapDown: (details) async {
        if (Platform.isWindows) {
          // windows平台鼠标按下时maximize不生效, 强行释放一下鼠标
          await windowsInterface.releaseMouse();
        }
        bool isMaximized = await windowManager.isMaximized();
        if (!isMaximized) {
          await windowManager.maximize();
        } else {
          await windowManager.unmaximize();
        }
        _isDoubleTapped = true;
      },
      child: widget.child,
    );
  }
}

class FixWindowCaption extends StatefulWidget {
  const FixWindowCaption({
    super.key,
    this.title,
    this.backgroundColor,
    this.brightness,
    this.isFullScreen,
  });

  final Widget? title;
  final Color? backgroundColor;
  final Brightness? brightness;
  final bool? isFullScreen;

  @override
  State<FixWindowCaption> createState() => _FixWindowCaptionState();
}

class _FixWindowCaptionState extends State<FixWindowCaption>
    with WindowListener {
  @override
  void initState() {
    windowManager.addListener(this);
    super.initState();
  }

  @override
  void dispose() {
    windowManager.removeListener(this);
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final showBtn =
        (Platform.isWindows || Platform.isLinux) && widget.isFullScreen != true;
    return DecoratedBox(
      decoration: BoxDecoration(
        color: widget.backgroundColor ??
            (widget.brightness == Brightness.dark
                ? const Color(0xff1C1C1C)
                : Colors.transparent),
      ),
      child: Row(
        children: [
          Expanded(
            child: FixDragToMoveArea(
              child: SizedBox(
                height: double.infinity,
                child: Row(
                  children: [
                    Container(
                      padding: const EdgeInsets.only(left: 16),
                      child: DefaultTextStyle(
                        style: TextStyle(
                          color: widget.brightness == Brightness.light
                              ? Colors.black.withOpacity(0.8956)
                              : Colors.white,
                          fontSize: 14,
                        ),
                        child: widget.title ?? Container(),
                      ),
                    ),
                  ],
                ),
              ),
            ),
          ),
          if (showBtn)
            FixWindowCaptionButton(
              brightness: widget.brightness,
              type: WindowButtonType.minimize,
            ),
          if (showBtn)
            FixWindowCaptionButton(
              brightness: widget.brightness,
              type: WindowButtonType.maximize,
            ),
          if (showBtn)
            FixWindowCaptionButton(
              brightness: widget.brightness,
              type: WindowButtonType.close,
            ),
        ],
      ),
    );
  }

  @override
  void onWindowMaximize() {
    setState(() {});
  }

  @override
  void onWindowUnmaximize() {
    setState(() {});
  }
}
