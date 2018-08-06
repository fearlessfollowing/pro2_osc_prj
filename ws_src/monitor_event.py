# -*- coding:utf-8 -*-
import os
import threading
import queue
import json
from util.str_util import *
from util.log_util import *
from util.ins_util import *
from util.fifo_util import *
from util import time_util
from collections import OrderedDict
import config
import platform
from osc_protocol.ins_osc_state import osc_state_handle

HEAD_LEN = 8
CON_LEN_OFF  = HEAD_LEN - 4

#read from pro service
EVENT_BATTERY = 0
EVENT_NET_CHANGE = 1
EVENT_OLED_KEY = 2
EVENT_DEV_NOTIFY = 3
EVENT_SAVE_PATH = 4
EVENT_AGEING_TEST = 5



#write to pro service
# CMD_OLED_DISP_EXT = 1
# CMD_OLED_KEY_RES = 2
CMD_OLED_DISP_TYPE = 0
CMD_OLED_SYNC_INIT = 1

CMD_OLED_DISP_TYPE_ERR = 16
CMD_OLED_POWER_OFF = 17
CMD_OLED_SET_SN = 18
CMD_CONFIG_WIFI = 19



MAX_QUEUE_SIZE = 20
class mointor_fifo_write_handle:
    def __init__(self):
        self._queue = queue.Queue(MAX_QUEUE_SIZE)
        self._write_thread = monitor_fifo_write(self._queue)

    def start(self):
        self._write_thread.start()

    def stop(self):
        self._write_thread.stop()
        self._write_thread.join()

    def send_req(self,req):
        if self._queue._qsize() < MAX_QUEUE_SIZE:
            self._queue.put(req)
        else:
            #avoid full
            Warn('monitor write queue exceed')



class monitor_fifo_write(threading.Thread):
    def __init__(self, queue):
        threading.Thread.__init__(self)
        self._queue = queue
        self._exit = False
        self._write_fd = -1
        if platform.machine() == 'x86_64' or platform.machine() == 'aarch64' or file_exist('/sdcard/http_local_monitor'):
            if file_exist(config.MONITOR_FIFO_WRITE) is False:
                os.mkfifo(config.MONITOR_FIFO_WRITE)

    def get_write_fd(self):
        if self._write_fd == -1:
            self._write_fd = fifo_wrapper.open_write_fifo(config.MONITOR_FIFO_WRITE)

    def close_write_fd(self):
        if self._write_fd != -1:
            fifo_wrapper.close_fifo(self._write_fd)
            self._write_fd = -1;

    def start_write(self, cmd, req):
        if platform.machine() != 'x86_64' or file_exist('/sdcard/http_local_monitor') is False:
            
            # 将字典数据转换为json串
            content = json.dumps(req)
            Info('start_write conent {}'.format(content))
            contet_len = len(content)
            bytes_cmd = int_to_bytes(cmd)
            bytes_content_len = int_to_bytes(contet_len)
            bytes_content = str_to_bytes(content)

            # Print('write {} {} {}'.format(bytes_cmd,bytes_content_len,bytes_content))
            content = join_byte_list((bytes_cmd, bytes_content_len,bytes_content))
            
            # contet_len = len(content)
            write_len = fifo_wrapper.write_fifo(self._write_fd,content)
            # Print('fifo monitor write req:{} content len {} write len {}'.format(content, contet_len, write_len))
            # assert_match(write_len, contet_len)
        else:
            Info('x86 rec req {}'.format(req))


    def handle_disp_oled_type(self,req):
        #just reopen fifo write fd
        if req['type'] == config.WRITE_FOR_BROKEN:
            # Info('rec monitor_fifo_write write broken ')
            self.close_write_fd()
        else:
            # Info('handle_disp_oled_type req[type] is {}'.format(req['type']))
            self.start_write(CMD_OLED_DISP_TYPE, req)

    def handle_disp_oled_type_err(self,req):
        Info('disp oled err req {}'.format(req))
        self.start_write(CMD_OLED_DISP_TYPE_ERR, req)

    def handle_set_wifi_config(self,req):
        self.start_write(CMD_CONFIG_WIFI, req)

    def handle_sync_init(self,req):
        self.start_write(CMD_OLED_SYNC_INIT, req)

    def handle_set_sn(self,req):
        self.start_write(CMD_OLED_SET_SN,req)

    # def handle_power_off(self,req):
    #     self.start_write(CMD_OLED_POWER_OFF, req)

    def run(self):
        self.func = OrderedDict({
            # config.OLED_DISP_STR:self.handle_disp_oled_str,
            # config.OLED_DISP_EXT:self.handle_disp_oled_ext,
            # config.OLED_KEY_RES:self.handle_oled_key_res,
            config.OLED_DISP_TYPE_ERR:  self.handle_disp_oled_type_err,
            config.OLED_DISP_TYPE:      self.handle_disp_oled_type,
            config.OLED_SET_SN:         self.handle_set_sn,
            # config.OLED_POWER_OFF:    self.handle_power_off,
            config.OLED_CONIFIG_WIFI:   self.handle_set_wifi_config,
            config.OLED_SYNC_INIT:      self.handle_sync_init
        })
        while self._exit is False:
            try:
                # Print('monitor_fifo_write open')
                self.get_write_fd()
                req = self._queue.get()
                # Info('1rec write fifo req {}'.format(req))
                msg_what = req['msg_what']
                # try:
                # Print('monitor_fifo_write name {} args {}'.format(msg_what, req['args']))
                # Print('self.switcher {0}'.format(self.switcher))
                self.func[msg_what](req['args'])
                # except Exception as e:
                #     Err("monitor_fifo_write1 name {} error {}".format(msg_what,e))
            except Exception as e:
                Err('monitor_fifo_write2 e {}'.format(e))
                #fifo will close while rec fifo broken msg sent from monitor_fifo_read
                #self.close_write_fd()

        if self._write_fd:
            self._write_fd.close()
            self._write_fd = None

    def stop(self):
        Print('stop monitor?')
        self._exit = True




#
# 监听来自pro_servcie的消息
#
class monitor_fifo_read(threading.Thread):
    def __init__(self, controller):
        threading.Thread.__init__(self)
        self.read_fd = -1
        self.write_fd = -1
        self._exit = False
        if platform.machine() == 'x86_64' or platform.machine() == 'aarch64' or file_exist('/sdcard/http_local'):
            if file_exist(config.MONITOR_FIFO_READ) is False:
                os.mkfifo(config.MONITOR_FIFO_READ)
        self.control_obj = controller

    def get_read_fd(self):
        if self.read_fd == -1:
            self.read_fd = fifo_wrapper.open_read_fifo(config.MONITOR_FIFO_READ)
            # Info('monitor_fifo_read get read fd {}'.format(self.read_fd))

    def close_read_fd(self):
        if self.read_fd != -1:
            # Info('monitor_fifo_read close_fifo  read fd {}'.format(self.read_fd))
            fifo_wrapper.close_fifo(self.read_fd)
            self.read_fd = -1;

    def start_read(self,len):
        return fifo_wrapper.read_fifo(self.read_fd,len)

    # def handle_usb_event(self,content):
    #     Print('handler usb content {}'.format(content))
    #     osc_state.handle_usb_action(content)

    def handle_battery_event(self,content):
        osc_state_handle.send_osc_req(osc_state_handle.make_req(osc_state_handle.HANDLE_BAT,content))

    # def handle_net_change_event(self,content):
        # Print('handler net content {}'.format(content))
        #osc_state.handle_net_change(content)

    def handle_oled_key(self,content):
        # Print('handle oled content {}'.format(content))
        self.control_obj.handle_oled_key(content)

    def handle_dev_notify(self,content = None):
        if content is not None:
            # Print('handler dev notify content {} dev_list {}'.format(content,content['dev_list']))
            osc_state_handle.send_osc_req(osc_state_handle.make_req(osc_state_handle.HANDLE_DEV_NOTIFY, content))
        else:
            osc_state_handle.send_osc_req(osc_state_handle.make_req(osc_state_handle.HANDLE_DEV_NOTIFY))

    def handle_save_path(self,content):
        Print('handle_save_path content {}'.format(content))
        self.control_obj.start_change_save_path(content)

        # 检查路径文件上是否有factory.json文件
        # 如果文件存在,将解析该文件(提取老化的时长)
        # 给camerad发送录像请求, 通知oled_hander进入老化模式
        # content['path']
        #Print('path {}'.format(content['path']));
        if os.path.exists(content['path'] + '/factory.json'):
            Print('new path is exist')
            file_object = open(content['path'] + '/factory.json')
            file_context = file_object.read()
            file_object.close()

            # 将内容发送给Camerad
            # 给oled_hander发送ageing消息,进入老化界面
            file_json = json.loads(file_context)
            
            Print('file content: {}'.format(file_json))
            age_time = file_json['parameters']['duration']    # 得到老化的时间
            self.control_obj.start_ageing_test(file_json, age_time);
        else:
            Print('new storage path is not exist')
        

    def handle_ageing_test(self, content):
        self.control_obj.start_ageing_test(content);

    def run(self):
        self.func = OrderedDict({
            # EVENT_USB:            self.handle_usb_event,
            EVENT_BATTERY:          self.handle_battery_event,
            # EVENT_NET_CHANGE:     self.handle_net_change_event,
            EVENT_OLED_KEY:         self.handle_oled_key,
            EVENT_DEV_NOTIFY:       self.handle_dev_notify,
            EVENT_SAVE_PATH:        self.handle_save_path,
            EVENT_AGEING_TEST:      self.handle_ageing_test,
        })

        while self._exit is False:
            try:
                # Info('monitor_fifo_read pro open ')
                self.get_read_fd()
                # Info('monitor_fifo_read pro read ')
                header = self.start_read(HEAD_LEN)
                # Print(' read monitor header {} {} {}'.format(len(header), HEAD_LEN, header))
                # assert_match(len(header),HEAD_LEN)

                # Info('debug headers :')
                # i = 0
                # while i < HEAD_LEN:
                #     Info("{} ".format(header[i]))
                #     i += 1
                event = bytes_to_int(header,0)
                Print('monitor_fifo_read event {}'.format(event))
                content_len = bytes_to_int(header, CON_LEN_OFF)
                Info('content_len is {}'.format(content_len))
                if content_len > 0:
                    read_content = self.start_read(content_len)
                    Print('monitor fifo read_content is {} content_len {}'.format(read_content, content_len))
                    content = bytes_to_str(read_content)
                    
                    Print('len(content) {} content_len {} '.format(len(content),content_len))
                    #assert_match(len(content), content_len)

                    Print('read monitor content len {} content {}'.format(content_len,content))
                    self.func[event](jsonstr_to_dic(content))
                else:
                    self.func[event]()
            except Exception as e:
                #pro_service fifo error
                Err('monitor_fifo_read {}'.format(e))
                self.close_read_fd()
                self.control_obj.send_oled_type_wrapper(config.WRITE_FOR_BROKEN)
                # Err('monitor_fifo_read over {}'.format(e))

    def stop(self):
        Print('stop monitorread')
        self._exit = True



class monitor_camera_active_handle:
    def __init__(self, controller):
        self._write_thread = monitor_camera_active(controller)

    def start(self):
        self._write_thread.start()

    def stop(self):
        self._write_thread.stop()
        self._write_thread.join()

#read fifo info from
class monitor_camera_active(threading.Thread):
    def __init__(self, controller):
        threading.Thread.__init__(self)
        self.read_fd = -1
        self._exit = False
        if platform.machine() == 'x86_64' or platform.machine() == 'aarch64' or file_exist('/sdcard/http_local'):
            if file_exist(config.INS_ACTIVE_FROM_CAMERA) is False:
                os.mkfifo(config.INS_ACTIVE_FROM_CAMERA)
        self.control_obj = controller
        # Print('read self {0} read_fd {1}'.format(self,self._read_fd))

    def get_read_fd(self):
        if self.read_fd == -1:
            self.read_fd = fifo_wrapper.open_read_fifo(config.INS_ACTIVE_FROM_CAMERA)

    def close_read_fd(self):
        if self.read_fd != -1:
            # Info('monitor_camera_active close read fd {}'.format(self.read_fd))
            fifo_wrapper.close_fifo(self.read_fd)
            self.read_fd = -1;

    def start_read(self,len):
        return fifo_wrapper.read_fifo(self.read_fd,len)

    def handle_camera_notify(self,content):
        self.control_obj.handle_notify_from_camera(content)

    def run(self):
        while self._exit is False:
            try:
                # Info('monitor_camera active open ')
                self.get_read_fd()
                # Info('monitor_camera active read ')
                header = self.start_read(config.HEADER_LEN)
                # Print(' monitor_camera_active header {} {} {}'.format(len(header), config.HEADER_LEN, header))
                assert_match(len(header),config.HEADER_LEN)
                content_len = bytes_to_int(header, CON_LEN_OFF)
                if content_len > 0:
                    read_content = self.start_read(content_len)
                    Print('monitor_camera_active read_content is {} content_len {}'.format(read_content, content_len))
                    content = bytes_to_str(read_content)
                    assert_match(len(content), content_len)
                else:
                    Err(' camera active fifo len <= 0')
                # Print('read monitor content len {} content {}'.format(content_len,content))
                dic = jsonstr_to_dic(content)

                # 处理来自camerad的通知
                self.handle_camera_notify(dic)
                
            except Exception as e:
                #pro_service fifo error
                Err('monitor_camera_active Exception {}'.format(e))
                self.close_read_fd()
                Info('add reset notify while monitor active fifo disconnect')
                self.handle_camera_notify(OrderedDict({'name':config._RESET_NOTIFY}))
                #sometimes met fifo not created
                time_util.delay_ms(2000)
                Err('monitor_camera_active over {}'.format(e))

    def stop(self):
        Print('stop camera active')
        self._exit = True
