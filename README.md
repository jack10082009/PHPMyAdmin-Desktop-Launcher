# PHPMyAdmin-Desktop-Launcher
一个封装好了的PHPMyAdmin桌面启动器。A PHPMyAdmin desktop launcher.
---
PHPMyAdmin是一个非常好用的MySQL数据库管理软件，但是其运行需要web环境，临时搭建运行环境较为繁琐，故编写此启动器，直接在Windows平台尽情使用您熟悉的PHPMyAdmin！

使用Dev cpp构建。链接时需要参数`-mwindows`。使用`MinGW-w64 GCC 11.4.0 64-bit Release`。

如欲用于32位系统，请自行下载32位PHP软件包，并自行使用32位编译器编译main.cpp。

PHPMyAdmin is a very easy-to-use MySQL database management software, but it requires a web environment to run. It is rather cumbersome to temporarily build a running environment, so this launcher is written to allow you to use the familiar PHPMyAdmin directly on the Windows platform!

Built with Dev cpp. Requires `-mwindows` when linking. Use `MinGW-w64 GCC 11.4.0 64-bit Release`.

If you want to use it on a 32-bit system, please download the 32-bit PHP package and compile main.cpp using a 32-bit compiler.

## 使用 Using

将PHP-Windows的文件放入编译好的main.exe同目录下的php目录中；将PHPMyAdmin的文件放入main.exe同目录下的web文件夹中。双击main.exe，即可使用。

注意需要配置`php.ini`以及`config.inc.php`。如果您不知道这是什么，请您直接下载Release。

Put the PHP-Windows file into the php directory in the same directory as the compiled main.exe; put the PHPMyAdmin file into the web folder in the same directory as the main.exe. Double-click main.exe to use it.

Note that you need to configure `php.ini` and `config.inc.php`. If you don't know what these are, please download the Release directly.

## 感谢 Thanks

本项目使用了以下开源项目，感谢它们的贡献：

This project uses the following open source projects, thanks for their contributions:

- [PHP](https://github.com/php/php-src)
- [PHPMyAdmin](https://github.com/phpmyadmin/phpmyadmin)
- Dev cpp
- MinGW
