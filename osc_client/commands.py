######################################################################################################
# -*- coding: UTF-8 -*-
# 文件名：  commands.py 
# 版本：    V0.0.1
# 修改记录：
# 日期              修改人                  版本            备注
######################################################################################################
import os
import json
import socket
import config
import requests
import base64
import time
import timer_util
from threading import Semaphore

class CommandsUtil():
    def __init__(self):
        self.serverIp = 'http://127.0.0.1:80'
        self.oscInfoUrl  = self.serverIp + '/osc/info'
        self.oscStateUrl = self.serverIp + '/osc/state'
        self.contentTypeHeader  = {'Content-Type': 'application/json'}
        self.checkUpdateUrl = '/osc/checkForUpdates'
        self.commandUrl = '/osc/commands/execute'
        self.commandStateUrl = '/osc/commands/status'

    def setServerIp(self, hostIp):
        self.serverIp = hostIp

    
    def getOscInfo(self):
        oscInfoRet = requests.get(self.stateUrl, headers=self.genericHeader)
        if oscInfoRet.status_code == requests.codes.ok:
            return oscInfoRet.json()    # 返回服务器返回的数据
        else:
            return None

    def getOscState(self):
        oscStateRet = requests.get(self.oscStateUrl, headers=self.genericHeader)
        if oscStateRet.status_code == requests.codes.ok:
            return oscStateRet.json()    # 返回服务器返回的数据
        else:
            return None

    def getOptions(self):
        pass 

    def setOptions(self):
        pass


    def takePicture(self, index):
        pass

    
    def takeInterval(self, interval):
        pass


    def takeVideo(self, index):
        pass

    def getLivePreview(self, index):
        pass


    def listFiles(self):
        pass

    def delteFile(self, url):
        pass


    
    

