from util.log_util import *
VERSION = 'V1.0.1_2018.09.29'

class ins_version:
    @classmethod
    def get_version(self):
        Info('version is {}'.format(VERSION))
        return VERSION

    @classmethod
    def get_git_version(self):
        pass