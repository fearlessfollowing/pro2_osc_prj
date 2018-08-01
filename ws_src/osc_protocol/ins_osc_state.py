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

# 获取文件
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


# 对于大卡: 需要手动计算出卡的容量
# 对于小卡: 直接使用查询到存储数据来填充卡信息
# 卡信息:
#   path   - 卡的挂载路径(对于外部TF卡，该项为NULL)
#   type   - 表示是设备的类型(本地还是模组的))
#   index  - 索引号(表示是第几号卡)
#   totoal - 卡的容量
#   free   - 卡的剩余空间
# 查询本地卡的信息
def get_local_storage_info(path, dev_type, dev_name, unit='M'):
    info = OrderedDict()
    division  = {
        'M':1024 *1024,
        'K':1024,
        'G':1024 *1024 * 1024,
        }
    pass

    info['type'] = dev_type   # 现在所有的设备类型都是USB，将该字段改为区分大卡和小卡
    info['mounttype'] = 'nv'
    info['path'] = path
    info['name'] = dev_name
    info['index'] = 0           # 对于本地卡，只支持一张的模式
    vfs = get_vfs(path)
    if vfs is not None:
        info['free'] = vfs.f_bsize * vfs.f_bfree / division[unit]
        info['total'] = vfs.f_bsize * vfs.f_blocks / division[unit]
    else:
        info['free'] = 0
        info['total'] = 0

    # 如果该路径下含有/.pro_suc文件，表示已经通过了测速
    if file_exist(join_str_list((path, "/.pro_suc"))):
        info['test'] = True
    else:
        info['test'] = False

    Print('internal info {}'.format(info))
    return info


# get_tf_storage_info
# 获取TF卡信息
def get_tf_storage_info(path, dev_type, dev_name, index, total, free):
    
    info = OrderedDict() 

    info['type']  = 'sd'   # 现在所有的设备类型都是USB，将该字段改为区分大卡和小卡
    info['mounttype'] = 'module'    
    info['path']  = path
    info['name']  = dev_name
    info['index'] = index           # 对于本地卡，只支持一张的模式

    info['free']  = free
    info['total'] = total
    info['test']  = False        # 没有进行速度测试，默认

    Print('external info {}'.format(info))
    return info


def get_dev_info_detail(dev_list):
    # Info('type {} count {} dev_list {}'.format(type(dev_list),len(dev_list),dev_list))
    dev_info = []
    try:
        for dev in dev_list:
            Info('dev is {}'.format(dev))
            info = get_local_storage_info(dev['path'], dev_type = dev['dev_type'], dev_name = dev['name'])

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
        
        # 小卡信息字典列表
        self._tf_info = []      # 外部TF设备列表
        self._local_dev = []    # 本地存储设备列表
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
            osc_state_handle.HANDLE_DEV_NOTIFY: self.handle_dev_notify_action,
            osc_state_handle.SET_TF_INFO:       self.set_tf_info,
            osc_state_handle.CLEAR_TF_INFO:     self.clear_tf_info,
            osc_state_handle.TF_STATE_CHANGE:   self.change_tf_info,

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


    def set_tf_info(self, dev_infos):
        Info('tf card info: {}'.format(dev_infos))
        #self._local_dev = dev_infos['storagePath']
        self._tf_info = dev_infos['module']
        for dev_info in self._tf_info:
            Info('tfcard dev info {}'.format(dev_info))


    # 方法名称: change_tf_info
    # 功能: 处理TF卡状态变化
    # 参数: params - 有变化的卡的信息(dict类型)
    # 返回值: 无
    def change_tf_info(self, params):
        Info('tf card change info: {}'.format(params))
        print(type(params))
        print(len(params))
        
        for tmp_dev in self._tf_info:
            if tmp_dev['index'] == params['index']:
                tmp_dev['storage_total'] = params['storage_total']
                tmp_dev['storage_left'] = params['storage_left']
        
        for debug_dev in self._tf_info:
            Info('tf info {}'.format(debug_dev))


    def clear_tf_info(self):
        Info('>>>> Query tf storage failed, clear tf info in osc_state....')
        _tf_info.clear()

    # vendor specifix: _usb_in -- usb inserted;_sd_in --sdcard inserted
    def get_osc_state(self, bStitch):
        state = OrderedDict()
        state['state'] = self.get_poll_info(bStitch)
        # print(' osc_state is ', osc_state)
        return dict_to_jsonstr(state)


    def set_external_info(self, dev_info):
        # Info('dev_info {} typed dev_info {}'.format(dev_info,type(dev_info)))
        try:
            #self.poll_info[EXTERNAL_DEV][EXTERNAL_ENTRIES] = dev_info
            self._local_dev = dev_info
        except Exception as e:
            Err('set_external_info exception {}'.format(e))
        # Info('set set_external_info info {} len dev_info {}'.format(self.poll_info,len(dev_info)))


    def set_save_path(self, content):
        try:
            self.poll_info[EXTERNAL_DEV]['save_path'] = content['path']
        except Exception as e:
            Err('set_save_path exception {}'.format(e))
        # Print('set_save_path info {}'.format(content))

    # 设置电池信息
    def set_battery_info(self,info):
        try:
            # Info('set battery info {}'.format(info))
            self.poll_info[BATTERY] = info
        except Exception as e:
            Err('set_battery_info exception {}'.format(e))
        # Print('set set_battery_info info {}'.format(self.poll_info))


    # 方法名称: handle_dev_notify_acttion
    # 功能: 处理存储设备变化通知
    # 参数: content - 存储设备列表
    # 返回值: 无
    def handle_dev_notify_action(self, content = None):
        dev_list = []
        if content is not None:
            dev_list = content['dev_list']
            # Info('2rec dev_list info {}'.format(dev_list))
        dev_info = get_dev_info_detail(dev_list)
        self.set_external_info(dev_info)


    def set_dev_speed_test_suc(self,path):
        try:
            #for dev in self.poll_info[EXTERNAL_DEV][EXTERNAL_ENTRIES]:
            for dev in self._local_dev:                
                if dev['path'] ==  path:
                    dev['test'] = True
                    break
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


    # 方法名称: check_storage_space
    # 功能：检查系统的存储空间（缓存的）
    # 参数：无
    # 返回值：无
    def check_storage_space(self):
        new_dev_info = []
        
        # 查询内部卡（大卡）
        for internal_dev_info in self._local_dev:            
            new_dev_info.append(get_local_storage_info(internal_dev_info['path'], dev_type = internal_dev_info['type'], dev_name = internal_dev_info['name']))

        # 查询外部卡（小卡）path, dev_type, dev_name, index, total, free
        for extern_dev_info in self._tf_info:
            new_dev_info.append(get_tf_storage_info('null', 'external', 'tfcard', extern_dev_info['index'], extern_dev_info['storage_total'], extern_dev_info['storage_left']))

        # 更新整个存储部分信息(大卡 + 小卡)
        self.poll_info[EXTERNAL_DEV][EXTERNAL_ENTRIES] = new_dev_info


    # 方法名称: get_poll_info
    # 功能：获取轮询信息（作为心跳包的数据）
    # 参数：bStitch - 是否Stitch状态
    # 返回值: 轮询信息（结构为字典）
    def get_poll_info(self, bStitch):

        self.aquire_sem()
        try:
            # 获取Camera当前的状态
            st = self.poll_info[CAM_STATE]
            
            # 检查存储空间
            self.check_storage_space()
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

    def set_tl_count(self, count):
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
            self.poll_info[SND_STATE] = param
            #self.poll_info[SND_STATE] = json.loads(param)
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
    SET_TF_INFO = 8
    CLEAR_TF_INFO = 9
    TF_STATE_CHANGE = 10

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