# -*- coding: UTF-8 -*-
# 文件名：  config.py 
# 版本：    V1.0.1
# 修改记录：
# 日期              修改人                  版本            备注
# 2018年10月16日    Skymixos                V1.0.3          增加注释
#

# 模板参数的存放路径
TEMPLATE_ARGS_PATH = '/usr/local/bin/Pro2_OSC/pro2_osc/template'

DEFAULT_OPTIONS         = TEMPLATE_ARGS_PATH + '/defaultOptions.json'
CURRENT_OPTIONS         = TEMPLATE_ARGS_PATH + '/currentOptions.json'
SET_OPTION_TMPLATE      = TEMPLATE_ARGS_PATH + '/_setOptionsTemplate.json'

ENTRIES_TIMPLATE        = TEMPLATE_ARGS_PATH + '/entriesTemplate.json'

FILE_URLS_TEMPLATE      = TEMPLATE_ARGS_PATH + '/fileUrls.json'

TAKEPIC_TEMPLATE        = TEMPLATE_ARGS_PATH + '/takePictureTemplate.json' 
PIC_FORMAT_MAP_TMPLATE  = TEMPLATE_ARGS_PATH + '/pictureFormatMap.json'

PREVIEW_TEMPLATE        = TEMPLATE_ARGS_PATH + '/previewTemplate.json'

START_CAPTURE_TEMPLATE  = TEMPLATE_ARGS_PATH + '/startCaptureTemplate.json'

VIDEO_FORMAT_TEMPLATE   = TEMPLATE_ARGS_PATH + '/videoFormatMap.json'

OSC_INFO_TEMPLATE       = TEMPLATE_ARGS_PATH + '/osc_info.json'

EXPOSURE_MAP_TEMPLATE   = TEMPLATE_ARGS_PATH + '/exposureMapping.json'

UP_TIME_PATH            = '/proc/uptime'
