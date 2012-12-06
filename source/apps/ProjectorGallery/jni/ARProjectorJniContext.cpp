/*
 *  Copyright (C) 2012 Texas Instruments, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ARProjectorJniContext.h"

#include <arx/ARXFlatBufferMgr.h>
#include <arx/ARXImageBufferMgr.h>
#include <arx/ARXFlatBuffer.h>
#include <arx/ARXBufferTypes.h>
#include <arx/ARXJniUtil.h>
#include <arx_debug.h>

using namespace tiarx;
using namespace android;

ARProjectorJniContext::ARProjectorJniContext(JNIEnv *env, jobject obj)
{
    mutex_init(&mLock);
    mObj = env->NewGlobalRef(obj);
    env->GetJavaVM(&mVm);
    jclass cls = env->GetObjectClass(obj);
    mOnARXDeath = env->GetMethodID(cls, "onARXDeath", "()V");
    mOnBufferChanged = env->GetMethodID(cls, "onProjPoseUpdate", "(IFF)V");
    mViewMatrix = NULL;
}

ARProjectorJniContext::~ARProjectorJniContext()
{
    if (mArx) {
        mArx->destroy();
    }
    //A callback may be currently running, acquire mutex
    //before releasing reference to the float array ref
    mutex_lock(&mLock);
    JNIEnv *env = getJNIEnv();
    if (mViewMatrix) {
        env->DeleteGlobalRef(mViewMatrix);
    }
    mutex_unlock(&mLock);
    mutex_deinit(&mLock);
    env->DeleteGlobalRef(mObj);
}

bool ARProjectorJniContext::setup(jfloatArray viewMatrix)
{
    JNIEnv *env = getJNIEnv();

    mArx = ARAccelerator::create(this);
    if (mArx == NULL) {
        ARX_PRINT(ARX_ZONE_ERROR, "Could not create ARX instance!");
        return false;
    }

    arxstatus_t status = mArx->loadEngine("arxprojpose");
    if (status != NOERROR) {
        ARX_PRINT(ARX_ZONE_ERROR, "Could not load camera pose engine!");
        return false;
    }

    ARXFlatBufferMgr *mgr = mArx->getFlatBufferMgr(BUFF_PROJECTOR_POSE);
    if (mgr == NULL) {
       ARX_PRINT(ARX_ZONE_ERROR, "Could not obtain camera matrix manager!");
       return false;
    }

    mgr->registerClient(this);

    mViewMatrix = (jfloatArray)env->NewGlobalRef(viewMatrix);

    mArx->setProperty(PROP_ENGINE_STATE, ENGINE_STATE_START);

    return true;
}

void ARProjectorJniContext::onPropertyChanged(uint32_t property, int32_t value)
{
    if (property == PROP_ENGINE_STATE && value == ENGINE_STATE_DEAD) {
        JNIEnv *env = getJNIEnv();
        env->CallVoidMethod(mObj, mOnARXDeath);
    }
}

void ARProjectorJniContext::onBufferChanged(ARXFlatBuffer *pBuffer)
{
    ARXProjectorPose *p = reinterpret_cast<ARXProjectorPose *>(pBuffer->data());
    JNIEnv *env = getJNIEnv();
    mutex_lock(&mLock);

    env->SetFloatArrayRegion(mViewMatrix, 0, 16, (jfloat *)p->view_matrix);
    mutex_unlock(&mLock);
    pBuffer->release();
    env->CallVoidMethod(mObj, mOnBufferChanged, p->status, p->pointerX, p->pointerY);
}

void ARProjectorJniContext::reset()
{
    mArx->setProperty(PROP_ENGINE_PROJPOSE_RESET, true);
}

void ARProjectorJniContext::zoomChanged(jfloat val)
{
    int value = *((int *)&val);
    mArx->setProperty(PROP_ENGINE_PROJPOSE_ZOOM, value);
}

JNIEnv *ARProjectorJniContext::getJNIEnv()
{
    union {
        JNIEnv *env;
        void *env_void;
    };
    env = NULL;
    jint ret;
    ret = mVm->GetEnv(&env_void, JNI_VERSION_1_6);
    if (ret == JNI_EDETACHED) {
        ARX_PRINT(ARX_ZONE_ERROR, "the thread is not attached to JNI!\n");
    }
    return env;
}
