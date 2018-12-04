# -*- coding: UTF-8 -*-
# 文件名：  cameraConnect.py 
# 版本：    V1.0.1
# 修改记录：
# 日期              修改人                  版本            备注
# 2018年10月16日    Skymixos                V1.0.3          增加注释
# 
# OSC 服务器作为客户端去连接web_server拥有更高的优先级，更够强制服务器去断开普通客户端的连接
# 建立与WebServer连接的时候会启动

import os
import json
import socket
import config
import requests
import base64
import time
import timer_util
from threading import Semaphore
from log_util import *

# 1s的定时器
POLL_TO = 1000

class connector():
    def __init__(self):
        self.localIp = 'http://127.0.0.1:20000'
        self.serverIp = ''
        self.defaultPath = '/mnt/udisk1/'
        self.storagePath = self.defaultPath
        self.commandUrl = self.localIp + '/osc/commands/execute'
        self.stateUrl = self.localIp + '/osc/state'
        self.contentTypeHeader  = {'Content-Type': 'application/json'}
        self.genericHeader      = {'Content-Type': 'application/json', 'Fingerprint': 'test'}
        self.connectBody        = json.dumps({'name': 'camera._connect', 'parameters':{'client':'osc'}})
        self.disconnectBody     = json.dumps({'name':'camera._disconnect'})
        self.queryBody          = json.dumps({'name': 'camera._queryState'})
        self.hbPackeLock        = Semaphore()

        self.oscStateDict       = {'fingerprint': 'test', 'state': {'batteryLevel': 0.0, 'storageUri':None}}
        
        self.tickLock           = Semaphore()
        self.localTick          = 0

        # 用于标识服务器是否处于预览状态
        self.serverInPreview = False
        self.previewLock = Semaphore()

        # 是否在处理OSC客户端的请求
        # 当启动OSC服务器时，会设置一个1分钟超时的定时器，如果1分钟内没有OSC请求将主动断开与web_server的连接
        self.isOscCmdProcess = False

        # 是否与WebServer建立了连接
        # 何时与WebServer建立连接?
        # 1.启动OSC-Server时会与WebServer建立连接(self.isWebServerConnected = True)
        # 2.OSC请求到来，但(self.isWebServerConnected=False)时会与WebServer建立连接
        # 何时断开?
        # 1.OSC-Server处于空闲状态，1min中没有任何OSC请求时，主动与WebServer断开
        self.isWebSerConnected = False


        with open(config.PREVIEW_TEMPLATE) as previewFile:
            self.previewBody = json.load(previewFile)


        # 
        # 启动心跳包更新线程（该线程伴随osc_server一直存在）
        # 
        def hotBit():
            if self.isWebSerConnected == True and self.isOscCmdProcess == True:
                requests.get(self.stateUrl, headers=self.genericHeader)
                self.tickLock.acquire()
                self.localTick = 0
                self.tickLock.release()
            else:
                if self.isWebSerConnected == True:
                    self.tickLock.acquire()
                    self.localTick += 1
                    self.tickLock.release()
                    requests.get(self.stateUrl, headers=self.genericHeader)
    
                    # print("---> localtick = ", self.localTick)
                    Info("----> locltick: {}".format(self.localTick))
                    if self.localTick == 30:    # 主动与WebServer断开连接
                        self.disconnect()
                        self.tickLock.acquire()
                        self.localTick = 0
                        self.tickLock.release()
                        self.serverInPreview = False
                        self.isWebSerConnected = False

        t = timer_util.perpetualTimer(1, hotBit)
        t.start()

    def getServerIp(self):
        serverIp = "http://192.168.43.1:8000"
        return serverIp

    # 
    # 判断服务器是否处于预览状态
    # 
    def isServerInPreview(self):
        self.previewLock.acquire()
        previewState = self.serverInPreview
        self.previewLock.release()
        return previewState

    def clearLocalTick(self):
        self.tickLock.acquire()
        self.localTick = 0
        self.tickLock.release()


    def setIsCmdProcess(self, result):
        self.isOscCmdProcess = result

    # 判断是否与WebServer处于连接状态
    # 处于连接状态返回True;否则返回False
    def isWebServerConnected(self):
        return self.isWebSerConnected

    # 
    # 获取存储路径
    # /osc/state
    # 响应的["_external_dev"]["save_path"]
    def getStoragePath(self):
        try:
            oscStatePacket = requests.post(self.stateUrl, headers=self.genericHeader)
            oscStateJson = oscStatePacket.json()
            if oscStateJson['state']['_external_dev']['save_path'] is not None:
                storagePath = oscStateJson['state']['_external_dev']['save_path']
            else:
                storagePath = 'none'
        except IndexError:
            return None
        return storagePath

    def listUrls(self, dirUrl):
        urlList = os.listdir(dirUrl)
        return [self.getServerIp() + dirUrl + url for url in urlList]

    #
    # connect - 与web_server建立连接(connect)
    #
    def connect(self):
        Info("---> Ready connect Web-Server")
        connectResponse = requests.post(self.commandUrl, data=self.connectBody, headers=self.contentTypeHeader).json()
        print(json.dumps(connectResponse))
        try:
            self.genericHeader["Fingerprint"] = json.dumps(connectResponse['results']['Fingerprint'])
        except KeyError:
            self.genericHeader["Fingerprint"] = "test"

        self.isWebSerConnected = True  # 与服务器建立连接
        if self.isServerInPreview() == False:
            startPreviewRes = self.startPreview()
            print(startPreviewRes)
            Info("---> request start preview: {}".format(startPreviewRes))

            if startPreviewRes["state"] == "done":
                self.serverInPreview = True
                time.sleep(0.5)

        return connectResponse

    #   
    # disconnect - 与Web_server断开连接(disconnect)
    #  
    def disconnect(self):
        requests.post(self.commandUrl, data=self.disconnectBody, headers=self.genericHeader)
        self.isWebSerConnected = False  # 与服务器断开连接
        self.serverInPreview = False


    # getCamOscState
    # 1.从web_server中获取/osc/state信息
    # 2.如果获取成功，更新本地的oscStateDict;如果不成功返回本地的oscStateDict
    # 3.最终返回oscStateDict
    # /system/python/usr/local/bin/python3.5 /system/python/prog/ws_src/pro_osc_main.py
    def getCamOscState(self):
        oscStatePacket = requests.post(self.stateUrl, headers=self.genericHeader)
        oscStateJson = oscStatePacket.json()
        self.oscStateDict['fingerprint'] = str(hex(oscStateJson['state']['_cam_state']))
        batteryPer = oscStateJson['state']['_battery']['battery_level'] / 100
        if batteryPer == 10:
            batteryPer = 0
        self.oscStateDict['state']['batteryLevel'] = float(batteryPer)
        if oscStateJson['state']['_external_dev']['save_path'] is not None:
            self.oscStateDict['state']['storageUri'] = oscStateJson['state']['_external_dev']['save_path']
    
        return self.oscStateDict


    def command(self, bodyJson):
        response = requests.post(self.commandUrl, data=bodyJson, headers=self.genericHeader).json()
        return response

    # 
    # 启动预览
    # 
    def startPreview(self):
        return self.command(json.dumps(self.previewBody))


    def listCommand(self, jsonList):
        response = []        
        self.command(json.dumps(self.previewBody))

        for bodyJson in jsonList:
            response.append(requests.post(self.commandUrl, data=bodyJson, headers=self.genericHeader).json())

        return response
