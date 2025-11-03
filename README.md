# PHPMyAdmin-Desktop-Launcher
一个封装好了的PHPMyAdmin桌面启动器。A PHPMyAdmin desktop launcher.
---
PHPMyAdmin是一个非常好用的MySQL数据库管理软件，但是其运行需要web环境，临时搭建运行环境较为繁琐，故编写此启动器，直接在Windows平台尽情使用您熟悉的PHPMyAdmin！

使用Dev cpp构建。使用`MinGW-w64 GCC 11.4.0 64-bit Release`。

如欲用于32位系统，请自行下载32位PHP软件包，并自行使用32位编译器编译main.cpp。

PHPMyAdmin is a very easy-to-use MySQL database management software, but it requires a web environment to run. It is rather cumbersome to temporarily build a running environment, so this launcher is written to allow you to use the familiar PHPMyAdmin directly on the Windows platform!

Built with Dev cpp. Use `MinGW-w64 GCC 11.4.0 64-bit Release`.

If you want to use it on a 32-bit system, please download the 32-bit PHP package and compile main.cpp using a 32-bit compiler.

## 使用 Using

将PHP-Windows的文件放入编译好的main.exe同目录下的php目录中；将PHPMyAdmin的文件放入main.exe同目录下的web文件夹中。双击main.exe，即可使用。

注意需要配置`php.ini`以及`config.inc.php`。如果您不知道这是什么，请您直接下载Release。

支持的启动参数：`-option={{option_No}}`/`-mini`。

其中：option_No是数字1-8，分别对应下面界面中第一行开始的选项。例如，使用参数`-option=1 -mini`即可最小化打开程序并自动使用Chrome-APP模式打开界面。

最小化后程序不会显示在任务栏中，请在系统托盘处找到程序。可以右键打开界面或直接退出。当然双击也能调出界面。

Put the PHP-Windows file into the php directory in the same directory as the compiled main.exe; put the PHPMyAdmin file into the web folder in the same directory as the main.exe. Double-click main.exe to use it.

Note that you need to configure `php.ini` and `config.inc.php`. If you don't know what these are, please download the Release directly.

Supported launch parameters: `-option={{option_No}}`/`-mini`.

Where: option_No is a number from 1-8, corresponding to the options starting from the first line in the interface below. For example, using the parameter `-option=1 -mini` will minimize the program and automatically open the interface in Chrome-APP mode.

After minimization, the program will not be displayed in the taskbar, please find the program in the system tray. You can right-click to open the interface or exit directly. Of course, double-clicking can also bring up the interface.

## 编译 Compilation

注意：在编译之前需要先`windres resource.rc -O coff -o resource.res`编译资源文件。

编译时加入下列选项：`-mwindows`

链接时使用：`-lgdi32 -lshell32 -static resource.res`

Note: Before compiling, you need to compile the resource file first with `windres resource.rc -O coff -o resource.res`.

Add the following option when compiling: `-mwindows`

Use the following when linking: `-lgdi32 -lshell32 -static resource.res`

## 感谢 Thanks

本项目使用了以下开源项目，感谢它们的贡献：

This project uses the following open source projects, thanks for their contributions:

- [PHP](https://github.com/php/php-src)
- [PHPMyAdmin](https://github.com/phpmyadmin/phpmyadmin)
- Dev cpp
- MinGW
