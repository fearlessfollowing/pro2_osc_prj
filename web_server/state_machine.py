# -*- coding: UTF-8 -*-
# 文件名：  state_machine.py 
# 版本：    V1.0.1
# 修改记录：
# 日期              修改人                  版本            备注
# 2018年9月4日      skymixos                V0.2.19
#

import os
import threading
import queue
from collections import OrderedDict
from util.ins_util import *
import config
from util.log_util import *
from util.time_util import *
from threading import Semaphore
from osc_protocol.ins_osc_state import osc_state_handle

# 状态机器类
# 用于查询Server的当前状态及允许的操作
class StateMachine:
    # @classmethod
    # def check_in_process(self):
    #     if self._cam_state & config.STATE_TAKE_CAPTURE_IN_PROCESS == config.STATE_TAKE_CAPTURE_IN_PROCESS or config.STATE_COMPOSE_IN_PROCESS == self._cam_state:
    #         return True
    #     else:
    #         return False
    #

    # @classmethod
    # def check_in_picture(self):
    #     if config.STATE_TAKE_CAPTURE_IN_PROCESS == self.get_cam_state():
    #         return True
    #     else:
    #         return False

    @classmethod
    def getCamState(cls):
        return osc_state_handle.get_cam_state()

    @classmethod
    def getCamStateFormatHex(cls):
        return hex(osc_state_handle.get_cam_state())

    @classmethod
    def setCamState(cls, state):
        osc_state_handle.set_cam_state(state)


    @classmethod
    def check_in_compose(cls):
        if config.STATE_COMPOSE_IN_PROCESS == StateMachine.getCamState():
            return True
        else:
            return False

    @classmethod
    def check_in_rec(cls):
        if StateMachine.getCamState() & config.STATE_RECORD == config.STATE_RECORD:
            return True
        else:
            return False

    @classmethod
    def checkInPreviewState(cls):
        if StateMachine.getCamState() & config.STATE_PREVIEW == config.STATE_PREVIEW:
            return True
        else:
            return False

    @classmethod
    def check_in_test_speed(cls):
        if StateMachine.getCamState() & config.STATE_SPEED_TEST == config.STATE_SPEED_TEST:
            return True
        else:
            return False


    @classmethod
    def check_in_qr(cls):
        if (StateMachine.getCamState() & config.STATE_START_QR == config.STATE_START_QR):
            return True
        else:
            return False

    @classmethod
    def check_in_live(cls):
        if (StateMachine.getCamState() & config.STATE_LIVE == config.STATE_LIVE):
            return True
        else:
            return False

    @classmethod
    def check_in_live_connecting(cls):
        if StateMachine.getCamState() & config.STATE_LIVE_CONNECTING == config.STATE_LIVE_CONNECTING:
            return True
        else:
            return False

    @classmethod
    def check_allow_pic(cls):
        Info('>>>> check_allow_pic, cam state {}'.format(StateMachine.getCamState()))
        if (StateMachine.getCamState() in (config.STATE_IDLE, config.STATE_PREVIEW)):
            return True
        else:
            return False

    @classmethod
    def check_allow_compose(cls):
        if StateMachine.getCamState() == config.STATE_IDLE:
            return True
        else:
            return False

    @classmethod
    def check_allow_rec(cls):
        if StateMachine.getCamState() in (config.STATE_IDLE, config.STATE_PREVIEW):
            return True
        else:
            return False

    @classmethod
    def checkAllowPreview(cls):
        if (StateMachine.getCamState() & config.STATE_PREVIEW) != config.STATE_PREVIEW:
            Info('>>>>>>>>>>>>>>>>>>>> check_allow_preview: current cam state {}'.format(StateMachine.getCamStateFormatHex()))
            allow_state = [config.STATE_IDLE, config.STATE_RECORD, config.STATE_LIVE, config.STATE_LIVE_CONNECTING]
            for st in allow_state:
                if StateMachine.getCamState() == st:                    
                    return True
            else:
                return False
        else:
            return False

    @classmethod
    def check_allow_live(cls):
        if StateMachine.getCamState() in (config.STATE_IDLE, config.STATE_PREVIEW):
            return True
        else:
            return False

    @classmethod
    def checkAllowListFile(cls):
        Info('>>>> check_allow_list_file, cam state {}'.format(StateMachine.getCamState()))
        if (StateMachine.getCamState() in (config.STATE_IDLE, config.STATE_PREVIEW)):
            return True
        else:
            return False

    @classmethod
    def addServerState(cls, state):
        Info('---> before add, server state is {}'.format(StateMachine.getCamStateFormatHex()))
        StateMachine.setCamState(StateMachine.getCamState() | state)
        Info('---> after add, server state is {}'.format(StateMachine.getCamStateFormatHex()))

    @classmethod
    def rmServerState(cls, state):
        Info('---> before rm, server state is {}'.format(StateMachine.getCamStateFormatHex()))
        StateMachine.setCamState(StateMachine.getCamState() & ~state)
        Info('---> after rm, server state is {}'.format(StateMachine.getCamStateFormatHex()))
           