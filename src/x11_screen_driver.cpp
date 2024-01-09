#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <malloc.h>

int main_()
{
    Display *display;
    Window root_window;
    GC gc;
    XImage *ximage;

    int screen;
    unsigned char *data; // 这里假设您有RGB888格式的图像数据

    int width = 640;  // 图像宽度
    int height = 480; // 图像高度

    // 初始化Xlib
    display = XOpenDisplay(NULL);
    if (!display)
    {
        printf("Unable to open X display.\n");
        return 1;
    }
    screen = DefaultScreen(display);
    root_window = RootWindow(display, screen);

    // 创建图形上下文
    gc = XCreateGC(display, root_window, 0, NULL);

    // 创建XImage对象并设置数据
    data = (unsigned char *)malloc(width * height * 3); // 3 bytes per pixel (RGB888)
    // 在这里将RGB888格式的图像数据填充到"data"数组中

    ximage = XCreateImage(display, DefaultVisual(display, screen),
                          24, ZPixmap, 0, (char *)data, width, height, 32, 0);

    // 创建X11窗口
    Window window = XCreateSimpleWindow(display, root_window, 0, 0, width, height, 0,
                                        BlackPixel(display, screen), WhitePixel(display, screen));
    XMapWindow(display, window);

    // 设置XImage数据到窗口上
    XPutImage(display, window, gc, ximage, 0, 0, 0, 0, width, height);
    XFlush(display);

    // 等待用户关闭窗口
    XEvent event;
    while (1)
    {
        XNextEvent(display, &event);
        if (event.type == DestroyNotify)
        {
            break;
        }
    }

    // 清理资源
    XDestroyWindow(display, window);
    XFreeGC(display, gc);
    XCloseDisplay(display);
    free(data);

    return 0;
}
