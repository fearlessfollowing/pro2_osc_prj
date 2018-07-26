import os
import threading
import queue
from collections import OrderedDict
from util.ins_util import *
import config
from util.log_util import *
from util.time_util import *
from threading import Semaphore

EXTERNAL_DEV = '_external_dev'
EXTERNAL_ENTRIES='entries'
SD1 = 'sd1'
SD2 = 'sd2'
USB = 'usb'
BATTERY = '_battery'
ID_RES = '_idRes'
TL_INFO = '_tl_info'
# PIC_RES = '_picRes'
# REC_RES = '_recRes'
# NET = '_net'
INTERVAL = 10000
STORAGE_POS = 1
CAM_STATE = '_cam_state'
GPS_STATE = '_gps_state'
SND_STATE = '_snd_state'

sem_vfs = Semaphore()

def get_vfs(name):
    # Print('get_vfs {}'.format(name))
    vfs = None
    sem_vfs.acquire()
    try:
        if os.path.exists(name) and os.path.isdir(name):
            vfs = os.statvfs(name)
        # Print('2get_vfs {} '.format(name))
    except OSError as e:
        Err('get_vfs OSError {} name {}'.format(e, name))
    except Exception as e:
        Err('get_vfs exception {}'.format(e, name))
    sem_vfs.release()
    # Print('3get_vfs {}'.format(name))
    return vfs

def get_storage_info(path,dev_type, dev_name,unit='M'):
    info = OrderedDict()
    division  = {
                'M':1024 *1024,
                'K':1024,
                'G':1024 *1024 * 1024,
                 }
    info['type'] = dev_type
    info['path'] = path
    info['name'] = dev_name
    vfs = get_vfs(path)
    if vfs is not None:
        info['free'] = vfs.f_bsize * vfs.f_bfree / division[unit]
        info['total'] = vfs.f_bsize * vfs.f_blocks / division[unit]
    else:
        info['free'] = 0
        info['total'] = 0
    if file_exist(join_str_list((path, "/.pro_suc"))):
        info['test'] = True
    else:
        info['test'] = False
    # Print('info {}'.format(info))
    return info;

def get_dev_info_detail(dev_list):
    # Info('type {} count {} dev_list {}'.format(type(dev_list),len(dev_list),dev_list))
    dev_info = []
    try:
        for dev in dev_list:
            # Info('dev is {}'.format(dev));
            info = get_storage_info(dev['path'],dev_type = dev['dev_type'],dev_name = dev['name'])
            dev_info.append(info)
    except Exception as e:
        Info('get_dev_info_detail exception {}'.format(str(e)))
    # Info('2type {} count {} dev_list {}'.format(type(dev_list), len(dev_list), dev_list))
    return dev_info

class osc_state(threading.Thread):
    def __init__(self, queue):
        threading.Thread.__init__(self)
        self._queue = queue
        self._exit = False
        self.poll_info = OrderedDict({BATTERY: {},
                                 ID_RES: [],
                                 EXTERNAL_DEV: {EXTERNAL_ENTRIES: [],'save_path':None},
                                 TL_INFO: {},
                                 CAM_STATE: config.STATE_IDLE,
                                 GPS_STATE:0,
                                      SND_STATE:{}})
        self.sem = Semaphore()

    def run(self):
        self.func = OrderedDict({
            osc_state_handle.CLEAR_TL_COUNT:    self.clear_tl_count,
            osc_state_handle.RM_RES_ID:         self.rm_res_id,
            osc_state_handle.SET_DEV_SPEED_SUC: self.set_dev_speed_test_suc,
            osc_state_handle.SET_TL_COUNT:      self.set_tl_count,
            osc_state_handle.ADD_RES_ID:        self.add_res_id,
            osc_state_handle.HAND_SAVE_PATH:    self.handle_save_path_change,
            osc_state_handle.HANDLE_BAT:        self.handle_battery,
            osc_state_handle.HANDLE_DEV_NOTIFY: self.handle_dev_notify_action
        })

        while self._exit is False:
            try:
                req = self._queue.get()
                # Info('rec osc req {}'.format(req))
                msg_what = req['msg_what']
                self.aquire_sem()
                if check_dic_key_exist(req,'args'):
                    self.func[msg_what](req['args'])
                else:
                    self.func[msg_what]()
                self.release_sem()
            except Exception as e:
                Err('monitor_fifo_write2 e {}'.format(e))
                self.release_sem()

    # vendor specifix: _usb_in -- usb inserted;_sd_in --sdcard inserted
    def get_osc_state(self,bStitch):
        state = OrderedDict()
        state['state'] = self.get_poll_info(bStitch)
        # print(' osc_state is ', osc_state)
        return dict_to_jsonstr(state)

    def set_external_info(self,dev_info):
        # Info('dev_info {} typed dev_info {}'.format(dev_info,type(dev_info)))
        try:
            self.poll_info[EXTERNAL_DEV][EXTERNAL_ENTRIES] = dev_info
        except Exception as e:
            Err('set_external_info exception {}'.format(e))
        # Info('set set_external_info info {} len dev_info {}'.format(self.poll_info,len(dev_info)))

    def set_save_path(self,content):
        try:
            self.poll_info[EXTERNAL_DEV]['save_path'] = content['path']
        except Exception as e:
            Err('set_save_path exception {}'.format(e))
        # Print('set_save_path info {}'.format(content))

    def set_battery_info(self,info):
        try:
            # Info('set battery info {}'.format(info))
            self.poll_info[BATTERY] = info
        except Exception as e:
            Err('set_battery_info exception {}'.format(e))
        # Print('set set_battery_info info {}'.format(self.poll_info))

    def handle_dev_notify_action(self,content = None):
        dev_list = []
        if content is not None:
            dev_list = content['dev_list']
            # Info('2rec dev_list info {}'.format(dev_list))
        #/sdcard no need
        #dev_list.append(OrderedDict({'path':config.ADD_STORAGE,'dev_type':'internal','name':'sdcard'}))
        # Info(' dev_list is {}'.format(dev_list))
        dev_info = get_dev_info_detail(dev_list)
        # Info('rec dev_info  {}'.format(dev_info))
        self.set_external_info(dev_info)
        # Info('2rec dev_info  {}'.format(dev_info))

    def set_dev_speed_test_suc(self,path):
        try:
            for dev in self.poll_info[EXTERNAL_DEV][EXTERNAL_ENTRIES]:
                if dev['path'] ==  path:
                    dev['test'] = True
                    break;
            else:
                Info('not found speed test path {}'.format(path))
        except Exception as e:
            Err('set_dev_speed_test_suc exception {}'.format(e))

    def handle_save_path_change(self,content):
        self.set_save_path(content)

    def handle_battery(self,content):
        self.set_battery_info(content)

    def aquire_sem(self):
        self.sem.acquire()

    def release_sem(self):
        self.sem.release()

    def check_storage_space(self):
        new_dev_info = []
        # Info('self.poll_info[EXTERNAL_DEV][entries] is {} type {}'.format(self.poll_info[EXTERNAL_DEV][EXTERNAL_ENTRIES],type(self.poll_info[EXTERNAL_DEV][EXTERNAL_ENTRIES])))
        for dev_info in self.poll_info[EXTERNAL_DEV][EXTERNAL_ENTRIES]:
            new_dev_info.append(get_storage_info(dev_info['path'],dev_type = dev_info['type'],dev_name = dev_info['name']))
        # Info('new_dev_info is {}'.format(new_dev_info))
        self.poll_info[EXTERNAL_DEV][EXTERNAL_ENTRIES] = new_dev_info

    def get_poll_info(self,bStitch):
        # Info('get_poll_info start')
        self.aquire_sem()
        try:
            # Print('1get pollinfo is {}'.format(self.poll_info))
            st = self.poll_info[CAM_STATE]
            #including rec and live_rec 170901
            #rechange for update storage every time 180119
            # if (st & config.STATE_RECORD) == config.STATE_RECORD or (st & config.STATE_LIVE) == config.STATE_LIVE or bStitch is True:
            self.check_storage_space()
                # Print('get pollinfo is {} st {}'.format(self.poll_info, st))
        except Exception as e:
            Err('get_poll_info exception {}'.format(e))
        info = self.poll_info
        self.release_sem()

        return info

    def add_res_id(self,id):
        try:
            # Info('add res is {}'.format(id))
            self.poll_info[ID_RES].append(id)
            # Info('poll_info {}'.format(self.poll_info))
        except Exception as e:
            Err('add_res_id exception {}'.format(e))

    def set_tl_count(self,count):
        try:
            # Info('add tl_info {}'.format(count))
            self.poll_info[TL_INFO]['tl_count'] = count
            # Info('poll_info {}'.format(self.poll_info))
        except Exception as e:
            Err('add_res_id exception {}'.format(e))

    def clear_tl_count(self):
        try:
            # Info('clear_tl_count tl_info')
            self.poll_info[TL_INFO] = {}
        except Exception as e:
            Err('add_res_id exception {}'.format(e))

    def rm_res_id(self,id):
        try:
            Info('rm res is {}'.format(id))
            self.poll_info[ID_RES].remove(id)
        except Exception as e:
            Err('rm_res_id exception {}'.format(e))

    def get_cam_state(self):
        self.aquire_sem()
        try:
            state = self.poll_info[CAM_STATE]
        except Exception as e:
            Err('get_cam_state exception {}'.format(e))
            state = config.STATE_EXCEPTION
        self.release_sem()
        return state

    def get_save_path(self):
        self.aquire_sem()
        try:
            path = self.poll_info[EXTERNAL_DEV]['save_path']
        except Exception as e:
            Err('get_cam_state exception {}'.format(e))
            path = None
        self.release_sem()
        return path

    def set_cam_state(self,state):
        # Info('set cam state {}'.format(state))
        try:
            self.poll_info[CAM_STATE] = state
        except Exception as e:
            Err('set_cam_state exception {}'.format(e))
        # Info('2set cam state {}'.format(state))

    def set_gps_state(self,state):
        try:
            self.poll_info[GPS_STATE] = state
        except Exception as e:
            Err('set_gps_state exception {}'.format(e))

    # 设置_snd_state
    def set_snd_state(self,param):
        try:
            # 将字符串转换为json对象, 2018年7月26日（修复BUG）
            #self.poll_info[SND_STATE] = param
            self.poll_info[SND_STATE] = json.loads(param)
        except Exception as e:
            Err('set_snd_state exception {}'.format(e))

    def stop(self):
        Print('stop osc state')
        self._exit = True

OSC_STATE_QUEUE_SIZE = 20
class osc_state_handle:
    _queue = queue.Queue(OSC_STATE_QUEUE_SIZE)
    _osc_state = osc_state(_queue)
    CLEAR_TL_COUNT = 0
    RM_RES_ID = 1
    SET_DEV_SPEED_SUC = 2
    SET_TL_COUNT = 3
    ADD_RES_ID = 4
    HAND_SAVE_PATH = 5
    HANDLE_BAT = 6
    HANDLE_DEV_NOTIFY = 7

    @classmethod
    def start(cls):
        cls._osc_state.start()

    @classmethod
    def stop(cls):
        cls._osc_state.stop()
        cls._osc_state.join()

    @classmethod
    def get_osc_state(cls,bStitch):
        return cls._osc_state.get_osc_state(bStitch)

    @classmethod
    def get_cam_state(cls):
        return cls._osc_state.get_cam_state()

    @classmethod
    def set_cam_state(cls,st):
        cls._osc_state.set_cam_state(st)

    @classmethod
    def set_gps_state(cls,st):
        cls._osc_state.set_gps_state(st)

    @classmethod
    def set_snd_state(cls,st):
        cls._osc_state.set_snd_state(st)

    @classmethod
    def get_save_path(cls):
        return cls._osc_state.get_save_path()

    @classmethod
    def make_req(cls, cmd, args=None):
        dic = OrderedDict()
        dic['msg_what'] = cmd
        if args is not None:
            dic['args'] = args
        return dic

    @classmethod
    def send_osc_req(cls, req):
        # Info('osc req {}'.format(req))
        if cls._queue._qsize() < OSC_STATE_QUEUE_SIZE:
            cls._queue.put(req)
        else:
            # avoid full
            Warn('osc_state_handle write queue exceed')