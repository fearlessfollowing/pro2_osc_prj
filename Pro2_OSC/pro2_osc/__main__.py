# -*- coding: UTF-8 -*-
#########################################################################################
# 文件名：  __main__.py 
# 版本：    V1.0.1
# 修改记录：
# 日期              修改人                  版本            备注
# 2018年10月16日    Skymixos                V1.0.3          增加注释
# 
# BUG467 - 设置时钟之前不能进入预览，否则相机会报467错误
# 1.OSC服务器在1min中没有接收到客户端的任何请求时会主动断开与Web_server的连接
# 2.OSC服务器的连接Web_server的优先级大于客户端,当osc去去连接Web_server时,如果Web_server处于
#   与客户端连接的状态会先断开与客户端的连接再与OSC建立连接
#########################################################################################

import os
import json
import time
import config
import commands
import requests
import base64
import cameraConnect
import commandUtility
from shutil import copyfile
from flask import Flask, make_response, request, abort, redirect, Response
from log_util import *

#
# 与服务器(web_server)建立连接
# 1.如果此时服务器与客户端已经连接了连接，应该返回错误
# 2.在获取/osc/info时去连接web_server
#
# 一段时间没有请求后将自动退出预览，断开与web_server的连接
# 启动预览
# 获取Sensor相关Option到Current Options中


app = Flask(__name__)

# 构造全局的Camera连接对象 #
gCameraConObj = cameraConnect.connector()

# 加载osc_info.json的内容，保存在全局变量gOscInfoResponse中 #
with open(config.OSC_INFO_TEMPLATE) as oscInfoFile:
    gOscInfoResponse = json.load(oscInfoFile)

with open(config.SN_FIRM_JSON) as snFirmFile:
    gSnFirmInfo = json.load(snFirmFile)


@app.before_first_request
def setup():
    copyfile(config.DEFAULT_OPTIONS, config.CURRENT_OPTIONS)


# getInfo()
# 描述: /osc/info API返回有关支持的相机和功能的基本信息(不需要与web_server建立连接即可)
# 输入: 无
# 输出:
#   manufacturer - string(相机制造商)
#   model - string(相机型号)
#   serialNumber - string(序列号)
#   firmwareVersion - string(当前固件版本)
#   supportUrl - string(相机支持网页的URL)
#   gps - bool(相机GPS)
#   gyro - 陀螺仪(bool)
#   uptime - 相机启动后的秒数(int)
#   api - 支持的API列表(字符串数组)
#   endpoints - {httpPort, httpUpdatesPort, httpsPort, httpsUpdatesPort}
#   apiLevel - [1], [2], [1,2]
#   _vendorSpecific
#
@app.route('/osc/info', methods=['GET'])
def getInfo():
    Info('[---- OSC Request: /osc/info ------]')
    gOscInfoResponse["serialNumber"]    = gSnFirmInfo["serialNumber"]
    gOscInfoResponse["firmwareVersion"] = gSnFirmInfo["firmwareVersion"]

    # 如果OSC-Server没有与WebServer建立连接，先在次建立连接
    if gCameraConObj.isWebServerConnected() == False:
        gCameraConObj.connect()

    # 读取系统已经启动的时间
    with open(config.UP_TIME_PATH) as upTimeFile:
        startUptimeLine = upTimeFile.readline()
        upTimes = startUptimeLine.split(" ")
        upTime = float(upTimes[0])
        gOscInfoResponse["uptime"] = int(upTime)

    Info('Result[/osc/info]: {}'.format(gOscInfoResponse))

    response = make_response(json.dumps(gOscInfoResponse))
    response.headers['Content-Type'] = "application/json;charset=utf-8"
    response.headers['X-Content-Type-Options'] = 'nosniff'
    return response


# getState()
# 描述：/osc/state API返回相机的state属性(不需要与WebServer建立连接)
# 输入: 无
# 输出:
#   fingerprint - string(当前相机状态的指纹)
#   state - json对象(相机型号)
#   {
#       batterLevel - float(电池电量)
#       storageUri  - string(区分不通存储的唯一标识)
#   }
#
@app.route('/osc/state', methods=['POST'])
def getState():
    Info('[---- Client Request: /osc/state ------]')    
    
    # 返回一个响应字典
    response = gCameraConObj.getCamOscState()
    
    print(response)
    
    Info("Result[/osc/state]: {}".format(response))
    finalResponse = make_response(json.dumps(response))
    finalResponse.headers['Content-Type'] = "application/json;charset=utf-8"
    finalResponse.headers['X-Content-Type-Options'] = 'nosniff'    
    return finalResponse



@app.route('/osc/checkForUpdates', methods=['POST'])
def compareFingerprint():
    Info('[-----/osc/checkForUpdates--------------]')
    errorValues = None
    waitTimeout = None
    bodyJson = request.get_json()

    try:
        stateFingerprint = bodyJson["stateFingerprint"]
        if "waitTimeout" in bodyJson.keys():
            waitTimeout = bodyJson["waitTimeout"]
    except KeyError:
        errorValues = commandUtility.buildError("missingParameter", "stateFingerprint not specified")
    
    if (waitTimeout is None and len(bodyJson.keys()) > 1) or len(bodyJson.keys()) > 2:
        errorValues = commandUtility.buildError("invalidParameterName")

    if errorValues is not None:
        responseValues = ("camera.checkForUpdates", "error", errorValues)
        response = commandUtility.buildResponse(responseValues)
    else:
        oscState = gCameraConObj.getCamOscState()

        throttleTimeout = 60
        newFingerprint = oscState['fingerprint']
        results = {"stateFingerprint": newFingerprint, "throttleTimeout": throttleTimeout}
        responseValues = ("camera.checkForUpdates", "done", results)

    finalResponse = make_response(json.dumps(response))
    finalResponse.headers['Content-Type'] = "application/json;charset=utf-8"
    return finalResponse


# getResponse()
# 描述: 处理(status, command)
# 
@app.route('/osc/commands/<option>', methods=['POST'])
def getResponse(option):
    errorValues = None
    bodyJson = request.get_json()

    Info("------------- REQUEST: {}".format(bodyJson))
    if bodyJson is None:
        response = ''
        print("???: ", request)
        return response, 204

    # 设置正在处理命令
    gCameraConObj.setIsCmdProcess(True)    
    gCameraConObj.clearLocalTick()

    if gCameraConObj.isWebServerConnected() == False:
        gCameraConObj.connect()


    # 获取状态
    if option == 'status':
        Info("STATUS: /osc/commands/status")
        try:
            commandId = int(bodyJson["id"])
        except KeyError:
            errorValues = commandUtility.buildError('missingParameter', "command id required")
        
        if len(bodyJson.keys()) > 1:
            errorValues = commandUtility.buildError('invalidParameterName', "invalid param")
        
        if errorValues is not None:
            responseValues = ("camera.status", "error", errorValues)
            response = commandUtility.buildResponse(responseValues)
            finalResponse = make_response(response)
            finalResponse.headers['Content-Type'] = "application/json;charset=utf-8"
            finalResponse.headers['X-Content-Type-Options'] = "nosniff"

            gCameraConObj.setIsCmdProcess(False)
            return finalResponse, 400

        progressRequest = json.dumps({"name": "camera._getResult", "parameters": {"list_ids": [commandId]}})
        response = gCameraConObj.command(progressRequest)
        if "error" in response.keys():
            responseValues = ("camera.takePicture", "inProgress", commandId, 0.5)
            response = commandUtility.buildResponse(responseValues)
        else:
            progress = response["results"]["res_array"][0]["results"]
            with open(config.STATUS_MAP_TEMPLATE) as statusFile:
                statusMap = json.load(statusFile)

            state = progress["state"]
            if state == "done":
                resultKey = list(progress["results"].keys())[0]
                mappedList = statusMap[resultKey]
                name = mappedList[0]
                results = progress["results"][resultKey]
                if name == "camera.takePicture":
                    finalResults = gCameraConObj.getServerIp() + results + '/pano.jpg'
                elif name == "camera.startCapture":
                    finalResults = []
                    folderUrl = gCameraConObj.getServerIp() + results + '/'
                    for f in os.listdir(results):
                        if "jpg" or "jpeg" in f:
                            finalResults.append(folderUrl + f)
                        if "mp4" in f:
                            finalResults = gCameraConObj.getServerIp() + results + '/pano.mp4'
                            break
                responseValues = (name, state, {mappedList[1]: finalResults})
            if state == "inProgress":
                responseValues = (name, state, commandId, 0.1)
            if state == "error":
                # responseValues = (name, state, commandId, 0.5)
                error = commandUtility.buildError('disabledCommand', "internal camera error")
                responseValues = (name, state, error)
            response = commandUtility.buildResponse(responseValues)

    # 请求执行命令
    elif option == 'execute' and bodyJson is not None:
        name = bodyJson['name'].split('.')[1]
        Info("Execute command: " + name)
        
        hasParams = "parameters" in bodyJson.keys()
        try:
            if hasParams:
                commandParams = bodyJson["parameters"]
                response = getattr(commands, name)(gCameraConObj, commandParams)
            else:
                response = getattr(commands, name)(gCameraConObj)

            # if name == "getLivePreview" and "error" not in response:
            #     time.sleep(3)
            #     # Testing two methodologies, below:
            #     r = requests.get(response, stream=True)
            #     return Response(r.iter_content(chunk_size=10*1024),
            #                     content_type='multipart/x-mixed-replace')
                # or below
                # return redirect(response, 302)
        except AttributeError:
            errorValue = commandUtility.buildError('unknownCommand', 'command not found')
            responseValues = (bodyJson['name'], "error", errorValue)
            response = commandUtility.buildResponse(responseValues)
        except TypeError:
            errorValue = commandUtility.buildError('invalidParameterName', 'invalid param name')
            responseValues = (name, "error", errorValue)
            response = commandUtility.buildResponse(responseValues)
    else:
        abort(404)


    Info("Command Response: {}".format(response))

    finalResponse = make_response(response)
    finalResponse.headers['Content-Type'] = "application/json;charset=utf-8"
    finalResponse.headers['X-Content-Type-Options'] = "nosniff"

    # 对于Option类型的命令返回响应代表命令处理完成    
    if option == 'status':
        gCameraConObj.setIsCmdProcess(False)
    elif option == 'execute':
        # 对于录像，启动后不能算命令完成
        # 停止的条件: 
        #   1.空间不足，被动停止（由其他的执行路径设置setIsCmdProcess）
        #   2.主动停止stopCapture
        if name != "startCapture" or name != 'getLivePreview':
            gCameraConObj.setIsCmdProcess(False)
    else:
        gCameraConObj.setIsCmdProcess(False)

    Info("RESPONSE: {}".format(response))
    if response == '':
        return finalResponse, 204
    else:    
        return finalResponse


if __name__ == "__main__":
    app.run(host='0.0.0.0', port=80, threaded=True)
