//
// Created by vans on 17-2-6.
//

/*
 * V1.00 2017.2.6
 *
 *
 */
#include <common/include_common.h>

#define TAG ("pro_version")

const char * get_version_num()
{
    return "0.7.0";
}

static const char *get_version_date()
{
    return "2017.2.6";
}

static const char *get_git_version()
{
    return "";
}

void debug_version_info()
{
    char buf[128];

    snprintf(buf, sizeof(buf),"%s %s %s",get_version_num(),get_version_date(),get_git_version());

    Log.d(TAG,"%s",buf);
}
