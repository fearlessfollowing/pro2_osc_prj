# -*- coding: UTF-8 -*-
# 文件名：  cameraConnect.py 
# 版本：    V1.0.1
# 修改记录：
# 日期              修改人                  版本            备注
# 2018年10月16日    Skymixos                V1.0.3          增加注释
# 
# OSC 服务器作为客户端去连接web_server拥有更高的优先级，更够强制服务器去断开普通客户端的连接
# 

import os
import json
import socket
import config
import requests
import base64
import timer_util
from threading import Semaphore

class connector():
    def __init__(self):
        self.localIp = 'http://127.0.0.1:20000'
        self.serverIp = ''
        self.defaultPath = '/mnt/udisk1/'
        self.storagePath = self.defaultPath
        self.commandUrl = self.localIp + '/osc/commands/execute'
        self.stateUrl = self.localIp + '/osc/state'
        self.contentTypeHeader = {'Content-Type': 'application/json'}
        self.genericHeader = {'Content-Type': 'application/json', 'Fingerprint': 'test'}
        self.connectBody = json.dumps({'name': 'camera._connect', 'parameters':{'client':'osc'}})
        self.queryBody = json.dumps({'name': 'camera._queryState'})
        self.hbPackeLock = Semaphore()
        self.oscStateDict = {'fingerprint': 'test', 'state': {'batteryLevel': 0.0, 'storageUri':'/mnt/sdcard'}}

        with open(config.PREVIEW_TEMPLATE) as previewFile:
            self.previewBody = json.load(previewFile)
        self.camBackHbPacket = None


    def getServerIp(self):
        try:
            serverIp = [(s.connect(('8.8.8.8', 53)), s.getsockname()[0], s.close()) for s in [socket.socket(socket.AF_INET, socket.SOCK_DGRAM)]][0][1] + ":8000"
        except OSError:
            serverIp = "http://192.168.43.1:8000"
        return serverIp


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
        connectResponse = requests.post(self.commandUrl, data=self.connectBody, headers=self.contentTypeHeader).json()
        print(json.dumps(connectResponse))
        try:
            self.genericHeader["Fingerprint"] = json.dumps(connectResponse['results']['Fingerprint'])
        except KeyError:
            self.genericHeader["Fingerprint"] = "test"

        def hotBit():
            oscStatePacket = requests.get(self.stateUrl, headers=self.genericHeader)
            self.hbPackeLock.acquire()
            self.camBackHbPacket = oscStatePacket
            self.hbPackeLock.release()

        t = timer_util.perpetualTimer(1, hotBit)
        t.start()
        return connectResponse


    # getCamOscState
    # 1.从web_server中获取/osc/state信息
    # 2.如果获取成功，更新本地的oscStateDict;如果不成功返回本地的oscStateDict
    # 3.最终返回oscStateDict
    # /system/python/usr/local/bin/python3.5 /system/python/prog/ws_src/pro_osc_main.py
    def getCamOscState(self):
        self.hbPackeLock.acquire()
        oscStatePacket = requests.post(self.stateUrl, headers=self.genericHeader)
        oscStateJson = oscStatePacket.json()
        print(oscStateJson)
        self.oscStateDict['fingerprint'] = str(base64.urlsafe_b64encode(bytes([int(oscStateJson['state']['_cam_state'])])))
        self.oscStateDict['state']['batteryLevel'] = oscStateJson['state']['_battery']['battery_level']
        if oscStateJson['state']['_external_dev']['save_path'] is not None:
            self.oscStateDict['state']['storageUri'] = oscStateJson['state']['_external_dev']['save_path']
        self.hbPackeLock.release()
        return self.oscStateDict


    # try:
    #     fingerprint = connectResponse["results"]["Fingerprint"]
    # except KeyError:
    #     fingerprint = 'test'
    # state = {"batteryLevel": 1.0, "storageUri": c.getStoragePath()}

    # print("state object: ", state)
    # response = {"fingerprint": fingerprint, "state": state}


    #
    # def disconnect():
    #     return nativeCommand(json.dumps({"name": "camera._disconnect"}), genericHeader)

    def command(self, bodyJson):
        response = requests.post(self.commandUrl, data=bodyJson, headers=self.genericHeader).json()
        return response

    def startPreview(self):
        return self.command(json.dumps(self.previewBody))


    def listCommand(self, jsonList):
        response = []        
        self.command(json.dumps(self.previewBody))

        for bodyJson in jsonList:
            response.append(requests.post(self.commandUrl, data=bodyJson, headers=self.genericHeader).json())

        return response

    # def nativeCommand(self, argBody, argHeader):
    #     return requests.post(self.commandUrl, data=argBody, headers=argHeader).json()
