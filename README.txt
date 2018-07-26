系统部分的改动记录：

日期：2018年6月29日
禁止Ubuntu系统中的非必要服务：
/usr/sbin/dnsmasq
/usr/sbin/nvcamera-daemon
network-manager


日期：2018年07月26日
针对PVT版本的量产硬件：
1.系统自带的ws_src中支持gps状态查询
2.系统中模组上电程序改动，由power_on/off.sh 调用power_manager程序，来实现给模组及HUB的供电控制
3.在/home/nvidia/insta360/etc/下新增.firm_ver,来代表系统固件的版本，本次发的系统版本为0.0.14，默认的.sys_ver也为0.0.14
4.固件包里去除factory_test文件。
测试的硬件：
Speaker
Audio
Gps
HUB 
Fan
