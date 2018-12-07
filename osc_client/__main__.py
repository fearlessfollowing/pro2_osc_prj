######################################################################################################
# -*- coding: UTF-8 -*-
# 文件名：  __main__.py 
# 版本：    V1.0.1
# 修改记录：
# 日期              修改人                  版本            备注
######################################################################################################

# 操作流程
# 1.请求/osc/info
# 2.请求/osc/state
# 3.通过/osc/commands/execute发命令
# 3.1 getOptions
# 3.2 setOptions
# 3.3 getLivePreview

import os
import platform
import json
import time
import base64
import config

from flask import Flask, request, session, g, redirect, url_for, abort, \
    render_template, flash, make_response, Response,send_file,jsonify,send_from_directory


from functools import wraps
from collections import OrderedDict
from werkzeug.utils import secure_filename

import commands


gCommandUtil = CommandsUtil()


def main():
    Info('[------ start osc client ----]')
    gCommandUtil.setServerIp('http://192.168.1.1:80')

    oscInfo = gCommandUtil.getOscInfo()
    if oscInfo != None:
        print('get osc info ret: ', oscInfo)
    else:
        print('Failed: get osc info.')


    oscState = gCommandUtil.getOscState()
    if oscState != None:
        print('get osc state ret: ', oscState)
    else:
        print('Failed: get osc state.')


main()


