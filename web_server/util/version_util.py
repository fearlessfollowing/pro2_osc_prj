# from util.log_util import *
from util.ins_log_util import *

VERSION = '-------------- V1.0.2_2018.10.15 -----------------'

class ins_version:
    @classmethod
    def get_version(self):
        Info('version is {}'.format(VERSION))
        return VERSION

    @classmethod
    def get_git_version(self):
        pass