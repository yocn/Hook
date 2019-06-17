#include <jni.h>
#include <string.h>
#include <vector>
#include <stdio.h>
#include <android/log.h>
#include <android/bitmap.h>
#include <string>
#include "reader.h"
#include "document.h"
#include "writer.h"
#include "stringbuffer.h"
#include <unistd.h>
#include <sys/stat.h>

#define LOG_TAG "libbitmaputils"
#define LOGV(...)  __android_log_print(ANDROID_LOG_VERBOSE,LOG_TAG,__VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#define ANIMATION "animation/"
#define MAKESHAPE "makeshape/"

using namespace rapidjson;
using namespace std;

extern "C" {

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

vector<string> filenames;
vector<string> masknames;

string filePath;

/**
 * File_ISExist--判断文件是否存在
 * 0存在  -1不存在
 */
int File_ISExist(const char *path) {
    if (path == NULL) {
        return 100;
    }
    return access(path, 0);
}


/**
 * 解析Animation.zip
 */
void parseJson(JNIEnv *env, jobject thiz, const char *json) {
    LOGE(" parseJson begin~~~~~~~~~~~~~~~~~~~~");
    filenames.clear();
    Document document;
    document.Parse(json);
    assert(document.IsArray());
    LOGE(" parseJson begin~~~~~~~~~~~~~~~~~~~document.IsArray()~");
    for (size_t m = 0; m < document.Size(); m++) {
        Value &root = document[m];
        Value &layersArray = root["layers"];
        if (layersArray.IsArray()) {
            for (size_t i = 0; i < layersArray.Size(); i++) {
                Value &layer = layersArray[i];
                if (layer.HasMember("opacity")) {
                    Value &opacity = layer["opacity"];
                    if (opacity.HasMember("filename")) {
                        string filename = opacity["filename"].GetString();
                        filenames.push_back(filename);
                        LOGE(" opacity filename-> %s", filename.c_str());
                    }
                }

                if (layer.HasMember("transform")) {
                    //含有transform key
                    Value &transform = layer["transform"];
                    if (transform.HasMember("filename")) {
                        string filename = transform["filename"].GetString();
                        filenames.push_back(filename);
                        LOGE("transform filename-> %s", filename.c_str());
                    }
                }

                if (layer.HasMember("effects")) {
                    Value &effects = layer["effects"];
                    if (effects.IsArray()) {
                        for (size_t i = 0; i < effects.Size(); i++) {
                            Value &effect = effects[i];
                            if (effect.HasMember("filename")) {
                                string filename = effect["filename"].GetString();
                                filenames.push_back(filename);
                                LOGE("effects filename-> %s", filename.c_str());
                            }
                        }
                    }
                }

            }
        }
    }
    long size = filenames.size();
    LOGE("Animation -> size %ld", size);
    LOGE("document.Size() -> size %d", document.Size());
}

void parseMask(JNIEnv *env, jobject thiz, const char *mask) {
    masknames.clear();
    Document document;
    document.Parse(mask);
    Value &masks = document["masks"];
    if (masks.Empty()) {
        return;
    }
    assert(masks.IsArray());
    for (size_t i = 0; i < masks.Size(); i++) {
        Value &mask = masks[i];
        if (mask.HasMember("filename")) {
            string filename = mask["filename"].GetString();
            masknames.push_back(filename);
//            LOGE("filename-> %s", filename.c_str());
        }
    }
    long size = masknames.size();
    LOGE("Mask -> size %ld", size);
}

string getFilePath(JNIEnv *env, jstring jfilepath) {
    const char *v1Str = env->GetStringUTFChars(jfilepath, NULL);
    //LOGV(" libbitmaputils source = %s", v1Str);

    string path;
    string filename;
    int pathPosition = 0;
    string str_pathfilename(v1Str);
    pathPosition = str_pathfilename.find_last_of('/');
    pathPosition++;   //add 1,

    path.assign(str_pathfilename.c_str(), pathPosition);
    env->ReleaseStringUTFChars(jfilepath, v1Str);
    return path;
    //filename.assign(&str_pathfilename[pathPosition],str_pathfilename.length()-pathPosition);
}

string getFileName(string filepath) {
    string path;
    string filename;
    int pathPosition = 0;
    string str_pathfilename(filepath);
    pathPosition = str_pathfilename.find_last_of('/');
    pathPosition++;   //add 1,

    path.assign(str_pathfilename.c_str(), pathPosition);
    filename.assign(&str_pathfilename[pathPosition], str_pathfilename.length() - pathPosition);
    return filename;
}

string getFilePrefix(string filepath) {
    string path;
    string filename;
    int pathPosition = 0;
    string str_pathfilename(filepath);
    pathPosition = str_pathfilename.find_last_of('/');
    pathPosition++;   //add 1,
    path.assign(str_pathfilename.c_str(), pathPosition);
    return path;
}

void printfJstring(JNIEnv *env, jobject thiz,
                   jstring v1, int id) {
    int len = env->GetStringLength(v1);
    const char *v1Str = env->GetStringUTFChars(v1, NULL);
    LOGE(" libbitmaputils source = id->%d , length->%d : %s", id, len, v1Str);
    env->ReleaseStringUTFChars(v1, v1Str);
}

void writeJstringToFile(JNIEnv *env, jobject thiz,
                        jstring srcfilename, jstring strContent) {
//    LOGE("writeJstringToFile");
    const char *filenameStr = env->GetStringUTFChars(srcfilename, NULL);
    const char *strCon = env->GetStringUTFChars(strContent, NULL);

    LOGE("writeFileBegin->%s", filenameStr);
//    LOGE("writeFileBegin->%s:%s", filenameStr, strCon);

    if (filePath.empty()) {
        string path(filenameStr);
        filePath = getFilePrefix(path);
        LOGE("---1-----------------------------filePath->%s", filePath.c_str());
    }

    //LOGV(" libbitmaputils source = %s", v1Str);
    string filePath(filenameStr);
    filePath += ".dec.json";

    FILE *pFile;
    if ((pFile = fopen(filePath.c_str(), "wb")) == NULL) {
        LOGE("cant open the file,%s", filePath.c_str());
    }
    fwrite(strCon, sizeof(char), strlen(strCon), pFile);
    fclose(pFile);
//    LOGE("writeFileCompplete");
    env->ReleaseStringUTFChars(srcfilename, filenameStr);
    env->ReleaseStringUTFChars(strContent, strCon);
}

void writeNewJstringToFile(JNIEnv *env, jobject thiz,
                           string srcfilename, jstring strContent, int type) {
    LOGE("writeNewJstringToFile->%d", type);
//    if (type == 1) {
//        srcfilename += ".animation.json";
//    } else {
//        srcfilename += ".mask.json";
//    }
    const char *strCon = env->GetStringUTFChars(strContent, NULL);

//    LOGE("writeNewFileBegin->id:%d  %s:%s <-over", type, srcfilename.c_str(), strCon);
    LOGE("writeNewFileBegin->id:%d  %s <-over", type, srcfilename.c_str());

    FILE *pFile;
    if ((pFile = fopen(srcfilename.c_str(), "wb")) == NULL) {
        LOGE("cant open the file,%s", srcfilename.c_str());
    }
    fwrite(strCon, sizeof(char), strlen(strCon), pFile);
    fclose(pFile);
    LOGE("writeNewFileCompplete   -----   ");
//    env->ReleaseStringUTFChars(srcfilename, filenameStr);
    env->ReleaseStringUTFChars(strContent, strCon);
}

void writeJstringToFileNeedParseJson(JNIEnv *env, jobject thiz,
                                     jstring srcfilename, jstring strContent, int type) {

    LOGE("writeJstringToFileNeedParseJson->%d", type);
    const char *filenameStr = env->GetStringUTFChars(srcfilename, NULL);
    const char *strCon = env->GetStringUTFChars(strContent, NULL);

    if (type == 1) {
        /**1->Animation.zip*/
        //解析json并存到vertor里面
        parseJson(env, thiz, strCon);
        LOGE("解析Animation!!!!!!!!!!!!!!!!!!!!!!!");
    } else {
        /**2->maskshape.zip*/
        //解析json并存到vertor里面
        parseMask(env, thiz, strCon);
        LOGE("解析MaskShape!!!!!!!!!!!!!!!!!!!!!!!");
    }

    //LOGV(" libbitmaputils source = %s", v1Str);
    string filePath(filenameStr);
    filePath += ".dec.json";

    FILE *pFile;
    if ((pFile = fopen(filePath.c_str(), "wb")) == NULL) {
        LOGV("cant open the file,%s", filePath.c_str());
    }
    fwrite(strCon, sizeof(char), strlen(strCon), pFile);
    fclose(pFile);

    env->ReleaseStringUTFChars(srcfilename, filenameStr);
    env->ReleaseStringUTFChars(strContent, strCon);
}

//动态链接库路径
#define LIB_CACULATE_PATH "/data/app/com.qutui360.app-1/lib/libsrcvenus.so"


//函数指针
typedef void (*CAC_FUNC_PREPARE)(JNIEnv *env, jobject thiz);
//函数指针
typedef void (*CAC_FUNC_NATIVE_INIT)(JNIEnv *env, jobject thiz, jobject obj);
//函数指针
typedef void (*CAC_FUNC_VOID)(JNIEnv *env, jobject thiz, jstring v1, jstring v2);
//函数指针
typedef jobject (*CAC_FUNC_HEADER)(JNIEnv *env, jobject thiz);
//函数指针
typedef void (*CAC_FUNC_SET_SURFACE)(JNIEnv *env, jobject thiz, jobject obj);
//函数指针
typedef jstring (*CAC_FUNC)(JNIEnv *env, jobject thiz, jstring v1, jstring v2);
//函数指针
typedef void (*CAC_FUNC_ADD_IMAGE)(JNIEnv *env, jobject thiz, jstring v1, jobject obj);
//函数指针
typedef void (*CAC_FUNC_ADD_WEBP)(JNIEnv *env, jobject thiz, jstring s1, jstring s2, jint i1,
                                  jint i2);
//函数指针
typedef jboolean (*CAC_FUNC_CREATE_JUST_NOW)(JNIEnv *env, jobject thiz, jstring v1);
//函数指针
typedef void (*CAC_FUNC_START)(JNIEnv *env, jobject thiz);
//函数指针
typedef void (*CAC_FUNC_DECODE_WEBP_ANIM)(JNIEnv *env, jobject thiz, jint i1);
//函数指针
typedef void (*CAC_FUNC_READ_FREAME)(JNIEnv *env, jobject thiz, jlong l1);
//函数指针
typedef void (*CAC_FUNC_DESTROY)(JNIEnv *env, jobject thiz);
//函数指针
typedef void (*CAC_FUNC_SOURCE)(JNIEnv *env, jobject thiz, jstring s);
//函数指针
typedef void (*CAC_FUNC_CREATE_VIDEO_TEXTURE)(JNIEnv *env, jobject thiz, jstring s, jint i1,
                                              jint i2, jint i3);
//函数指针
typedef void (*CAC_FUNC_UPDATE_VIDEO)(JNIEnv *env, jobject thiz, jstring s);
//函数指针
typedef jlong (*CAC_FUNC_MASK_SHAPE)(JNIEnv *env, jobject thiz);
//函数指针
typedef jstring (*CAC_FUNC_READ_MASK_SHAPE)(JNIEnv *env, jobject thiz, jlong l, jstring s);
//函数指针
typedef void (*CAC_FUNC_CLOSE_MASK_SHAPE)(JNIEnv *env, jobject thiz, jlong l);
//函数指针
typedef void (*CAC_FUNC_UPDATE_MASK)(JNIEnv *env, jobject thiz, jstring s, jint i, jobject obj);
//函数指针
typedef void (*CAC_FUNC_GET_FIRST_FRAME)(JNIEnv *env, jobject thiz, jstring s);

jstring callFunc(JNIEnv *env, jobject thiz,
                 jstring v1, jstring v2) {
    void *handle;
    char *error;
    CAC_FUNC cac_func = NULL;

    //打开动态链接库
    handle = dlopen(LIB_CACULATE_PATH, RTLD_LAZY);
    if (!handle) {
        LOGV("dlopen: %s\n", dlerror());
    }

    //清除之前存在的错误
    dlerror();

    //获取一个函数
    *(void **) (&cac_func) = dlsym(handle, "Java_doupai_venus_venus_Venus_decrypt");

    if ((error = const_cast<char *>(dlerror())) != NULL) {
        LOGV("dlsym: %s\n", error);
    }
    jstring ret = (*cac_func)(env, thiz, v1, v2);
//    printfJstring(env, thiz, ret);
    //关闭动态链接库
//    dlclose(handle);
    return ret;
}


void callFuncNativeInit(JNIEnv *env, jobject thiz, jobject obj) {
    void *handle;
    char *error;
    CAC_FUNC_NATIVE_INIT cac_func = NULL;

    //打开动态链接库
    handle = dlopen(LIB_CACULATE_PATH, RTLD_LAZY);
    if (!handle) {
        LOGV("dlopen: %s\n", dlerror());
    }

    //清除之前存在的错误
    dlerror();

    //获取一个函数
    *(void **) (&cac_func) = dlsym(handle, "Java_doupai_venus_venus_NativeObject_native_1init");
    if ((error = const_cast<char *>(dlerror())) != NULL) {
        LOGV("dlsym: %s\n", error);
    }
    (*cac_func)(env, thiz, obj);
    //关闭动态链接库
//    dlclose(handle);
}

void callFuncCreate(JNIEnv *env, jobject thiz, jstring v1, jstring v2) {
    void *handle;
    char *error;
    CAC_FUNC_VOID cac_func = NULL;

    //打开动态链接库
    handle = dlopen(LIB_CACULATE_PATH, RTLD_NOW);
    if (!handle) {
        LOGV("dlopen: %s\n", dlerror());
    }

    //清除之前存在的错误
    dlerror();

    //获取一个函数
    *(void **) (&cac_func) = dlsym(handle, "Java_doupai_venus_venus_TemplateEngine_native_1create");
    if ((error = const_cast<char *>(dlerror())) != NULL) {
        LOGV("dlsym: %s\n", error);
    }
    (*cac_func)(env, thiz, v1, v2);
    //关闭动态链接库
//    dlclose(handle);
}

jobject callFuncHeader(JNIEnv *env, jobject thiz) {
    void *handle;
    char *error;
    CAC_FUNC_HEADER cac_func = NULL;

    //打开动态链接库
    handle = dlopen(LIB_CACULATE_PATH, RTLD_NOW);
    if (!handle) {
        LOGV("dlopen: %s\n", dlerror());
    }

    //清除之前存在的错误
    dlerror();

    //获取一个函数
    *(void **) (&cac_func) = dlsym(handle,
                                   "Java_doupai_venus_venus_TemplateEngine_getTemplateHeader");
    if ((error = const_cast<char *>(dlerror())) != NULL) {
        LOGV("dlsym: %s\n", error);
    }
    jobject obj = (*cac_func)(env, thiz);
    //关闭动态链接库
//    dlclose(handle);
    return obj;
}

jobject callFuncVersion(JNIEnv *env, jobject thiz) {
    void *handle;
    char *error;
    CAC_FUNC_HEADER cac_func = NULL;

    //打开动态链接库
    handle = dlopen(LIB_CACULATE_PATH, RTLD_NOW);
    if (!handle) {
        LOGV("dlopen: %s\n", dlerror());
    }

    //清除之前存在的错误
    dlerror();

    //获取一个函数
    *(void **) (&cac_func) = dlsym(handle,
                                   "Java_doupai_venus_venus_TemplateEngine_getTemplateVersion");
    if ((error = const_cast<char *>(dlerror())) != NULL) {
        LOGV("dlsym: %s\n", error);
    }
    jobject obj = (*cac_func)(env, thiz);
    //关闭动态链接库
//    dlclose(handle);
    return obj;
}


void callFuncSetSurface(JNIEnv *env, jobject thiz, jobject obj) {
    void *handle;
    char *error;
    CAC_FUNC_SET_SURFACE cac_func = NULL;

    //打开动态链接库
    handle = dlopen(LIB_CACULATE_PATH, RTLD_NOW);
    if (!handle) {
        LOGV("dlopen: %s\n", dlerror());
    }

    //清除之前存在的错误
    dlerror();

    //获取一个函数
    *(void **) (&cac_func) = dlsym(handle,
                                   "Java_doupai_venus_venus_TemplateEngine_native_1setSurface");
    if ((error = const_cast<char *>(dlerror())) != NULL) {
        LOGV("dlsym: %s\n", error);
    }
    (*cac_func)(env, thiz, obj);
    //关闭动态链接库
//    dlclose(handle);
}

jobject callFuncNativePrepare(JNIEnv *env, jobject thiz) {
    void *handle;
    char *error;
    CAC_FUNC_PREPARE cac_func = NULL;

    //打开动态链接库
    handle = dlopen(LIB_CACULATE_PATH, RTLD_NOW);
    if (!handle) {
        LOGV("dlopen: %s\n", dlerror());
    }

    //清除之前存在的错误
    dlerror();

    //获取一个函数
    *(void **) (&cac_func) = dlsym(handle,
                                   "Java_doupai_venus_venus_TemplateEngine_native_1prepare");
    if ((error = const_cast<char *>(dlerror())) != NULL) {
        LOGV("dlsym: %s\n", error);
    }
    (*cac_func)(env, thiz);
    //关闭动态链接库
//    dlclose(handle);
}

void callFuncAddImage(JNIEnv *env, jobject thiz, jstring jstring1, jobject obj) {
    void *handle;
    char *error;
    CAC_FUNC_ADD_IMAGE cac_func = NULL;

    //打开动态链接库
    handle = dlopen(LIB_CACULATE_PATH, RTLD_NOW);
    if (!handle) {
        LOGV("dlopen: %s\n", dlerror());
    }

    //清除之前存在的错误
    dlerror();

    //获取一个函数
    *(void **) (&cac_func) = dlsym(handle,
                                   "Java_doupai_venus_venus_TemplateEngine_addImage");
    if ((error = const_cast<char *>(dlerror())) != NULL) {
        LOGV("dlsym: %s\n", error);
    }
    (*cac_func)(env, thiz, jstring1, obj);
    //关闭动态链接库
//    dlclose(handle);
}

void callFuncAddWebp(JNIEnv *env, jobject thiz, jstring s1, jstring s2,
                     jint i1, jint i2) {
    void *handle;
    char *error;
    CAC_FUNC_ADD_WEBP cac_func = NULL;

    //打开动态链接库
    handle = dlopen(LIB_CACULATE_PATH, RTLD_NOW);
    if (!handle) {
        LOGV("dlopen: %s\n", dlerror());
    }

    //清除之前存在的错误
    dlerror();

    //获取一个函数
    *(void **) (&cac_func) = dlsym(handle,
                                   "Java_doupai_venus_venus_TemplateEngine_addWebp");
    if ((error = const_cast<char *>(dlerror())) != NULL) {
        LOGV("dlsym: %s\n", error);
    }
    (*cac_func)(env, thiz, s1, s2, i1, i2);
    //关闭动态链接库
//    dlclose(handle);
}

jboolean callFuncCreateJustNow(JNIEnv *env, jobject thiz, jstring s1) {
    void *handle;
    char *error;
    CAC_FUNC_CREATE_JUST_NOW cac_func = NULL;

    //打开动态链接库
    handle = dlopen(LIB_CACULATE_PATH, RTLD_NOW);
    if (!handle) {
        LOGV("dlopen: %s\n", dlerror());
    }

    //清除之前存在的错误
    dlerror();

    //获取一个函数
    *(void **) (&cac_func) = dlsym(handle,
                                   "Java_doupai_venus_venus_TemplateEngine_createJustNow");
    if ((error = const_cast<char *>(dlerror())) != NULL) {
        LOGV("dlsym: %s\n", error);
    }
    jboolean jb = (*cac_func)(env, thiz, s1);
    //关闭动态链接库
//    dlclose(handle);
    return jb;
}

void callFuncStart(JNIEnv *env, jobject thiz) {
    void *handle;
    char *error;
    CAC_FUNC_START cac_func = NULL;

    //打开动态链接库
    handle = dlopen(LIB_CACULATE_PATH, RTLD_NOW);
    if (!handle) {
        LOGE("dlopen: %s\n", dlerror());
    }

    //清除之前存在的错误
    dlerror();

    //获取一个函数
    *(void **) (&cac_func) = dlsym(handle,
                                   "Java_doupai_venus_venus_TemplateEngine_native_1start");
    if ((error = const_cast<char *>(dlerror())) != NULL) {
        LOGE("dlsym: %s\n", error);
    }
    (*cac_func)(env, thiz);
    //关闭动态链接库
//    dlclose(handle);
}

void callFuncDecodeWebpAnim(JNIEnv *env, jobject thiz, jint i1) {
    void *handle;
    char *error;
    CAC_FUNC_DECODE_WEBP_ANIM cac_func = NULL;

    //打开动态链接库
    handle = dlopen(LIB_CACULATE_PATH, RTLD_NOW);
    if (!handle) {
        LOGE("dlopen: %s\n", dlerror());
    }

    //清除之前存在的错误
    dlerror();

    //获取一个函数
    *(void **) (&cac_func) = dlsym(handle,
                                   "Java_doupai_venus_venus_TemplateEngine_decodeWebpAnimations");
    if ((error = const_cast<char *>(dlerror())) != NULL) {
        LOGE("dlsym: %s\n", error);
    }
    (*cac_func)(env, thiz, i1);
    //关闭动态链接库
//    dlclose(handle);
}

void callFuncReadFrame(JNIEnv *env, jobject thiz, jlong i1) {
    void *handle;
    char *error;
    CAC_FUNC_READ_FREAME cac_func = NULL;

    //打开动态链接库
    handle = dlopen(LIB_CACULATE_PATH, RTLD_NOW);
    if (!handle) {
        LOGE("dlopen: %s\n", dlerror());
    }

    //清除之前存在的错误
    dlerror();

    //获取一个函数
    *(void **) (&cac_func) = dlsym(handle,
                                   "Java_doupai_venus_venus_TemplateEngine_renderFrame");
    if ((error = const_cast<char *>(dlerror())) != NULL) {
        LOGE("dlsym: %s\n", error);
    }
    (*cac_func)(env, thiz, i1);
    //关闭动态链接库
//    dlclose(handle);
}

void callFuncDestroy(JNIEnv *env, jobject thiz) {
    void *handle;
    char *error;
    CAC_FUNC_DESTROY cac_func = NULL;

    //打开动态链接库
    handle = dlopen(LIB_CACULATE_PATH, RTLD_NOW);
    if (!handle) {
        LOGE("dlopen: %s\n", dlerror());
    }

    //清除之前存在的错误
    dlerror();

    //获取一个函数
    *(void **) (&cac_func) = dlsym(handle,
                                   "Java_doupai_venus_venus_TemplateEngine_native_1destroy");
    if ((error = const_cast<char *>(dlerror())) != NULL) {
        LOGE("dlsym: %s\n", error);
    }
    (*cac_func)(env, thiz);
    //关闭动态链接库
//    dlclose(handle);
}

void callFuncSourceVideo(JNIEnv *env, jobject thiz, jstring s) {
    void *handle;
    char *error;
    CAC_FUNC_SOURCE cac_func = NULL;

    //打开动态链接库
    handle = dlopen(LIB_CACULATE_PATH, RTLD_NOW);
    if (!handle) {
        LOGE("dlopen: %s\n", dlerror());
    }

    //清除之前存在的错误
    dlerror();

    //获取一个函数
    *(void **) (&cac_func) = dlsym(handle,
                                   "Java_doupai_venus_venus_TemplateEngine_sourceWithVideo");
    if ((error = const_cast<char *>(dlerror())) != NULL) {
        LOGE("dlsym: %s\n", error);
    }
    (*cac_func)(env, thiz, s);
    //关闭动态链接库
//    dlclose(handle);
}

void callFuncCreateVT(JNIEnv *env, jobject thiz, jstring s, jint i1, jint i2, jint i3) {
    void *handle;
    char *error;
    CAC_FUNC_CREATE_VIDEO_TEXTURE cac_func = NULL;

    //打开动态链接库
    handle = dlopen(LIB_CACULATE_PATH, RTLD_NOW);
    if (!handle) {
        LOGE("dlopen: %s\n", dlerror());
    }

    //清除之前存在的错误
    dlerror();

    //获取一个函数
    *(void **) (&cac_func) = dlsym(handle,
                                   "Java_doupai_venus_venus_TemplateEngine_createVideoTexture");
    if ((error = const_cast<char *>(dlerror())) != NULL) {
        LOGE("dlsym: %s\n", error);
    }
    (*cac_func)(env, thiz, s, i1, i2, i3);
    //关闭动态链接库
//    dlclose(handle);
}

void callFuncUpdateVideo(JNIEnv *env, jobject thiz, jstring s) {
    void *handle;
    char *error;
    CAC_FUNC_UPDATE_VIDEO cac_func = NULL;

    //打开动态链接库
    handle = dlopen(LIB_CACULATE_PATH, RTLD_NOW);
    if (!handle) {
        LOGE("dlopen: %s\n", dlerror());
    }

    //清除之前存在的错误
    dlerror();

    //获取一个函数
    *(void **) (&cac_func) = dlsym(handle,
                                   "Java_doupai_venus_venus_TemplateEngine_updateVideo");
    if ((error = const_cast<char *>(dlerror())) != NULL) {
        LOGE("dlsym: %s\n", error);
    }
    (*cac_func)(env, thiz, s);
    //关闭动态链接库
//    dlclose(handle);
}

jlong callFuncMask(JNIEnv *env, jobject thiz) {
    void *handle;
    char *error;
    CAC_FUNC_MASK_SHAPE cac_func = NULL;

    //打开动态链接库
    handle = dlopen(LIB_CACULATE_PATH, RTLD_NOW);
    if (!handle) {
        LOGE("dlopen: %s\n", dlerror());
    }

    //清除之前存在的错误
    dlerror();

    //获取一个函数
    *(void **) (&cac_func) = dlsym(handle,
                                   "Java_doupai_venus_venus_TemplateEngine_openMaskshape");
    if ((error = const_cast<char *>(dlerror())) != NULL) {
        LOGE("dlsym: %s\n", error);
    }
    (*cac_func)(env, thiz);
    //关闭动态链接库
//    dlclose(handle);
}


jstring callFuncReadMask(JNIEnv *env, jobject thiz, jlong l1, jstring s) {
    void *handle;
    char *error;
    CAC_FUNC_READ_MASK_SHAPE cac_func = NULL;

    //打开动态链接库
    handle = dlopen(LIB_CACULATE_PATH, RTLD_NOW);
    if (!handle) {
        LOGE("dlopen: %s\n", dlerror());
    }

    //清除之前存在的错误
    dlerror();

    //获取一个函数
    *(void **) (&cac_func) = dlsym(handle,
                                   "Java_doupai_venus_venus_TemplateEngine_readMaskshape");
    if ((error = const_cast<char *>(dlerror())) != NULL) {
        LOGE("dlsym: %s\n", error);
    }
    (*cac_func)(env, thiz, l1, s);
    //关闭动态链接库
//    dlclose(handle);
}


void callFuncCloseMask(JNIEnv *env, jobject thiz, jlong l1) {
    void *handle;
    char *error;
    CAC_FUNC_CLOSE_MASK_SHAPE cac_func = NULL;

    //打开动态链接库
    handle = dlopen(LIB_CACULATE_PATH, RTLD_NOW);
    if (!handle) {
        LOGE("dlopen: %s\n", dlerror());
    }

    //清除之前存在的错误
    dlerror();

    //获取一个函数
    *(void **) (&cac_func) = dlsym(handle,
                                   "Java_doupai_venus_venus_TemplateEngine_closeMaskshape");
    if ((error = const_cast<char *>(dlerror())) != NULL) {
        LOGE("dlsym: %s\n", error);
    }
    (*cac_func)(env, thiz, l1);
    //关闭动态链接库
//    dlclose(handle);
}

void callFuncUpdateMask(JNIEnv *env, jobject thiz, jstring s, jint ii, jobject obj) {
    void *handle;
    char *error;
    CAC_FUNC_UPDATE_MASK cac_func = NULL;

    //打开动态链接库
    handle = dlopen(LIB_CACULATE_PATH, RTLD_NOW);
    if (!handle) {
        LOGV("dlopen: %s\n", dlerror());
    }

    //清除之前存在的错误
    dlerror();

    //获取一个函数
    *(void **) (&cac_func) = dlsym(handle,
                                   "Java_doupai_venus_venus_TemplateEngine_updateMask");
    if ((error = const_cast<char *>(dlerror())) != NULL) {
        LOGV("dlsym: %s\n", error);
    }
    (*cac_func)(env, thiz, s, ii, obj);
    //关闭动态链接库
//    dlclose(handle);
}

void callFuncGetFirstFrame(JNIEnv *env, jobject thiz, jstring s) {
    void *handle;
    char *error;
    CAC_FUNC_GET_FIRST_FRAME cac_func = NULL;

    //打开动态链接库
    handle = dlopen(LIB_CACULATE_PATH, RTLD_NOW);
    if (!handle) {
        LOGV("dlopen: %s\n", dlerror());
    }

    //清除之前存在的错误
    dlerror();

    //获取一个函数
    *(void **) (&cac_func) = dlsym(handle,
                                   "Java_doupai_venus_venus_TemplateEngine_getFirstFrame");
    if ((error = const_cast<char *>(dlerror())) != NULL) {
        LOGV("dlsym: %s\n", error);
    }
    (*cac_func)(env, thiz, s);
    //关闭动态链接库
//    dlclose(handle);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

JNIEXPORT void JNICALL
Java_doupai_venus_venus_NativeObject_native_1init(JNIEnv *env, jobject thiz,
                                                  jobject obj) {
    LOGE("umeng1");
    callFuncNativeInit(env, thiz, obj);
}


JNIEXPORT jstring JNICALL
Java_doupai_venus_venus_Venus_decrypt(JNIEnv *env, jobject thiz,
                                      jstring filename, jstring key) {
    LOGE("umeng2 decrypt");
//    printfJstring(env, thiz, filename);
//    printfJstring(env, thiz, key);

    jstring ret = callFunc(env, thiz, filename, key);
    writeJstringToFile(env, thiz, filename, ret);

    {
        LOGE("umeng3");
        string path = getFilePath(env, filename);
        filePath = path;
        LOGE("umeng4,%s", path.c_str());
        string footpath = path + "footage.json";
        string drawablepath = path + "drawable.json";
        string contextpath = path + "context.json";
        string animationpath = path + "animation.zip";

        LOGE("umeng5,%s", footpath.c_str());

        jstring jfootpath = env->NewStringUTF(footpath.c_str());
        jstring jdrawpath = env->NewStringUTF(drawablepath.c_str());
        jstring jconpath = env->NewStringUTF(contextpath.c_str());
        jstring janimatpath = env->NewStringUTF(animationpath.c_str());

        jstring ret1 = callFunc(env, thiz, jfootpath, key);
        writeJstringToFileNeedParseJson(env, thiz, jfootpath, ret1, 2);

        ret1 = callFunc(env, thiz, jdrawpath, key);
        //需要解析
        writeJstringToFileNeedParseJson(env, thiz, jdrawpath, ret1, 1);

        ret1 = callFunc(env, thiz, jconpath, key);
        writeJstringToFile(env, thiz, jconpath, ret1);

//        ret1 = callFunc(env, thiz, janimatpath, key);
//        writeJstringToFile(env, thiz, janimatpath, ret1);


    }
    return ret;
}

JNIEXPORT void JNICALL
Java_doupai_venus_venus_TemplateEngine_native_1create(JNIEnv *env, jobject thiz,
                                                      jstring paramString1, jstring paramString2) {
    LOGE("native_create");
    callFuncCreate(env, thiz, paramString1, paramString2);

}

JNIEXPORT jobject JNICALL
Java_doupai_venus_venus_TemplateEngine_getTemplateHeader(JNIEnv *env, jobject thiz) {
    LOGE("native_header");

    jobject obj = callFuncHeader(env, thiz);
    return obj;
}

JNIEXPORT jobject JNICALL
Java_doupai_venus_venus_TemplateEngine_getTemplateVersion(JNIEnv *env, jobject thiz) {
    LOGE("native_version");

    jobject obj = callFuncVersion(env, thiz);
    return obj;
}

JNIEXPORT void JNICALL
Java_doupai_venus_venus_TemplateEngine_native_1setSurface(JNIEnv *env, jobject thiz, jobject obj) {
    LOGE("native_setSurface");

    callFuncSetSurface(env, thiz, obj);
}

JNIEXPORT void JNICALL
Java_doupai_venus_venus_TemplateEngine_native_1prepare(JNIEnv *env, jobject thiz) {
    LOGE("native_parpare ->%s", filePath.c_str());
    string animSrcZipPath = filePath + "animation.zip";
    string maskSrcZipPath = filePath + "maskshape.zip";
    string animTempZipPath = filePath + "animation_temp.zip";
    string maskTempZipPath = filePath + "maskshape_temp.zip";
    //解析出来的animation.json存储的位置
    string animationPath = filePath + ANIMATION;
    //解析出来的maskshape.json存储的位置
    string makeshapePath = filePath + MAKESHAPE;
    //创建animation和maskshape文件夹
    int animDirStatus = mkdir(animationPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    int makeDirStatus = mkdir(makeshapePath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    //调用他们的nativePrepare
    callFuncNativePrepare(env, thiz);
    LOGE("native_parpare1");
    //判断是否有maskshape.zip 存在为0 不存在为-1
    int maskStatus = File_ISExist(maskSrcZipPath.c_str());
    int animStatus = File_ISExist(animSrcZipPath.c_str());
    LOGE("maskshape:%s 状态->%d anim:%s 状态->%d", maskSrcZipPath.c_str(), maskStatus,
         animSrcZipPath.c_str(), animStatus);
    if (maskStatus != 0 && animStatus != 0) {
        LOGE("------------------------------------zip都不存在，不需要转化---------------------------------------------------");
        return;
    }

    //存在maskshape.zip openMask readMask closeMask
    if (maskStatus == 0) {
        jlong l1 = callFuncMask(env, thiz);
        LOGE("do decrype maskshape.zip");

        for (int ix = 0; ix < masknames.size(); ++ix) {
            //解析maskshape.zip
            jstring s = (env)->NewStringUTF(masknames[ix].c_str());
            jstring ss = callFuncReadMask(env, thiz, l1, s);
            string fullName = makeshapePath + masknames[ix];
            LOGE("begin parse maskshape json file:%s ", fullName.c_str());
            writeNewJstringToFile(env, thiz, fullName, ss, 2);
        }

        callFuncCloseMask(env, thiz, l1);
        LOGE("------------------------------------mask complete---------------------------------------------------");
    }
    //maskshape->maskshape_temp
    rename(maskSrcZipPath.c_str(), maskTempZipPath.c_str());
    jlong l2;
    //存在animation.zip
    if (animStatus == 0) {
        LOGE("do decrype animation.zip");
        //animation->maskshape
        rename(animSrcZipPath.c_str(), maskSrcZipPath.c_str());
        LOGE("do decrype animation.zip1");
        l2 = callFuncMask(env, thiz);

        LOGE("do decrype animation.zip2");
        for (int ix = 0; ix < filenames.size(); ++ix) {
            //解析Animation.zip
            jstring s = (env)->NewStringUTF(filenames[ix].c_str());
            jstring ss = callFuncReadMask(env, thiz, l2, s);
            string fullName = animationPath + filenames[ix];
            LOGE("begin parse Animation json file:%s ", fullName.c_str());
            writeNewJstringToFile(env, thiz, fullName, ss, 1);
        }

        LOGE("do decrype animation.zip3");
        //maskshape->animation
        rename(maskSrcZipPath.c_str(), animSrcZipPath.c_str());
        LOGE("do decrype animation.zip4");
    }

    //maskshape_temp->maskshape
    rename(maskTempZipPath.c_str(), maskSrcZipPath.c_str());

    if (animStatus == 0) {
        callFuncCloseMask(env, thiz, l2);
        LOGE("do decrype animation.zip5");
    }
    LOGE("------------------------------------all complete---------------------------------------------------");

}

JNIEXPORT void JNICALL
Java_doupai_venus_venus_TemplateEngine_addImage(JNIEnv *env, jobject thiz, jstring jstring1,
                                                jobject obj) {
    LOGE("native_add_image");
    callFuncAddImage(env, thiz, jstring1, obj);
}
//Java_doupai_venus_venus_TemplateEngine_addWebp	.text	0000000000063528	00000128	00000038	00000000	R	.	.	.	B	.	.
JNIEXPORT void JNICALL
Java_doupai_venus_venus_TemplateEngine_addWebp(JNIEnv *env, jobject thiz,
                                               jstring s1, jstring s2,
                                               jint i1, jint i2) {
    LOGE("native_add_webp");
    callFuncAddWebp(env, thiz, s1, s2, i1, i2);
}

JNIEXPORT jboolean JNICALL
Java_doupai_venus_venus_TemplateEngine_createJustNow(JNIEnv *env, jobject thiz,
                                                     jstring s1) {
    LOGE("createJustNow");
    jboolean is = callFuncCreateJustNow(env, thiz, s1);
    return is;
}

JNIEXPORT void JNICALL
Java_doupai_venus_venus_TemplateEngine_native_1start(JNIEnv *env, jobject thiz) {
    LOGE("native_1start");
    callFuncStart(env, thiz);
}

JNIEXPORT void JNICALL
Java_doupai_venus_venus_TemplateEngine_decodeWebpAnimations(JNIEnv *env, jobject thiz, jint i1) {
//    LOGE("decodeWebpAnimations ");
    callFuncDecodeWebpAnim(env, thiz, i1);
}

JNIEXPORT void JNICALL
Java_doupai_venus_venus_TemplateEngine_renderFrame(JNIEnv *env, jobject thiz, jlong l1) {
//    LOGV("renderFrame -> %l", l1);
//    LOGE("renderFrame");
    callFuncReadFrame(env, thiz, l1);
}

JNIEXPORT void JNICALL
Java_doupai_venus_venus_TemplateEngine_native_1destroy(JNIEnv *env, jobject thiz) {
    LOGE("native_1destroy");
    callFuncDestroy(env, thiz);
}

JNIEXPORT void JNICALL
Java_doupai_venus_venus_TemplateEngine_sourceWithVideo(JNIEnv *env, jobject thiz, jstring str) {
//    LOGE("sourceWithVideo");
    callFuncSourceVideo(env, thiz, str);
}
JNIEXPORT void JNICALL
Java_doupai_venus_venus_TemplateEngine_createVideoTexture(JNIEnv *env, jobject thiz, jstring str,
                                                          jint i1, jint i2, jint i3) {
    LOGE("createVideoTexture");
    callFuncCreateVT(env, thiz, str, i1, i2, i3);
}

JNIEXPORT void JNICALL
Java_doupai_venus_venus_TemplateEngine_updateVideo(JNIEnv *env, jobject thiz, jstring str) {
//    LOGE("updateVideo");
    callFuncUpdateVideo(env, thiz, str);
}

JNIEXPORT jlong JNICALL
Java_doupai_venus_venus_TemplateEngine_openMaskshape(JNIEnv *env, jobject thiz) {
    LOGE("openMaskshape");
    jlong l = callFuncMask(env, thiz);

    LOGE("1111111111111111111 id->   %lld", l);
    return l;
}

JNIEXPORT jstring JNICALL
Java_doupai_venus_venus_TemplateEngine_readMaskshape(JNIEnv *env, jobject thiz, jlong l1,
                                                     jstring s) {

    LOGE("readMaskshape");
    jstring ss = callFuncReadMask(env, thiz, l1, s);

    return ss;
}

JNIEXPORT void JNICALL
Java_doupai_venus_venus_TemplateEngine_closeMaskshape(JNIEnv *env, jobject thiz, jlong l1) {
    LOGE("closeMaskshape");
    callFuncCloseMask(env, thiz, l1);
}

JNIEXPORT void JNICALL
Java_doupai_venus_venus_TemplateEngine_updateMask(JNIEnv *env, jobject thiz, jstring s1,
                                                  jint i1,
                                                  jobject obj) {
    callFuncUpdateMask(env, thiz, s1, i1, obj);
}

JNIEXPORT jint JNICALL
Java_doupai_venus_venus_TemplateEngine_getFirstFrame(JNIEnv *env, jobject thiz, jstring s1) {
    callFuncGetFirstFrame(env, thiz, s1);
}


} //extern "C"

