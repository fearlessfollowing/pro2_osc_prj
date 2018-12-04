#include <common/include_common.h>
#include <sys/types.h>			/* See NOTES */
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#include <log/log_wrapper.h>

#include <util/font_ascii.h>

#undef  TAG
#define TAG "Util"


#if 0
/*
 * 拼接出一个字符串，并计算出该字符串在UI上居中显示的起始位置
 */
int calcStrCat(char* srcBuf, char* subBuf, int iErrno, int iStartPos, int iEndPos)
{
    if (srcBuf) {
        int iStartSrcPos = strlen(srcBuf);
        if (subBuf) {
            strcat(srcBuf, subBuf);
        }

        if (iErrno > 0) {
            char errno_buf[12] = {0};
            sprintf(errno_buf, "%d", iErrno);
            strcat(srcBuf, errno_buf);
        }

    } else {
        return -1;
    }
}
#endif



#if 0
int exec_sh(const char *str)
{
    int status = system(str);
    int iRet = -1;

    if (-1 == status) {
        LOGERR(TAG, "system %s error\n", str);
    } else {
    
        // printf("exit status value = [0x%x]\n", status);
        if (WIFEXITED(status)) {
            if (0 == WEXITSTATUS(status)) {
                iRet = 0;
            } else {
                LOGERR(TAG,"%s fail script exit code: %d\n", str,WEXITSTATUS(status));
            }
        } else {
            LOGERR(TAG,"exit status %s error  = [%d]\n",str, WEXITSTATUS(status));
        }
    }
    return iRet;
}
#endif

//bigedian or litteledian
bool sh_isbig(void)
{
    static union {
        unsigned short _s;
        u8 _c;
    } __u = { 1 };
    return __u._c == 0;
}

int read_line(int fd, void *vptr, int maxlen)
{
    int n, rc;
    char c;
    char *ptr;

    ptr = (char *)vptr;
    for (n = 1; n < maxlen; n++) {
        again:
        if ((rc = read(fd, &c, 1)) == 1) {
            //not add '\n' to buf
            if (c == '\n' || c == '\r')
                break;
            *ptr++ = c;

        } else if (rc == 0) {
            *ptr = 0;
            return (n - 1);
        } else {
            if (errno == EINTR) {
                printf("read line error\n");
                goto again;
            }
            return -1;
        }
    }
    *ptr = 0;
    return(n);
}

bool check_path_access(const char *path,int mode)
{
    bool bRet = false;
    if (access(path,mode) == -1) {
    } else {
        bRet = true;
    }
    return bRet;
}

bool check_path_exist(const char *path)
{
    return check_path_access(path,F_OK);
}

#if 0
int move_cmd(const char *src,const char *dest)
{
    int ret;
    char cmd[1024];

    snprintf(cmd, sizeof(cmd), "mv %s %s", src, dest);

    ret = exec_sh(cmd);
    if (ret != 0) {
        fprintf(stderr, "move cmd %s error", cmd);
    }
    return exec_sh(cmd);
}
#endif

// #define ENABLE_SPEED_TEST
#define SDCARD_TEST_SUC     "/.pro_suc"
#define SDCARD_TEST_FAIL    "/pro_test_fail"


bool check_dev_speed_good(const char *path)
{
    bool ret = false;
#ifdef ENABLE_SPEED_TEST
    char buf[1024];

    snprintf(buf, sizeof(buf), "%s%s", path, SDCARD_TEST_SUC);
    LOGDBG(TAG, "check dev speed path %s", path);
    if (check_path_exist(buf)) {
        ret = true;
    }
#else
    ret = true;
#endif
    return ret;
}


#if 0
int ins_rm_file(const char *name)
{
    char cmd[1024];
    int ret = 0;
    if (check_path_exist(name)) {
        snprintf(cmd, sizeof(cmd), "rm -rf %s", name);
        ret = exec_sh(cmd);
        if (ret != 0) {
            LOGERR(TAG, "rm file %s error\n",name);
        }
    }
    return ret;
}
#endif



/*
 * create_socket - creates a Unix domain socket in ANDROID_SOCKET_DIR
 * ("/dev/socket") as dictated in init.rc. This socket is inherited by the
 * daemon. We communicate the file descriptor's value via the environment
 * variable ANDROID_SOCKET_ENV_PREFIX<name> ("ANDROID_SOCKET_foo").
 * SOCK_STREAM/SOCK_DGRAM
 */
int create_socket(const char *name, int type, mode_t perm)
{
    struct sockaddr_un addr;
    int fd, ret;


    fd = socket(PF_UNIX, type, 0);
    if (fd < 0) {
        LOGERR(TAG, "Failed to open socket '%s': %s\n", name, strerror(errno));
        return -1;
    }

    memset(&addr, 0 , sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "/dev/socket/%s", name);

    ret = unlink(addr.sun_path);
    if (ret != 0 && errno != ENOENT) {
        LOGERR(TAG, "Failed to unlink old socket '%s': %s", name, strerror(errno));
        goto out_close;
    }

    ret = bind(fd, (struct sockaddr *) &addr, sizeof (addr));
    if (ret) {
        LOGERR(TAG, "Failed to bind socket '%s': %s", name, strerror(errno));
        goto out_unlink;
    }

    chmod(addr.sun_path, perm);

    LOGDBG(TAG, "Created socket '%s' with mode '%o', user '%d', group '%d'", addr.sun_path, perm);
    return fd;

out_unlink:
    unlink(addr.sun_path);
out_close:
    close(fd);
    return -1;
}




