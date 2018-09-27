import requests
import json
import os
import socket
import timer_util


class connector():

    def __init__(self):
        self.localIp = 'http://127.0.0.1:20000'
        self.serverIp = ''
        self.defaultPath = '/mnt/media_rw/'
        self.storagePath = self.defaultPath
        self.commandUrl = self.localIp + '/osc/commands/execute'
        self.stateUrl = self.localIp + '/osc/state'
        self.contentTypeHeader = {'Content-Type': 'application/json'}
        self.genericHeader = {'Content-Type': 'application/json', 'Fingerprint': 'test'}
        self.connectBody = json.dumps({'name': 'camera._connect'})
        self.queryBody = json.dumps({'name': 'camera._queryState'})
        with open("previewTemplate.json") as previewFile:
            self.previewBody = json.load(previewFile)

    def getServerIp(self):
        try:
            serverIp = [(s.connect(('8.8.8.8', 53)), s.getsockname()[0], s.close()) for s in [socket.socket(socket.AF_INET, socket.SOCK_DGRAM)]][0][1] + ":8000"
        except OSError:
            serverIp = "http://192.168.43.1:8000"
        return serverIp

    def getStoragePath(self):
        try:
            storagePath = os.path.join(self.defaultPath, os.listdir('/mnt/media_rw/')[0])
        except IndexError:
            return None
        return storagePath

    def listUrls(self, dirUrl):
        urlList = os.listdir(dirUrl)
        return [self.getServerIp() + dirUrl + url for url in urlList]

    def connect(self):
        connectResponse = requests.post(self.commandUrl, data=self.connectBody, headers=self.contentTypeHeader).json()

        try:
            self.genericHeader["Fingerprint"] = json.dumps(connectResponse['results']['Fingerprint'])
        except KeyError:
            pass

        def hotBit():
            requests.get(self.stateUrl, headers=self.genericHeader)
        t = timer_util.perpetualTimer(1, hotBit)

        t.start()
        return connectResponse

    #
    # def disconnect():
    #     return nativeCommand(json.dumps({"name": "camera._disconnect"}), genericHeader)

    def command(self, bodyJson):
        # self.connect()
        response = requests.post(self.commandUrl, data=bodyJson, headers=self.genericHeader).json()
        return response

    def listCommand(self, jsonList):
        # self.connect()
        response = []
        # startPreview()
        self.command(self.previewBody)
        for bodyJson in jsonList:
            response.append(requests.post(self.commandUrl, data=bodyJson, headers=self.genericHeader).json())
        return response

    # def nativeCommand(self, argBody, argHeader):
    #     return requests.post(self.commandUrl, data=argBody, headers=argHeader).json()
