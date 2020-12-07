# pjsip-for-esp32
------------------------------
This project is a VoIP(only audio) project maintained by Juphoon Systems.  
All the codes are an adapter for Espressif chips.  
It's based on #PJSIP from https://github.com/pjsip.  
PJSIP master branch with commit 047dc3862.  

What's more, the project learned a lot from project  
#pjlib-esp32 from https://github.com/nguyen-phuoc-dai/pjlib-esp32.  

# Toolchain Info  
------------------------------
Toolchain version:  
  crosstool-ng-1.22.0-80-g6c4433a.  
Chip:  
  ESP32-LyraTD-MSC_A v2.2.  
  
The corresponding ESP-ADF tools can be found at:  
  https://yunpan.360.cn/surl_ySftmT45ps5 (Verification Code:6f20)  

# Demo Usage
------------------------------
When run the main demo, one should change the following macro definitions first in #main.c(pjsip/main/main.c)  
  #define WIFI_SSID   "xxx"            // wifi ssid  
  #define WIFI_PWD    "xxx"            // wifi password  

  #define SIP_DOMAIN	"xx.xx.xx.xx"    // sip server ip address  
  #define SIP_PASSWD	"xxx"            // sip regiser password  
  #define SIP_USER	"xxx"              // sip account name  
  #define SIP_CALLEE  "xxx"            // callee account name  
  #define SIP_PORT    "5070"           // sip port  
  #define SIP_TRANSPORT_TYPE PJSIP_TRANSPORT_TCP   // default sip transport type  

------------------------------  
Finally, thanks a lot for the help from Michael Zhang in Espressif Systems!
