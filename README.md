Laucher
=================

Launcher(com.kos.launcher)是使用微软RDP协议的开源Android远程桌面服务。但即使是Android，权限等原因，也不能安装在市面Android手机和平板，需要能自编译内核。它专注于局域网，所有都是直连，使用场景集中在中、高端Android设备。

- 专用设备运行时，像门禁，不带或不方便用监视器，但会联网，这时可用Launcher设置、查看设备参数。
- 家用智能设备，不带或不方便用监视器，用Launcher可代替屏幕，并对各种设备提供统一操作界面。


如何使用、功能模块、定制Android等内容参考“Launcher”: <https://www.cswamp.com/post/23>

## 编译
Launcher基于Rose SDK，此处存放的是launcher私有包。为此需先下载Rose：<https://github.com/freeors/Rose>。运行studio，在左上角鼠标右键，弹出菜单“导入...”，选择launcher-res。

## 各版本要求的最低libkosapi.so版本  
0.0.1-20200706(libkosapi-0.0.1-20200705)  
1.0.0-20200830(libkosapi-1.0.0-20200827)
1.0.1-20201018(libkosapi-1.0.1-20201011

## 开源协议及开源库
Launcher自个源码的开源协议是BSD。当中用到不少开源软件，主要有FreeRDP、SDL、Chromium和BoringSSL，它们有在用不是BSD的开源协议。理论上说，FreeRDP就包括了网络部分，但Launcher网络部分使用Chromium，FreeRDP已和网络收发无关了。因为使用Chromium，加密采用BoringSSL，没有再用OpenSSL。