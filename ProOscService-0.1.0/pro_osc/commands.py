import json
import commandUtility
import findOptions
import changeOptions
import os
import requests
from shutil import rmtree

with open('currentOptions.json') as optionsFile:
    currentOptions = json.load(optionsFile)


def takePicture(c):
    takePictureJson = __loadJsonFile('takePictureTemplate.json')
    currentOptions = __loadJsonFile('currentOptions.json')
    pictureMap = __loadJsonFile('pictureFormatMap.json')
    if currentOptions["exposureProgram"] in [1, 2] and currentOptions["captureMode"] in ["image"]:
        fileFormat = currentOptions["fileFormat"]
        if fileFormat["type"] == "jpeg":
            takePictureJson["parameters"]["stiching"]["mime"] = fileFormat["type"]
            takePictureJson["parameters"]["stiching"]["width"] = fileFormat["width"]
            takePictureJson["parameters"]["stiching"]["height"] = fileFormat["height"]
        else:
            del takePictureJson["parameters"]["stiching"]
        takePictureJson["parameters"]["origin"]["mime"] = pictureMap["type"][str(fileFormat["type"])]
        takePictureJson["parameters"]["origin"]["width"] = pictureMap["width"][str(fileFormat["width"])]
        takePictureJson["parameters"]["origin"]["height"] = pictureMap["height"][str(fileFormat["height"])]
        hdr = currentOptions["hdr"]
        if hdr == "hdr":
            exposureBracket = currentOptions["exposureBracket"]
            takePictureJson["parameters"]["hdr"]["enable"] = True
            takePictureJson["parameters"]["hdr"]["count"] = exposureBracket["shots"]
            max_ev = ((exposureBracket["shots"] - 1)/2)*exposureBracket["increment"]
            takePictureJson["parameters"]["hdr"]["min_ev"] = -1 * max_ev
            takePictureJson["parameters"]["hdr"]["max_ev"] = max_ev
        else:
            del takePictureJson["parameters"]["hdr"]
        takePictureJson["parameters"]["delay"] = currentOptions["exposureDelay"]
        takePictureJson["stabilization"] = currentOptions["imageStabilization"]
        print(takePictureJson)
        # exposureOptions = __setExposure(currentOptions)
        # requests = (__genOptionsBody(exposureOptions))
        # requests.append(json.dumps(takePictureJson))
        request = json.dumps(takePictureJson)

        # results = c.listCommand(requests)[-1]
        results = c.command(request)
        responseValues = __checkCameraError(results, "camera.takePicture")
        if responseValues is None:
            print(results)
            commandId = results["sequence"]
            responseValues = ("camera.takePicture", "inProgress", commandId, 0)
    else:
        print(currentOptions["captureMode"])
        responseValues = ("camera.takePicture", "error",
                          commandUtility.buildError('disabledCommand', 'Cannot take picture in current mode/state'))
    return commandUtility.buildResponse(responseValues)


def processPicture(c, cParams):
    responseValues = ("camera.processPicture", "error",
                      commandUtility.buildError('disabledCommand', 'Not supported'))
    return commandUtility.buildResponse(responseValues)


def startCapture(c):
    currentOptions = __loadJsonFile('currentOptions.json')

    if currentOptions["captureMode"] not in ["interval", "video"]:
        errorValue = commandUtility.buildError('disabledCommand', 'video/interval recording not supported in this mode')
        responseValues = ("camera.startCapture", "error", errorValue)
        return commandUtility.buildResponse(responseValues)

    # exposureOptions = __setExposure(currentOptions)
    fileFormat = currentOptions["fileFormat"]

    if currentOptions["captureMode"] == "video":
        startCaptureJson = __loadJsonFile('startCaptureTemplate.json')
        videoMap = __loadJsonFile('videoFormatMap.json')
        startCaptureJson["parameters"]["origin"]["width"] = videoMap["width"][str(fileFormat["width"])]
        startCaptureJson["parameters"]["origin"]["height"] = videoMap["height"][str(fileFormat["height"])]
        startCaptureJson["parameters"]["origin"]["framerate"] = fileFormat["framerate"]
        if fileFormat["width"] > 3840 or fileFormat["framerate"] > 30:
            del startCaptureJson['stiching']
        else:
            startCaptureJson["parameters"]["stiching"]["width"] = fileFormat["width"]
            startCaptureJson["parameters"]["stiching"]["height"] = fileFormat["height"]
            startCaptureJson["parameters"]["stiching"]["framerate"] = fileFormat["framerate"]

    elif currentOptions["captureMode"] == "interval":
        startCaptureJson = __loadJsonFile('intervalTemplate.json')
        pictureMap = __loadJsonFile('pictureFormatMap.json')
        startCaptureJson["parameters"]["timelapse"]["enable"] = True
        startCaptureJson["parameters"]["timelapse"]["interval"] = currentOptions["captureInterval"]*1000
        # startCaptureJson["parameters"]["stiching"]["mime"] = fileFormat["type"]
        # startCaptureJson["parameters"]["stiching"]["width"] = fileFormat["width"]
        # startCaptureJson["parameters"]["stiching"]["height"] = fileFormat["height"]
        startCaptureJson["parameters"]["origin"]["mime"] = fileFormat["type"]
        startCaptureJson["parameters"]["origin"]["width"] = pictureMap["width"][str(fileFormat["width"])]
        startCaptureJson["parameters"]["origin"]["height"] = pictureMap["height"][str(fileFormat["height"])]
        if currentOptions["captureNumber"] > 0:
            captureTime = currentOptions["captureInterval"]*(currentOptions["captureNumber"]) + 1
            startCaptureJson["parameters"]["duration"] = captureTime
    print(startCaptureJson)
    # request = (__genOptionsBody(exposureOptions))
    # request.append(json.dumps(startCaptureJson))
    request = json.dumps(startCaptureJson)
    results = c.command(request)
    # results = c.listCommand(request)[-1]
    print(results)
    responseValues = __checkCameraError(results, "camera.startCapture")
    if responseValues is None:
        if currentOptions["captureMode"] == "interval" and currentOptions["captureNumber"] > 0:
            try:
                commandId = results['sequence']
                responseValues = ("camera.startCapture", "inProgress", commandId, 0)
            except KeyError:
                errorValues = commandUtility.buildError("disabledCommand", "internal camera error")
                responseValues = ("camera.startCapture", "error", errorValues)
        else:
            fileUrl = {"fileUrls": [results["results"]["_recordUrl"]]}
            print(fileUrl)
            with open('fileUrls.json', 'w') as urlFile:
                json.dump(fileUrl, urlFile)
            return ''
    return commandUtility.buildResponse(responseValues)


def stopCapture(c):
    currentOptions = __loadJsonFile('currentOptions.json')
    if currentOptions["captureMode"] in ["video", "interval"]:
        body = json.dumps({"name": "camera._stopRecording"})
        response = c.command(body)
        fileDir = __loadJsonFile('fileUrls.json')['fileUrls']
        if response["state"] == "error":
            errorValues = commandUtility.buildError('disabledCommand', "internal camera error")
            responseValues = ("camera.stopCapture", "error", errorValues)
        elif currentOptions["captureMode"] == "video":
            responseValues = ("camera.stopCapture", "done", fileDir[0])
        elif currentOptions["captureMode"] == "interval":
            finalResults = []
            folderUrl = c.getServerIp() + fileDir[0] + '/'
            for f in os.listdir(fileDir[0]):
                if "jpg" in f or "jpeg" in f:
                    finalResults.append(folderUrl + f)
                if "mp4" in f:
                    finalResults = c.getServerIp() + fileDir[0] + '/pano.mp4'
                    break
            responseValues = ("camera.stopCapture", "done", finalResults)
    else:
        errorValues = commandUtility.buildError('disabledCommand', "command not supported in this mode")
        responseValues = ("camera.stopCapture", "error", errorValues)
    return commandUtility.buildResponse(responseValues)


def getLivePreview(c):
    cameraState = c.command(json.dumps({"name": "camera._queryState"}))["results"]["state"]
    if cameraState not in ["idle", "preview"]:
        errorValues = commandUtility.buildError('disabledCommand',
                                                'preview disabled in this mode')
        responseValues = ("camera.getLivePreview", "error", errorValues)
        return commandUtility.buildResponse(responseValues)
    else:
        previewBody = __loadJsonFile("previewTemplate.json")
        previewBody["parameters"]["stiching"]["width"] = currentOptions["previewFormat"]["width"]
        previewBody["parameters"]["stiching"]["height"] = currentOptions["previewFormat"]["height"]
        previewBody["parameters"]["stiching"]["framerate"] = currentOptions["previewFormat"]["framerate"]
        response = c.command(json.dumps(previewBody))
        print(response)
        previewUrl = c.getServerIp() + response['results']['_previewUrl']
        print(previewUrl)
        # time.sleep(5)
        # preview = requests.get(previewUrl, stream=True)
        #
        return previewUrl


def listFiles(c, cParams):
    errorValues = None
    startPosition = None
    try:
        fileType = cParams["fileType"]
        entryCount = cParams["entryCount"]
        maxThumbSize = cParams["maxThumbSize"]
        if "startPosition" in list(cParams.keys()):
            startPosition = cParams["startPosition"]
    except KeyError:
        errorValues = commandUtility.buildError("missingParameter",
                                                "missing param")
    if (len(cParams) > 3 and startPosition is None) or len(cParams) > 4:
        errorValues = commandUtility.buildError("invalidParameterName",
                                                "unrecognized param")
    if startPosition is None:
        startPosition = 0
    if errorValues is not None:
        responseValues = ("camera.listFiles", "error", errorValues)
        return commandUtility.buildResponse(responseValues)

    storagePath = c.getStoragePath()

    print("reported storage path: " + storagePath)
    if fileType in ["all", "video", "image"]:
        if fileType == "all":
            fileList = __urlListHelper(c, 'ALL')
        elif fileType == "video":
            fileList = __urlListHelper(c, 'VID')
        elif fileType == "image":
            fileList = __urlListHelper(c, 'PIC')
        if startPosition >= len(fileList):
            responseValues = ("camera.listFiles", "done", [''])
            return commandUtility.buildResponse(responseValues)
    else:
        errorValues = commandUtility.buildError('invalidParameterValue',
                                                'invalid param value')
        responseValues = ("camera.listFiles", "error", errorValues)
        return commandUtility.buildResponse(responseValues)

    totalEntries = len(fileList)
    entries = __loadJsonFile('entriesTemplate.json')
    fileList = sorted(fileList, key=__getDate, reverse=True)[startPosition:]
    if entryCount < len(fileList):
        fileList = fileList[:entryCount]

    entryList = []
    # xmlDir = storagePath.split(':')[0] + ':' + "8000" + storagePath.split(':')[1][5:]
    for f in fileList:
        entries["name"] = f
        serverIp = c.getServerIp()
        fileUri = os.path.join(storagePath, f)
        folderUri = fileUri + '/'
        entries["size"] = sum(os.path.getsize(folderUri + a) for a in os.listdir(folderUri))
        # xmlUrl = xmlDir + '/' + f + '/' + "pro.proj"
        date = f.split('_')[1:]
        entries["dateTimeZone"] = ':'.join(date[:3]) + ' ' + ':'.join(date[3:])
        if "VID" in f:
            fn = "pano.mp4"
        elif "PIC" in f:
            fn = "pano.jpg"
        entries["fileUrl"] = serverIp + os.path.join(storagePath, f, fn)
        if fn in os.listdir(fileUri):
            entries["isProcessed"] = True
            if fn == "pano.jpg":
                # im = Image.open(folderPath + fn)
                # (entries["width"], entries["height"]) = im.size
                if maxThumbSize is not None:
                    entries["thumbnail"] = folderUri + "thumbnail.jpg" # how to encode int
                    # with open(os.path.join(folderUri, "thumbnail.jpg"), "rb") as image_file:
                    #     encoded_string = base64.b64encode(image_file.read())
                    # entries["thumbnail"] = encoded_string
            elif fn == "pano.mp4":
                entries["previewUrl"] = os.path.join(serverIp, folderUri, "preview.mp4")
        else:
            entries["isProcessed"] = False
        entryList.append(dict(entries))

    results = {"entries": entryList, "totalEntries": totalEntries}
    responseValues = ("camera.listFiles", "done", results)
    return commandUtility.buildResponse(responseValues)


def delete(c, cParams):
    errorValues = None
    try:
        fileList = cParams["fileUrls"]
    except KeyError:
        errorValues = commandUtility.buildError('missingParameter', 'missing param')
    if len(cParams.keys()) > 1:
        errorValues = commandUtility.buildError('invalidParameterName',
                                                'unrecognized parameter')
    if errorValues is not None:
        responseValues = ("camera.delete", "error", errorValues)
        return commandUtility.buildResponse(responseValues)

    urlList = fileList
    if len(fileList) == 1:
        if "all" in fileList:
            urlList = __urlListHelper(c, 'ALL')
        elif "video" in fileList:
            urlList = __urlListHelper(c, 'VID')
        elif "image" in fileList:
            urlList = __urlListHelper(c, 'PIC')
    storagePath = c.getStoragePath()
    print(urlList)
    for u in urlList:
        if '/' not in u:
            u = os.path.join(storagePath, u)
        else:
            u = os.path.join(storagePath, u.split('/')[-2])
        try:
            rmtree(u)
        except OSError:
            errorValues = commandUtility.buildError("invalidParameterValue",
                                                    u + " does not exist")
            responseValues = ("camera.delete", "error", errorValues)
            return commandUtility.buildResponse(responseValues)
    responseValues = ("camera.delete", "done", {"fileUrls": []})
    return commandUtility.buildResponse(responseValues)


def setOptions(c, cParams):
    errorValues = None
    print(cParams)
    try:
        options = cParams["options"]
    except KeyError:
        errorValues = commandUtility.buildError('missingParameter', 'missing param')
    if len(cParams.keys()) > 1:
        errorValues = commandUtility.buildError('invalidParameterName',
                                                'unrecognized param(s)')
    if errorValues is not None:
        responseValues = ("camera.setOptions", "error", errorValues)
        return commandUtility.buildResponse(responseValues)

    currentOptions = __loadJsonFile('currentOptions.json')

    errorMsg = "Invalid option/option value"

    if "exposureProgram" in options.keys():
        if options["exposureProgram"] in currentOptions["exposureProgramSupport"]:
            getattr(changeOptions, "exposureProgram")(c, options["exposureProgram"], currentOptions)
            currentOptions["exposureProgram"] = options["exposureProgram"]
            if currentOptions["exposureProgram"] == 2:
                currentOptions["iso"] = 0
                currentOptions["shutterSpeed"] = 0
            print("exposureProgram changed")
        else:
            errorMsg = 'Unsupported exposure program'
            errorValues = ("camera.setOptions", "error",
                           commandUtility.buildError('invalidParameterValue', errorMsg))
            return commandUtility.buildResponse(errorValues)
        del options["exposureProgram"]
    if "captureMode" in options.keys():
        if options["captureMode"] in currentOptions["captureModeSupport"]:
            currentOptions["captureMode"] = options["captureMode"]
            if options["captureMode"] in ["image", "interval"]:
                pictureDict = {"type": "jpeg", "width": 8192, "height": 4096}
                print("changed to jpeg")
                if ["captureMode"] == "interval":
                    currentOptions["captureInterval"] = 2
                    currentOptions["captureNumber"] = 0
                    currentOptions["photoStitching"] = "none"
                else:
                    currentOptions["photoStitching"] = "ondevice"
                currentOptions["fileFormat"] = pictureDict
            elif options["captureMode"] in ["video"]:
                videoDict = {"type": "mp4", "width": 3840, "height": 1920,
                             "framerate": 30}
                print("changed to mp4")
                currentOptions["fileFormat"] = videoDict
                currentOptions["videoStitching"] = True

        else:
            errorMsg = 'Unsupported capture mode'
            errorValues = ("camera.setOptions", "error",
                           commandUtility.buildError('invalidParameterValue', errorMsg))
            return commandUtility.buildResponse(errorValues)
        del options["captureMode"]

    if len(options) > 0:
        for optionKey in options.keys():
            optionValue = options[optionKey]
            print("Changing " + optionKey + " to:", optionValue)
            response = getattr(changeOptions, optionKey)(c, optionValue, currentOptions)
            print(response)
            if response == '':
                currentOptions[optionKey] = optionValue
            else:
                responseValues = ("camera.setOptions", "error", response)
                return commandUtility.buildResponse(responseValues)

    fileFormat = currentOptions["fileFormat"]
    if fileFormat["type"] == "dng":
        currentOptions["photoStitching"] = "none"
    captureMode = currentOptions["captureMode"]
    exposureProgram = currentOptions["exposureProgram"]
    exposureSettings = [currentOptions["shutterSpeed"], currentOptions["iso"]]

    formatConflict = (fileFormat["type"] in ["jpeg", "dng"] and captureMode == "video") or (fileFormat["type"] in ["mp4"] and captureMode in ["image", "interval"])
    autoConflict = exposureProgram == 2 and not exposureSettings == [0, 0]
    manualConflict = exposureProgram == 1 and (0 in exposureSettings)

    if manualConflict:
        errorMsg = 'Must enter all exposure values in manual mode'
    if formatConflict:
        errorMsg = 'Format unsupported in current capture mode'
    if autoConflict:
        errorMsg = 'Cannot exposure values must be auto in auto mode'

    if autoConflict or manualConflict:
        errorValues = ("camera.setOptions", "error",
                       commandUtility.buildError('invalidParameterValue', errorMsg))
        return commandUtility.buildResponse(errorValues)

    if captureMode == 'video':
        if fileFormat['width'] > 3840 or fileFormat['framerate'] > 30:
            currentOptions['videoStitching'] = 'none'
        else:
            currentOptions['videoStitching'] = 'ondevice'
    if "exposureCompensation" in options.keys() or "exposureProgram" in options.keys():
        _setOptionsBody = __genOptionsBody(__setExposure(currentOptions))
        results = c.listCommand(_setOptionsBody)
        for result in results:
            if "error" in result.keys():
                errorMsg = "camera error changing option"
                errorValues = ("camera.setOptions", "error",
                               commandUtility.buildError('invalidParameterValue', errorMsg))
                return commandUtility.buildResponse(errorValues)
    with open('currentOptions.json', 'w') as newOptions:
        json.dump(currentOptions, newOptions)
    return ''


def getOptions(c, cParams):
    errorValues = None
    try:
        optionNames = cParams["optionNames"]
    except KeyError:
        errorValues = commandUtility.buildError('missingParameter',
                                                'Missing input parameter')
    if len(cParams.keys()) > 2:
        errorValues = commandUtility.buildError('invalidParameterName',
                                                'invalid param name')
    if errorValues is not None:
        responseValues = ("camera.getOptions", "error", errorValues)
        return commandUtility.buildResponse(responseValues)

    currentOptions = __loadJsonFile('currentOptions.json')
    responseDict = {}
    for option in optionNames:
        if option not in list(currentOptions.keys()):
            responseValues = ("camera.getOptions", "error",
                              commandUtility.buildError('invalidParameterValue',
                                                        'Option does not exist'))
            response = commandUtility.buildResponse(responseValues)
            return response
        try:
            responseDict[option] = getattr(findOptions, option)(c)
            if responseDict[option] == "error":
                responseValues = ("camera.getOptions", "error",
                                  commandUtility.buildError('invalidParameterValue',
                                                            'Invalid option value'))
                response = commandUtility.buildResponse(responseValues)
                return response
        except AttributeError:
            responseDict[option] = currentOptions[option]

    results = {"options": responseDict}
    return commandUtility.buildResponse(("camera.getOptions", "done", results))


def reset(c):
    return commandUtility.commandBoilerplate("camera._reset",
                                             "camera.reset")


def switchWifi(c, cParams):
    errorValues = None
    try:
        preSharedKey = cParams["preSharedKey"]
        wifiSsid = cParams["wifiSsid"]
        wifiPwd = cParams["wifiPwd"]
    except KeyError:
        errorValues = commandUtility.buildError('missingParameter',
                                                'Missing input parameter')
    if len(cParams.keys()) > 3:
        errorValues = commandUtility.buildError('invalidParameterName',
                                                'unrecognized parameter(s)')
    if errorValues is not None:
        responseValues = ("camera.switchWifi", "error", errorValues)
        return commandUtility.buildResponse(responseValues)

    responseValues = ("camera.switchWifi", "error",
                      commandUtility.buildError('disabledCommand',
                                                'Currently unsupported'))
    return commandUtility.buildResponse(responseValues)


def uploadFile(c, cParams):
    errorValues = None
    try:
        fileUrl = cParams["fileUrl"]
        uploadUrl = cParams["uploadUrl"]
        accessToken = cParams["accessToken"]
    except KeyError:
        errorValues = commandUtility.buildError('missingParameter',
                                                'missing param')
    if len(cParams.keys()) > 3:
        errorValues = commandUtility.buildError('invalidParameterName',
                                                'unrecognized param(s)')
    if errorValues is not None:
        responseValues = ("camera.uploadFile", "error", errorValues)
        return commandUtility.buildResponse(responseValues)

    fileUri = '/' + '/'.join(fileUrl.split('/')[-5:])
    folderName = fileUrl.split('/')[-2]
    response = None

    try:
        dirFiles = os.listdir(fileUri)
    except OSError:
        with open(fileUri, 'rb') as f:
            response = requests.post(uploadUrl, files={folderName: f})
            return ''
    for fileName in dirFiles:
        if fileName in ["pano.jpg", "pano.mp4"]:
            print(fileName)
            fileUrl = os.path.join(fileUri, fileName)
            uploadHeader = {'Authorization': "Bearer " + accessToken,
                            'Content-Type': 'multipart/form-data'}

            with open(fileUrl, 'rb') as f:
                if fileName == "pano.jpg":
                    response = requests.post(uploadUrl,
                                             headers=uploadHeader,
                                             files={'file': f})
                    print(response)
                elif fileName == "pano.mp4":
                    response = requests.post(uploadUrl,
                                             headers=uploadHeader,
                                             data={'file': f})
            break
    if response == '':
        return response
    if response is None:
        errorMsg = "Stitched/processed file not found in given directory"
        errorValue = commandUtility.buildError("uploadError", errorMsg)
        responseValues = ("camera.uploadFile", "error", errorValue)
        return commandUtility.buildResponse(responseValues)
    elif "error" in response.keys():
        errorValue = commandUtility.buildError("uploadError", "camera error during upload")
        responseValues = ("camera.uploadFile", "error", errorValue)
        return commandUtility.buildResponse(responseValues)
    return ''


def __checkCameraError(results, commandName):
    if results["state"] == "error":
        errorMsg = results["error"]["description"]
        errorValue = commandUtility.buildError('disabledCommand', errorMsg)
        return (commandName, "error", errorValue)
    return None


def __getDate(fileName):
    return fileName.split('_', 1)[1]


def __loadJsonFile(fileName):
    with open(fileName) as miscFile:
        return json.load(miscFile)


def __setExposure(currentOptions):
    exposureOptions = {}
    if currentOptions["exposureProgram"] in [1, 2]:
        exposureOptions["ev_bias"] = (currentOptions["exposureCompensation"])*20
        exposureOptions["aaa_mode"] = currentOptions["exposureProgram"] - 1
    return exposureOptions


def __genOptionsBody(exposureOptions):
    _setOptionsJson = __loadJsonFile('_setOptionsTemplate.json')
    requestList = []
    for optionKey in exposureOptions.keys():
        _setOptionsJson["parameters"]["property"] = optionKey
        _setOptionsJson["parameters"]["value"] = exposureOptions[optionKey]
        requestList.append(json.dumps(_setOptionsJson))
    return requestList


def __urlListHelper(c, wanted):
    storagePath = c.getStoragePath()
    rawUrlList = os.listdir(storagePath)
    urlList = []
    for url in rawUrlList:
        if wanted != "ALL":
            if wanted == "VID" and "VID" in url:
                for f in os.listdir(storagePath + '/' + url):
                    if "mp4" in f:
                        urlList.append(url)
                        break
            elif wanted == "PIC" and ("VID" in url or "PIC" in url):
                for f in os.listdir(storagePath + '/' + url):
                    if "jpg" in f or "dng" in f:
                        urlList.append(url)
                        break
        else:
            if "VID" in url or "PIC" in url:
                urlList.append(url)
    return urlList
