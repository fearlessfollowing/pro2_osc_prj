#v1.00 -- 2017.2.6
from util.log_util import *
VERSION = 'V0.9.2_2017.11.8'

class ins_version:
    @classmethod
    def get_version(self):
        Info('version is {}'.format(VERSION))
        return VERSION

    @classmethod
    def get_git_version(self):
        pass