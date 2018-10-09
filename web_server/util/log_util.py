import logging
import config
import os
#20170223 could not import ins_util for util lay on log_util which need init firstly
from util.str_util import *
from util.time_util import get_local_time,get_local_date,get_log_date_time
#from util.timer_util import RepeatedTimer
from collections import OrderedDict
import shutil
from threading import Semaphore

D = logging.DEBUG
I = logging.INFO
W = logging.WARNING
E = logging.ERROR
C = logging.CRITICAL

l_level = D
log_wrapper = None
format_dict = {
D : logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s'),
I : logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s'),
W : logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s'),
E : logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s'),
C : logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
}

#2min
#LOG_TIMER_TO = 120 * 1000
#MBytes ,keep for update
#LIMIT_SIZE = 2048

NEW_LOG_LIMIT = 1024 * 1024 * 30

class L():
    #default logger tag to g
    def __init__(self, logger_name,loglevel = D):
        
     '''
        指定保存日志的文件路径，日志级别，以及调用文件
        将日志存入到指定的文件中
     '''

     # print('create logger {} '.format(logger))
     # 创建一个logger
     self.logger = logging.getLogger(logger_name)
     self.logger.setLevel(loglevel)

     # 再创建一个handler，用于输出到控制台
     ch = logging.StreamHandler()
     ch.setLevel(loglevel)

     # 定义handler的输出格式
    #formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
     # print(' int(loglevel) is ', int(loglevel))
     formatter = format_dict[int(loglevel)]
     ch.setFormatter(formatter)

     # # 创建一个handler，用于写入日志文件
     # if file_name is not None:
     #     fh = logging.FileHandler(file_name)
     #     #force to debug level
     #     fh.setLevel(D)
     #     fh.setFormatter(formatter)
     #     # 给logger添加handler
     #     self.logger.addHandler(fh)
     self.logger.addHandler(ch)

    def add_file_handler(self,file_name,loglevel = D):
        fh = logging.FileHandler(file_name)
        #force to debug level
        fh.setLevel(D)
        formatter = format_dict[int(loglevel)]
        fh.setFormatter(formatter)
        # 给logger添加handler
        self.logger.addHandler(fh)

    def getlog(self):
     return self.logger

def Print(*args):
    log_wrapper.debug(*args)

def Info(*args):
    log_wrapper.info(*args)

def Warn(*args):
    log_wrapper.warning(*args)

def Err(*args):
    log_wrapper.error(*args)

class class_log_timer:

    @classmethod
    def file_exist(cls, name):
        return os.path.exists(name)

    @classmethod
    def init(cls):
        cls.L_obj = L(loglevel=l_level, logger_name='g')
        global log_wrapper
        log_wrapper = cls.L_obj.getlog()

        # if cls.file_exist(config.LOG_ROOT) is False:
        #     os.mkdir(config.LOG_ROOT)
        # cls.log_path = cls.get_log_path()
        cls.log_file_name = config.LOG_FILE
        cls.timer = None
        # if check_space_full():
        #     cls.rm_old_log()
        #
        # if check_space_full():
        #     Info('full: no log file name')
        # else:

        # cls.log_file_name = cls.get_log_name(cls.log_path)
        # Info('log_path {} log_file_name {}'.format(cls.log_path, cls.log_file_name))
        # cls.log_obj = L(loglevel=l_level, logger='smtp', file_name=cls.log_file_name).getlog()
        if cls.file_exist(cls.log_file_name):
            file_size = os.path.getsize(cls.log_file_name)
            if file_size > NEW_LOG_LIMIT:
                #Info('rm log file size {} NEW_LOG_LIMIT {}'.format(file_size,NEW_LOG_LIMIT))
                os.remove(cls.log_file_name)
                os.mknod(cls.log_file_name)
        else:
            os.mknod(cls.log_file_name)

        cls.L_obj.add_file_handler(cls.log_file_name)
        
        # cls.timer = RepeatedTimer(LOG_TIMER_TO, cls.timeout_func, "log_timer", oneshot=False)
        # cls.start_timer()

    # @classmethod
    # def get_log_path(cls):
    #     # get log path but not create even if it not exist,for met no space error while created or deleted while judge full,
    #     # so create while create log_file
    #     path = join_str_list((config.LOG_ROOT, get_local_date()))
    #     return path

    # @classmethod
    # def get_log_name(cls,path):
    #     if cls.file_exist(path) is False:
    #         os.mkdir(path)
    #     file_name = join_str_list((path, '/log_', get_log_date_time()))
    #     return file_name

"""
    @classmethod
    def allow_rm(cls,name):
        bAllow = False
        if str_exits(name,config.LOG_ROOT) != -1:
            bAllow = True
        return bAllow

    @classmethod
    #remove today's log dir
    def rm_cur_dir(cls,path):
        file_lists = os.listdir(path)
        for f in file_lists:
            file_name = os.path.join(path, f)
            if os.path.isdir(file_name):
                Info('remove dir {}'.format(file_name))
                #confirm the path contain sdcard to avoid remove other key path
                if cls.allow_rm(file_name):
                    shutil.rmtree(file_name)
            else:
                if file_name != cls.log_file_name:
                    Info('remove file name {}'.format(file_name))
                    if cls.allow_rm(file_name):
                        os.remove(file_name)
                else:
                    Info('not remove log file name {} log_file_name {}'.format(file_name, cls.log_file_name))
    @classmethod
    def rm_old_log(cls):
        file_lists = os.listdir(config.LOG_ROOT)
        if len(file_lists) > 0:
            file_lists.sort()
            for f in file_lists:
                file_name = os.path.join(config.LOG_ROOT,f)
                if os.path.isdir(file_name):
                    if file_name != cls.log_path:
                        Info('remove path {} '.format(file_name))
                        if cls.allow_rm(file_name):
                            shutil.rmtree(file_name)
                    else:
                        cls.rm_cur_dir(file_name)
                else:
                    Info('remove file {}'.format(file_name))
                    if cls.allow_rm(file_name):
                        os.remove(file_name)
                if check_space_full() is False:
                    break;
            else:
                Err('after rm_old_log still full')
                #still full ,so no need to check space any more
                cls.stop_timer()
        else:
            Err('no files in logroot {}'.format(config.LOG_ROOT))

    @classmethod
    def timeout_func(cls):
        # Info('check log timeout func')
        if check_space_full():
            cls.rm_old_log()

    @classmethod
    def start_timer(cls):
        if cls.timer is not None:
            cls.timer.start()

    @classmethod
    def stop_timer(cls):
        Info('class_log_timer stop')
        if cls.timer is not None:
            cls.timer.stop()

    # @classmethod
    # def get_L_logger(cls):
    #     return cls.L_obj.getlog()

"""

class_log_timer.init()