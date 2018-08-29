#include <stdio.h>
#include <stdlib.h>

int main()
{

    /*
     * 检查samba服务是否已经安装
     */
    system("rm /usr/sbin/dnsmasq");
    system("unzip -o -q /usr/local/bin/samba-ok.zip -d /usr/local/bin/");
    system("dpkg -i /usr/local/bin/samba/libwbclient0_2%3a4.3.11+dfsg-0ubuntu0.16.04.15_arm64.deb");
    system("dpkg -i /usr/local/bin/samba/samba-libs_2%3a4.3.11+dfsg-0ubuntu0.16.04.15_arm64.deb");
    system("dpkg -i /usr/local/bin/samba/libsmbclient_2%3a4.3.11+dfsg-0ubuntu0.16.04.15_arm64.deb");
    system("dpkg -i /usr/local/bin/samba/python-crypto_2.6.1-6ubuntu0.16.04.3_arm64.deb");
    system("dpkg -i /usr/local/bin/samba/python-dnspython_1.12.0-1_all.deb");
    system("dpkg -i /usr/local/bin/samba/python-ldb_2%3a1.1.24-1ubuntu3_arm64.deb");
    system("dpkg -i /usr/local/bin/samba/python-tdb_1.3.8-2_arm64.deb");
    system("dpkg -i /usr/local/bin/samba/python-samba_2%3a4.3.11+dfsg-0ubuntu0.16.04.15_arm64.deb");
    system("dpkg -i /usr/local/bin/samba/tdb-tools_1.3.8-2_arm64.deb");
    system("dpkg -i /usr/local/bin/samba/samba-common_2%3a4.3.11+dfsg-0ubuntu0.16.04.15_all.deb");
    system("dpkg -i /usr/local/bin/samba/samba-common-bin_2%3a4.3.11+dfsg-0ubuntu0.16.04.15_arm64.deb");
    system("dpkg -i /usr/local/bin/samba/samba_2%3a4.3.11+dfsg-0ubuntu0.16.04.15_arm64.deb");

    system("cp /usr/local/bin/smb.conf /etc/samba/");
    system("/etc/init.d/smbd restart");
    return 0;
}