#ifndef __TX_SOFT_AP
#define __TX_SOFT_AP
#ifdef __cplusplus
extern "C"{
#endif
int tx_softap_start();  //����ap

int tx_softap_config(const char *ssid,  const char *pwd ,int bopen);
int tx_softap_stop();	//�ر�ap
int tx_softsta_start(const char *ssid, const char *pwd , int time);//time表示启动等待时间
int tx_softsta_stop();
int tx_wifi_status(); //0 ap,1 sta,2 unknow
#ifdef __cplusplus
};
#endif
#endif


