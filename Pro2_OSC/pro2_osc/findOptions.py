import json
import os

queryStateBody = json.dumps({'name': 'camera._queryState'})


def totalSpace(c):
    currentOptions = __loadCurrentOptions()
    storagePath = c.getStoragePath()
    dirInfo = os.statvfs(storagePath)
    currentOptions["totalSpace"] = dirInfo[1] * dirInfo[2]
    __updateOptions(currentOptions)
    return currentOptions["totalSpace"]


def remainingSpace(c): # query state
    currentOptions = __loadCurrentOptions()
    storagePath = c.getStoragePath()
    dirInfo = os.statvfs(storagePath)
    currentOptions["remainingSpace"] = dirInfo[1] * dirInfo[4]
    __updateOptions(currentOptions)
    return currentOptions["remainingSpace"]


def remainingPictures(c): # query state
    currentOptions = __loadCurrentOptions()
    remainingSpace = currentOptions["remainingPictures"]
    return currentOptions["remainingPictures"]


def gpsInfo(c):
    return 'unsupported'


def remainingVideoSeconds(c):
    currentOptions = __loadCurrentOptions()
    if currentOptions["captureMode"] != "video":
        return "error"
    else:
        updatedValue = __getNativeCommand(c, "record", queryStateBody)["record"]["timeLeft"]
        return currentOptions["remainingVideoSeconds"]


def videoGPS(c):
    return 'unsupported'


def __updateOptions(options):
    with open('currentOptions.json', 'w') as newOptions:
        json.dump(options, newOptions)


def __getNativeCommand(c, param, bodyJson):
    response = c.command(bodyJson)
    return response['results'][param]


def __loadCurrentOptions():
    with open("currentOptions.json") as optionsFile:
        currentOptions = json.load(optionsFile)
    return currentOptions
