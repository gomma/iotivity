/*
* //******************************************************************
* //
* // Copyright 2015 Intel Corporation.
* //
* //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
* //
* // Licensed under the Apache License, Version 2.0 (the "License");
* // you may not use this file except in compliance with the License.
* // You may obtain a copy of the License at
* //
* //      http://www.apache.org/licenses/LICENSE-2.0
* //
* // Unless required by applicable law or agreed to in writing, software
* // distributed under the License is distributed on an "AS IS" BASIS,
* // WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* // See the License for the specific language governing permissions and
* // limitations under the License.
* //
* //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
*/
#include "JniOnResourceFoundListener.h"
#include "JniOcResource.h"

JniOnResourceFoundListener::JniOnResourceFoundListener(JNIEnv *env, jobject jListener,
    RemoveListenerCallback removeListenerCallback)
{
    m_jwListener = env->NewWeakGlobalRef(jListener);
    m_removeListenerCallback = removeListenerCallback;
}

JniOnResourceFoundListener::~JniOnResourceFoundListener()
{
    LOGI("~JniOnResourceFoundListener()");
    if (m_jwListener)
    {
        jint ret = JNI_ERR;
        JNIEnv *env = GetJNIEnv(ret);
        if (nullptr == env)
        {
            return;
        }
        env->DeleteWeakGlobalRef(m_jwListener);
        m_jwListener = nullptr;
        if (JNI_EDETACHED == ret)
        {
            g_jvm->DetachCurrentThread();
        }
    }
}

void JniOnResourceFoundListener::foundResourceCallback(std::shared_ptr<OC::OCResource> resource)
{
    jint ret = JNI_ERR;
    JNIEnv *env = GetJNIEnv(ret);
    if (nullptr == env)
    {
        return;
    }

    jobject jListener = env->NewLocalRef(m_jwListener);
    if (!jListener)
    {
        checkExAndRemoveListener(env);
        if (JNI_EDETACHED == ret)
        {
            g_jvm->DetachCurrentThread();
        }
        return;
    }

    jobject jResource = env->NewObject(g_cls_OcResource, g_mid_OcResource_ctor);
    if (!jResource)
    {
        env->DeleteLocalRef(jListener);
        checkExAndRemoveListener(env);
        if (JNI_EDETACHED == ret)
        {
            g_jvm->DetachCurrentThread();
        }
        return;
    }

    JniOcResource *jniOcResource = new JniOcResource(resource);
    SetHandle<JniOcResource>(env, jResource, jniOcResource);
    if (env->ExceptionCheck())
    {
        delete jniOcResource;
        env->DeleteLocalRef(jResource);
        env->DeleteLocalRef(jListener);
        checkExAndRemoveListener(env);
        if (JNI_EDETACHED == ret)
        {
            g_jvm->DetachCurrentThread();
        }
        return;
    }
    jclass clsL = env->GetObjectClass(jListener);
    if (!clsL)
    {
        delete jniOcResource;
        env->DeleteLocalRef(jResource);
        env->DeleteLocalRef(jListener);
        checkExAndRemoveListener(env);
        if (JNI_EDETACHED == ret)
        {
            g_jvm->DetachCurrentThread();
        }
        return;
    }
    jmethodID midL = env->GetMethodID(clsL, "onResourceFound", "(Lorg/iotivity/base/OcResource;)V");
    if (!midL)
    {
        delete jniOcResource;
        env->DeleteLocalRef(clsL);
        env->DeleteLocalRef(jResource);
        env->DeleteLocalRef(jListener);
        checkExAndRemoveListener(env);
        if (JNI_EDETACHED == ret)
        {
            g_jvm->DetachCurrentThread();
        }
        return;
    }
    env->CallVoidMethod(jListener, midL, jResource);
    if (env->ExceptionCheck())
    {
        LOGE("Java exception is thrown");
        delete jniOcResource;
        env->DeleteLocalRef(clsL);
        env->DeleteLocalRef(jResource);
        env->DeleteLocalRef(jListener);
        checkExAndRemoveListener(env);
        if (JNI_EDETACHED == ret)
        {
            g_jvm->DetachCurrentThread();
        }
        return;
    }

    env->DeleteLocalRef(clsL);
    env->DeleteLocalRef(jResource);
    env->DeleteLocalRef(jListener);
    if (JNI_EDETACHED == ret)
    {
        g_jvm->DetachCurrentThread();
    }
}

void JniOnResourceFoundListener::findResourceErrorCallback(const std::string& uri, const int eCode)
{
    LOGI("findResource error callback value : %d", eCode);
    jint ret = JNI_ERR;
    JNIEnv *env = GetJNIEnv(ret);
    if (nullptr == env)
    {
        return;
    }

    jobject jListener = env->NewLocalRef(m_jwListener);
    if (!jListener)
    {
        checkExAndRemoveListener(env);
        if (JNI_EDETACHED == ret)
        {
            g_jvm->DetachCurrentThread();
        }
        return;
    }

    jclass clsL = env->GetObjectClass(jListener);
    if (!clsL)
    {
        checkExAndRemoveListener(env);
        if (JNI_EDETACHED == ret)
        {
            g_jvm->DetachCurrentThread();
        }
        return;
    }

    jobject ex = GetOcException(eCode, "stack error in onFindResourceErrorCallback");
    if (!ex)
    {
        checkExAndRemoveListener(env);
        if (JNI_EDETACHED == ret)
        {
            g_jvm->DetachCurrentThread();
        }
        return;
    }


    jmethodID midL = env->GetMethodID(clsL, "onFindResourceFailed",
                                      "(Ljava/lang/Throwable;Ljava/lang/String;)V");
    if (!midL)
    {
        checkExAndRemoveListener(env);
        if (JNI_EDETACHED == ret)
        {
            g_jvm->DetachCurrentThread();
        }
        return;
    }
    env->CallVoidMethod(jListener, midL, ex, env->NewStringUTF(uri.c_str()));

    if (JNI_EDETACHED == ret)
    {
        g_jvm->DetachCurrentThread();
    }
}

void JniOnResourceFoundListener::checkExAndRemoveListener(JNIEnv* env)
{
    if (env->ExceptionCheck())
    {
        jthrowable ex = env->ExceptionOccurred();
        env->ExceptionClear();
        m_removeListenerCallback(env, m_jwListener);
        env->Throw((jthrowable)ex);
    }
    else
    {
        m_removeListenerCallback(env, m_jwListener);
    }
}
