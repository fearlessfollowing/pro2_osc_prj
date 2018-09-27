import commands
import requests
import cameraConnect
import commandUtility
import json
import os
from shutil import copyfile
import time
from flask import Flask, make_response, request, abort, redirect, Response

app = Flask(__name__)
c = cameraConnect.connector()
connectResponse = c.connect()
# connectResponse = c.connect()
# print(connectResponse)
startTime = time.time()


@app.before_first_request
def setup():
    copyfile('defaultOptions.json', 'currentOptions.json')


@app.route('/osc/commands/<option>', methods=['POST'])
def getResponse(option):
    errorValues = None
    bodyJson = request.get_json()
    print("\nREQUEST: ", bodyJson)
    if bodyJson is None:
        response = ''
        print("???: ", request)
        return response, 204

    if option == 'status':
        print("STATUS: /osc/commands/status")
        try:
            commandId = int(bodyJson["id"])
        except KeyError:
            errorValues = commandUtility.buildError('missingParameter',
                                                    "command id required")
        if len(bodyJson.keys()) > 1:
            errorValues = commandUtility.buildError('invalidParameterName',
                                                    "invalid param")
        if errorValues is not None:
            responseValues = ("camera.status", "error", errorValues)
            response = commandUtility.buildResponse(responseValues)
            finalResponse = make_response(response)
            finalResponse.headers['Content-Type'] = "application/json;charset=utf-8"
            return finalResponse, 400

        progressRequest = json.dumps({"name": "camera._getResult",
                                      "parameters": {"list_ids": [commandId]}})
        response = c.command(progressRequest)
        if "error" in response.keys():
            # errorValues = commandUtility.buildError('invalidParameterValue',
            #                                         "command id not found")
            # responseValues = ("status", "error", errorValues)
            responseValues = ("camera.takePicture", "inProgress", commandId, 0.5)
            response = commandUtility.buildResponse(responseValues)
        else:
            progress = response["results"]["res_array"][0]["results"]
            with open('statusMapping.json') as statusFile:
                statusMap = json.load(statusFile)
            state = progress["state"]
            if state == "done":
                resultKey = list(progress["results"].keys())[0]
                mappedList = statusMap[resultKey]
                name = mappedList[0]
                results = progress["results"][resultKey]
                if name == "camera.takePicture":
                    #finalResults = os.path.join(c.getServerIp(), results)
                    finalResults = c.getServerIp() + results + '/pano.jpg'
                elif name == "camera.startCapture":
                    finalResults = []
                    folderUrl = c.getServerIp() + results + '/'
                    for f in os.listdir(results):
                        if "jpg" or "jpeg" in f:
                            finalResults.append(folderUrl + f)
                        if "mp4" in f:
                            finalResults = c.getServerIp() + results + '/pano.mp4'
                            break
                responseValues = (name, state, {mappedList[1]: finalResults})
            if state == "inProgress":
                responseValues = (name, state, commandId, 0.1)
            if state == "error":
                # responseValues = (name, state, commandId, 0.5)
                error = commandUtility.buildError('disabledCommand',
                                                  "internal camera error")
                responseValues = (name, state, error)
            response = commandUtility.buildResponse(responseValues)

    elif option == 'execute' and bodyJson is not None:
        name = bodyJson['name'].split('.')[1]
        print("COMMAND: " + name)
        hasParams = "parameters" in bodyJson.keys()
        try:
            if hasParams:
                commandParams = bodyJson["parameters"]
                response = getattr(commands, name)(c, commandParams)
            else:
                response = getattr(commands, name)(c)
            if name == "getLivePreview" and "error" not in response:
                time.sleep(3)
                # Testing two methodologies, below:
                r = requests.get(response, stream=True)
                return Response(r.iter_content(chunk_size=10*1024),
                                content_type='multipart/x-mixed-replace')
                # or below
                # return redirect(response, 302)
        except AttributeError:
            errorValue = commandUtility.buildError('unknownCommand',
                                                   'command not found')
            responseValues = (bodyJson['name'], "error", errorValue)
            response = commandUtility.buildResponse(responseValues)
        except TypeError:
            errorValue = commandUtility.buildError('invalidParameterName',
                                                   'invalid param name')
            responseValues = (name, "error", errorValue)
            response = commandUtility.buildResponse(responseValues)

    else:
        abort(404)
    finalResponse = make_response(response)
    finalResponse.headers['Content-Type'] = "application/json;charset=utf-8"
    finalResponse.headers['X-Content-Type-Options'] = "nosniff"
    print("RESPONSE: ", response)
    if response == '':
        return finalResponse, 204
    elif "error" in json.loads(response).keys():
        return finalResponse, 400
    return finalResponse


@app.route('/osc/info', methods=['GET'])
def getInfo():
    print("\nREQUEST: /osc/info")
    with open('info.json') as infoFile:
        response = json.load(infoFile)
    response["serialNumber"] = connectResponse["results"]["sys_info"]["sn"]
    response["firmwareVersion"] = connectResponse["results"]["sys_info"]["r_v"]
    response["uptime"] = int(time.time()-startTime)
    print("RESPONSE: ", response)
    response = make_response(json.dumps(response))
    response.headers['Content-Type'] = "application/json;charset=utf-8"
    return response


@app.route('/osc/state', methods=['POST'])
def getState():
    print("\nREQUEST: /osc/state")
    try:
        fingerprint = connectResponse["results"]["Fingerprint"]
    except KeyError:
        fingerprint = 'test'
    state = {"batteryLevel": 1.0, "storageUri": c.getStoragePath()}
    print("state object: ", state)
    response = {"fingerprint": fingerprint, "state": state}
    print("RESPONSE: ", response)
    finalResponse = make_response(json.dumps(response))
    finalResponse.headers['Content-Type'] = "application/json;charset=utf-8"
    return finalResponse


# @app.route('/osc/checkForUpdates', methods=['POST'])
# def compareFingerprint():
#     print("\nasked for update\n")
#     errorValues = None
#     waitTimeout = None
#     bodyJson = request.get_json()
#     try:
#         stateFingerprint = bodyJson["stateFingerprint"]
#         if "waitTimeout" in bodyJson.keys():
#             waitTimeout = bodyJson["waitTimeout"]
#     except KeyError:
#         errorValues = commandUtility.buildError("missingParameter",
#                                                 "stateFingerprint not specified")
#     if (waitTimeout is None and len(bodyJson.keys()) > 1) or len(bodyJson.keys()) > 2:
#         errorValues = commandUtility.buildError("invalidParameterName")
#     if errorValues is not None:
#         responseValues = ("camera.checkForUpdates", "error", errorValues)
#         response = commandUtility.buildResponse(responseValues)
#     else:
#         connectResponse = c.connect()
#         throttleTimeout = 30
#         newFingerprint = connectResponse["Fingerprint"]
#         # if newFingerprint != stateFingerprint:
#         results = {"stateFingerprint": newFingerprint,
#                    "throttleTimeout": throttleTimeout}
#         responseValues = ("camera.checkForUpdates", "done", results)
#         # while newFingerprint == stateFingerprint and :
#     finalResponse = make_response(json.dumps(response))
#     finalResponse.headers['Content-Type'] = "application/json;charset=utf-8"
#     return finalResponse


if __name__ == "__main__":
    app.run(host='0.0.0.0', port=80, threaded=True)
