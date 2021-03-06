#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <minigui/common.h>
#include <minigui/minigui.h>

#include <mdolphin/mdolphin.h>
#include <mdolphin/mdolphin_binding.h>
#include "mdolphin_native.h"

#if ENABLE_JSNATIVEBINDING

struct MediaPlayer{
    int   x;
    int   y;
    int   w;
    int   h;
    
    int open(const char *url) {
        TRACE("[%s,%d]url=%s\n", __FUNCTION__, __LINE__, url);
        return 0;
    }

    int play() {
        TRACE("[%s,%d]\n", __FUNCTION__, __LINE__);
        return 0;
    }

    void stop() {TRACE("[%s,%d]\n", __FUNCTION__, __LINE__);}
    void close() {TRACE("[%s,%d]\n", __FUNCTION__, __LINE__);}

    static const char* version() {
        TRACE("[%s,%d]\n", __FUNCTION__, __LINE__);
        return "mdolphin v2.0.0 - Native Binding";
    };
};

static JSValueRef myPrint(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception);

static JSValueRef runLocal(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception);

static JSObjectRef player_constructor(JSContextRef context, JSObjectRef constructor, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception);
static void player_destructor(JSObjectRef object);
static JSValueRef player_open(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception);
static JSValueRef player_play(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception);
static JSValueRef player_stop(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception);
static JSValueRef player_close(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception);
static JSValueRef player_getVersion(JSContextRef context, JSObjectRef object, JSStringRef propertyName, JSValueRef* exception);
static JSValueRef player_getX(JSContextRef context, JSObjectRef object, JSStringRef propertyName, JSValueRef* exception);
static bool player_setX(JSContextRef context, JSObjectRef object, JSStringRef propertyName, JSValueRef value, JSValueRef* exception);

static JSNativeFunction playerMethod[] = {
    {"open", player_open, kJSPropertyAttributeDontDelete}, 
    {"play", player_play, kJSPropertyAttributeDontDelete}, 
    {"stop", player_stop, kJSPropertyAttributeDontDelete},
    {"close", player_close, kJSPropertyAttributeDontDelete},
    {0, 0, 0}
};

static JSProperty playerProperty[] = {
    {"version", player_getVersion, NULL, kJSPropertyAttributeDontDelete | kJSPropertyAttributeReadOnly},
    {"x", player_getX, player_setX, kJSPropertyAttributeDontDelete},
    {0, 0, 0, 0}
};

static JSNativeClass Player = {
    "MediaPlayer",
    0,
    player_constructor,
    player_destructor,
    playerMethod,
    playerProperty,
};

static JSNativeFunction Print = {
    "print", myPrint, 0
};

static JSNativeFunction RunLocal= {
    "md_runLocal", runLocal, 0
};
static bool fileExisted()
{
    const char* file = "tmpjs";
    struct stat stats;
    if (stat(file, &stats) == 0) {
        if (stats.st_mode & S_IFREG) {
            return true;
        }
    }
    return false;
}
static JSValueRef runLocal(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    //test file existed
    if (!fileExisted()) {
        printf("file not existed\n");
        return JSValueMakeUndefined(context);
    }
    if (argumentCount >= 1) {
        char buffer[1024];
        int fd[2];
        if (pipe(fd) < 0)
            return JSValueMakeUndefined(context);

        JSStringRef str = JSValueToStringCopy(context, arguments[0], exception);
        size_t len = JSStringGetLength(str) + 1;
        char *buf = new char[len];
        memset(buf, 0x0, len);
        JSStringGetUTF8CString(str, buf, len-1); 
        JSStringRelease(str);

        JSStringRef str1 = JSValueToStringCopy(context, arguments[1], exception);
        size_t len1 = JSStringGetLength(str1) + 1;
        char *buf1 = new char[len1];
        memset(buf1, 0x0, len1);
        JSStringGetUTF8CString(str1, buf1, len1-1); 
        JSStringRelease(str1);

        printf("[%s,%d]str=%s, %s\n", __FUNCTION__, __LINE__, buf, buf1);
        pid_t pid;
       

        if ( (pid = fork()) < 0)
            printf("fork error\n");
        else if (pid == 0) {
            close(fd[0]);
            close(1);
            dup2(fd[1], 1);
            close(fd[1]);
            if (execl(buf, "md_runLocal", buf1, (char *) 0) < 0)
                printf("execl error\n");
            exit(0);
        }   

        if (waitpid(pid, NULL, 0) < 0)    /* parent */
            printf("waitpid error\n");
        close(fd[1]);
        read(fd[0], buffer, sizeof(buffer));
        printf("cmd result: [%s]\n", buffer);
        delete[] buf;
        delete[] buf1;
        return JSValueMakeString(context, JSStringCreateWithUTF8CString(buffer)); 
    }
    return JSValueMakeUndefined(context);
}

static JSValueRef myPrint(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount >= 1) {
        JSStringRef str = JSValueToStringCopy(context, arguments[0], exception);
        size_t len = JSStringGetLength(str) + 1;
        char *buf = new char[len];
        memset(buf, 0x0, len);
        JSStringGetUTF8CString(str, buf, len-1); 
        JSStringRelease(str);
        TRACE("[%s,%d]str=%s\n", __FUNCTION__, __LINE__, buf);
        delete[] buf;
    }
    return JSValueMakeUndefined(context);
}

static JSObjectRef player_constructor(JSContextRef context, JSObjectRef constructor, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    MediaPlayer *mplayer = new MediaPlayer;
    JSObjectRef newObj = JSObjectMake(context,
            mdolphin_lookup_jsnativeclass(Player.name),
            mplayer);
    return newObj;
}

static void player_destructor(JSObjectRef object)
{
    MediaPlayer *mplayer = (MediaPlayer*)JSObjectGetPrivate(object);
    delete mplayer;
    JSObjectSetPrivate(object, 0);
}

static JSValueRef player_open(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount >= 1) {
        MediaPlayer *mplayer = (MediaPlayer*)JSObjectGetPrivate(thisObject);
        JSStringRef strUrl = JSValueToStringCopy(context, arguments[0], exception);
        char buf[20];
        memset(buf, 0x0, sizeof(buf));
        JSStringGetUTF8CString(strUrl, buf, 19); 
        mplayer->open(buf);
        JSStringRelease(strUrl);
    }
    return JSValueMakeUndefined(context);
}

static JSValueRef player_play(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    MediaPlayer *mplayer = (MediaPlayer*)JSObjectGetPrivate(thisObject);
    mplayer->play();
    return JSValueMakeUndefined(context);
}

static JSValueRef player_stop(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    MediaPlayer *mplayer = (MediaPlayer*)JSObjectGetPrivate(thisObject);
    mplayer->stop();
    return JSValueMakeUndefined(context);
}

static JSValueRef player_close(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    MediaPlayer *mplayer = (MediaPlayer*)JSObjectGetPrivate(thisObject);
    mplayer->close();
    return JSValueMakeUndefined(context);
}

static JSValueRef player_getVersion(JSContextRef context, JSObjectRef object, JSStringRef propertyName, JSValueRef* exception)
{
    MediaPlayer *mplayer = (MediaPlayer*)JSObjectGetPrivate(object);
    JSStringRef strRef = JSStringCreateWithUTF8CString(mplayer->version());
    JSValueRef valRef = JSValueMakeString(context, strRef); 
    JSStringRelease(strRef);
    return valRef;
}

static JSValueRef player_getX(JSContextRef context, JSObjectRef object, JSStringRef propertyName, JSValueRef* exception)
{
    MediaPlayer *mplayer = (MediaPlayer*)JSObjectGetPrivate(object);
    return JSValueMakeNumber(context, mplayer->x);
}

static bool player_setX(JSContextRef context, JSObjectRef object, JSStringRef propertyName, JSValueRef value, JSValueRef* exception)
{
    MediaPlayer *mplayer = (MediaPlayer*)JSObjectGetPrivate(object);
    mplayer->x = (int)JSValueToNumber(context, value, exception); 
    TRACE("[%s,%d]x=%d\n", __FUNCTION__, __LINE__, mplayer->x);
    return true;
}

void define_nativeobjects(void)
{
    mdolphin_define_jsnativeclass(&Player);
    mdolphin_define_jsnativefunction(&Print);
    mdolphin_define_jsnativefunction(&RunLocal);
}

#endif /* ENABLE_JSNATIVEBINDING */
