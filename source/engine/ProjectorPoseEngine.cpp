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

#include <ProjectorPoseEngine.h>

#include <buffer/ImageBuffer.h>
#include <buffer/ImageBufferMgr.h>
#include <buffer/FlatBuffer.h>
#include <buffer/FlatBufferMgr.h>

#include <arx/ARXBufferTypes.h>
#include <arx/ARXStatus.h>
#include <arx/ARXProperties.h>
#include <arx_debug.h>

namespace tiarx {

ARXEngine *ARXEngineFactory() {
    return new ProjectorPoseEngine();
}

ProjectorPoseEngine::ProjectorPoseEngine() :
        ARXEngine() {
    ARX_PRINT(ARX_ZONE_API, "%s\n", __FUNCTION__);
    sp<FlatBufferMgr> mgr = new FlatBufferMgr(BUFF_PROJECTOR_POSE, sizeof(ARXProjectorPose));
    mFlatBuffMgrMap.add(BUFF_PROJECTOR_POSE, mgr);

    mARHandle = NULL;
    mReset = false;
    mInitialDelay = 35; //number of frames to wait until we start processing
    m_imgdbg_enabled = false_e;
    if (m_imgdbg_enabled) {
        strcpy(m_imgdbg_path, PATH_DELIM"sdcard"PATH_DELIM"raw"PATH_DELIM);
    }

    m_fps = 30;
    m_focus = VCAM_FOCUS_CONTROL_OFF;
    m_focusDepth = 30;
    mCam2ALockDelay = 30;
    mARTIBuffer = NULL;
    mARTITempBuffer = NULL;

    appGallery.projW = 854;
    appGallery.projH = 480;

    mEnableFilter = true;
    mAlpha1 = 0.95;
    mAlpha2 = 0.25;
    mPrevARStatus = 0;
}

ProjectorPoseEngine::~ProjectorPoseEngine() {
    ARX_PRINT(ARX_ZONE_API, "%s\n", __FUNCTION__);
}

void ProjectorPoseEngine::defineVirtualObject() {
    int i;
    float z = appGallery.objZ;
    int SF = 1;
    float offsetx, offsety;
    float pRatio = (float) appGallery.projW / (float) appGallery.projH;

    offsetx = appGallery.objScale;
    offsety = (1.0f * appGallery.objScale) / pRatio;

    virtObject.numFaces = 1;
    for (i = 0; i < virtObject.numFaces; i++)
        virtObject.faces[i].numPoints = 4;

    // face 0
    virtObject.faces[0].objP[3].p[0] = appGallery.objX - offsetx;
    virtObject.faces[0].objP[3].p[1] = appGallery.objY - offsety;
    virtObject.faces[0].objP[3].p[2] = z * SF;  // z
    virtObject.faces[0].objP[3].p[3] = 1;

    virtObject.faces[0].objP[2].p[0] = appGallery.objX + offsetx;
    virtObject.faces[0].objP[2].p[1] = appGallery.objY - offsety;
    virtObject.faces[0].objP[2].p[2] = z * SF;
    virtObject.faces[0].objP[2].p[3] = 1;

    virtObject.faces[0].objP[1].p[0] = appGallery.objX + offsetx;
    virtObject.faces[0].objP[1].p[1] = appGallery.objY + offsety;
    virtObject.faces[0].objP[1].p[2] = z * SF;
    virtObject.faces[0].objP[1].p[3] = 1;

    virtObject.faces[0].objP[0].p[0] = appGallery.objX - offsetx;
    virtObject.faces[0].objP[0].p[1] = appGallery.objY + offsety;
    virtObject.faces[0].objP[0].p[2] = z * SF;
    virtObject.faces[0].objP[0].p[3] = 1;
}

void ProjectorPoseEngine::transformCameratoProjector(float B[], float Bproj[], float RelCamProj[]) {
    Bproj[0] = RelCamProj[0] * B[0] + RelCamProj[1] * B[4] + RelCamProj[2] * B[8];
    Bproj[1] = RelCamProj[0] * B[1] + RelCamProj[1] * B[5] + RelCamProj[2] * B[9];
    Bproj[2] = RelCamProj[0] * B[2] + RelCamProj[1] * B[6] + RelCamProj[2] * B[10];

    Bproj[4] = RelCamProj[4] * B[0] + RelCamProj[5] * B[4] + RelCamProj[6] * B[8];
    Bproj[5] = RelCamProj[4] * B[1] + RelCamProj[5] * B[5] + RelCamProj[6] * B[9];
    Bproj[6] = RelCamProj[4] * B[2] + RelCamProj[5] * B[6] + RelCamProj[6] * B[10];

    Bproj[8] = RelCamProj[8] * B[0] + RelCamProj[9] * B[4] + RelCamProj[10] * B[8];
    Bproj[9] = RelCamProj[8] * B[1] + RelCamProj[9] * B[5] + RelCamProj[10] * B[9];
    Bproj[10] = RelCamProj[8] * B[2] + RelCamProj[9] * B[6] + RelCamProj[10] * B[10];

    Bproj[3] = RelCamProj[0] * B[3] + RelCamProj[1] * B[7] + RelCamProj[2] * B[11] + RelCamProj[3];
    Bproj[7] = RelCamProj[4] * B[3] + RelCamProj[5] * B[7] + RelCamProj[6] * B[11] + RelCamProj[7];
    Bproj[11] = RelCamProj[8] * B[3] + RelCamProj[9] * B[7] + RelCamProj[10] * B[11]
            + RelCamProj[11];
}

void ProjectorPoseEngine::moveProjector(float B[], float offset) {
    float C[3];

    // Compute projector center
    // C = -inv(R) * T = -transpose(R) * T
    // B is of the form [R T]
    C[0] = -1 * (B[0] * B[3] + B[4] * B[7] + B[8] * B[11]);
    C[1] = -1 * (B[1] * B[3] + B[5] * B[7] + B[9] * B[11]);
    C[2] = -1 * (B[2] * B[3] + B[6] * B[7] + B[10] * B[11]);

    // Move the camera center to the new location
    C[2] = C[2] + offset;

    // Recompute the translation vector
    // T = -R*C
    B[3] = -1 * (B[0] * C[0] + B[1] * C[1] + B[2] * C[2]);
    B[7] = -1 * (B[4] * C[0] + B[5] * C[1] + B[6] * C[2]);
    B[11] = -1 * (B[8] * C[0] + B[9] * C[1] + B[10] * C[2]);
}

void ProjectorPoseEngine::getImageCoordinatesOfObject(float P[], AR_object *obj, int imgW,
        int imgH) {
    int i, j;
    float imageP[3];
    for (i = 0; i < obj->numFaces; i++) {
        for (j = 0; j < obj->faces[i].numPoints; j++) {
            matrixVectorMultiply_3x4_4x1(P, obj->faces[i].objP[j].p, imageP);
            obj->faces[i].imageP[j].p[0] = (float) imageP[0] / (float) imageP[2] + 0.5 * imgW;
            obj->faces[i].imageP[j].p[1] = (float) imageP[1] / (float) imageP[2] + 0.5 * imgH;
        }
    }
}

void ProjectorPoseEngine::matrixMultiply3x3_3x4_float(float K[], float B[], float P[]) {
    // P = K * B
    // K is 3x3
    // B is 3x4
    // P is 3x4

    int i, j, k;
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 4; j++) {
            P[i * 4 + j] = 0;
            for (k = 0; k < 3; k++) {
                P[i * 4 + j] += K[i * 3 + k] * B[k * 4 + j];
            }
        }
    }
}

void ProjectorPoseEngine::matrixVectorMultiply_3x4_4x1(float P[], float V[], float I[]) {
    // P is 3x4
    // V is 4x1
    // I = P * V is 3x1
    int i, j, k;

    for (i = 0; i < 3; i++) {
        for (j = 0; j < 1; j++) {
            I[i * 1 + j] = 0;
            for (k = 0; k < 4; k++) {
                I[i * 1 + j] += P[i * 4 + k] * V[k * 1 + j];
            }
        }
    }
}

void ProjectorPoseEngine::getHomographyToWarpContent(int projW, int projH, AR_object *obj,
        float warpH[], short *buffer) {
    int i, j;
    float Xdest[4], Ydest[4];
    float Xsrc[4], Ysrc[4];

    Xsrc[0] = 0;
    Ysrc[0] = 0;
    Xsrc[1] = projW;
    Ysrc[1] = 0;
    Xsrc[2] = projW;
    Ysrc[2] = projH;
    Xsrc[3] = 0;
    Ysrc[3] = projH;

    for (i = 0; i < obj->numFaces; i++) {
        for (j = 0; j < obj->faces[i].numPoints; j++) {
            Xdest[j] = obj->faces[i].imageP[j].p[0];
            Ydest[j] = obj->faces[i].imageP[j].p[1];
        }
    }

    AR_computeFourPointHomography_float(Xdest, Ydest, Xsrc, Ysrc, warpH, (void *) buffer);
}

/* Digital filter designed by mkfilter/mkshape/gencode   A.J. Fisher
 Command line: /www/usr/fisher/helpers/mkfilter -Bu -Lp -o 2 -a 4.0000000000e-01 0.0000000000e+00 -l */
void ProjectorPoseEngine::butterworthFilter(float input[]) {
    int i;
    for (i = 0; i < 12; i++) {
        xv[i][0] = xv[i][1];
        xv[i][1] = xv[i][2];
        xv[i][2] = xv[i][3];
        xv[i][3] = xv[i][4];
        xv[i][4] = input[i] / BWFILTER_GAIN;
        yv[i][0] = yv[i][1];
        yv[i][1] = yv[i][2];
        yv[i][2] = yv[i][3];
        yv[i][3] = yv[i][4];

        // cutoff frequency = 5Hz
        yv[i][4] = (xv[i][0] + xv[i][4]) + 4 * (xv[i][1] + xv[i][3]) + 6 * xv[i][2]
                + (-0.0557639007 * yv[i][0]) + (0.3623690447 * yv[i][1])
                + (-1.0304538354 * yv[i][2]) + (1.3066051440 * yv[i][3]);

        input[i] = yv[i][4];
    }
}

arxstatus_t ProjectorPoseEngine::Setup() {
    arxstatus_t status = ARXEngine::Setup();
    if (status != NOERROR) {
        ARX_PRINT(ARX_ZONE_ERROR, "Failed setting up base engine! 0x%x", status);
        return status;
    }

    const sp<ImageBufferMgr>& camMgr = mImgBuffMgrMap.valueFor(BUFF_CAMOUT2);

    uint32_t w = camMgr->width();
    uint32_t h = camMgr->height();
    uint32_t size = w * h;

    //Default 12MP Sony camera, overriden if there's a calibration file
    cameraK[0] = 277.8069f;
    cameraK[1] = 0.00f;
    cameraK[2] = 161.273f;
    cameraK[3] = 0.00f;
    cameraK[4] = 278.9756f;
    cameraK[5] = 119.5077f;
    cameraK[6] = 0.00f;
    cameraK[7] = 0.00f;
    cameraK[8] = 1.0f;

    loadCalibration();
    loadGalleryParams();

    projectorFrameCounter = 0;
    projector_w = appGallery.projW;
    projector_h = appGallery.projH;

    projectorK[2] = projectorK[2] - 0.5 * projector_w;
    projectorK[5] = projectorK[5] - 0.5 * projector_h;

    mARHandle = AR_open();
    if (mARHandle == NULL) {
        ARX_PRINT(ARX_ZONE_ERROR, "Failed opening LIBARTI handle!");
        return NOMEMORY;
    }

    loadARParams();

    int bufSize = 0;
    AR_initialize(mARHandle, camMgr->width(), camMgr->height(), cameraK, &bufSize);
    AR_initializeProjectorInfo(mARHandle, projectorK, relCamProj, projector_w, projector_h);

    mARTIBuffer = (short *) malloc(bufSize);
    if (mARTIBuffer == NULL) {
        ARX_PRINT(ARX_ZONE_ERROR, "Failed allocating memory for LIBARTI!");
        return NOMEMORY;
    }

    mARTITempBuffer = (short *) malloc(4096);
    if (mARTITempBuffer == NULL) {
        ARX_PRINT(ARX_ZONE_ERROR, "Failed allocating memory for LIBARTI!");
        return NOMEMORY;
    }

    AR_configure(mARHandle, mARTIBuffer);

    mARImage.height = h;
    mARImage.width = w;
    mARImage.imageSize = size;
    mARImage.horzOffset = 0;
    mARImage.vertOffset = 0;
    mARImage.imageStride = w;
    mARImage.pixelDepth = AR_PIXEL_U08;
    mARImage.type = AR_IMG_LUMA;
    mARImage.imageCorners[0] = 0;
    mARImage.imageCorners[1] = 0;
    mARImage.imageCorners[2] = w;
    mARImage.imageCorners[3] = 0;
    mARImage.imageCorners[4] = w;
    mARImage.imageCorners[5] = h;
    mARImage.imageCorners[6] = 0;
    mARImage.imageCorners[7] = h;

    mARImage.imageData = calloc(1, size);
    if (mARImage.imageData == NULL) {
        ARX_PRINT(ARX_ZONE_ERROR, "Failed to allocate image data");
        return NOMEMORY;
    }

    m_procIdx = 0;

    DVP_Perf_Clear(&mPerf);

    if (m_imgdbg_enabled && AllocateImageDebug(1)) {
        sp<ImageBuffer> buf;
        DVP_Image_t camImage;
        camMgr->getImage(0, &buf);
        buf->copyInfo(&camImage, FOURCC_Y800);
        ImageDebug_Init(&m_imgdbg[0], &camImage, m_imgdbg_path, "camera");
        ImageDebug_Open(m_imgdbg, m_numImgDbg);
    }

    defineVirtualObject();

    return NOERROR;
}

arxstatus_t ProjectorPoseEngine::Process(ImageBuffer *, ImageBuffer *videoBuf) {
    const sp<FlatBufferMgr>& mgr = mFlatBuffMgrMap.valueFor(BUFF_PROJECTOR_POSE);
    FlatBuffer *buf = mgr->nextFree();
    ARXProjectorPose *m = reinterpret_cast<ARXProjectorPose *>(buf->data());

    m->pointerX = 0.0f;
    m->pointerY = 0.0f;

    if (m_capFrames > mInitialDelay) {
        DVP_Image_t im;
        videoBuf->copyInfo(&im);
        uint8_t *pDst = reinterpret_cast<uint8_t *>(mARImage.imageData);
        uint8_t *pSrc = im.pData[0];
        for (uint32_t i = 0; i < im.height; i++) {
            memcpy((void *) pDst, (void *) pSrc, im.width);
            pDst += im.width;
            pSrc += im.y_stride;
        }

        if (mReset) {
            if (m_imgdbg_enabled) {
                ImageDebug_Close(m_imgdbg, m_numImgDbg);
                ImageDebug_Open(m_imgdbg, m_numImgDbg);
            }
            int bufSize;
            AR_initialize(mARHandle, im.width, im.height, cameraK, &bufSize);
            mReset = false;
            mPrevARStatus = LOST_TRACKING;
        }

        DVP_PerformanceStart(&mPerf);
        if (mPrevARStatus != NOTENOUGH_FEATURES) {
            m->status = AR_process(mARHandle, mARImage, NULL, NULL, 0, camera_p, cameraPoseMatrix,
                    cameraCenter);
        } else {
            m->status = NOTENOUGH_FEATURES;
        }
        mPrevARStatus = m->status;

        computeProjectorPose(m);
        DVP_PerformanceStop(&mPerf);
        DVP_PerformancePrint(&mPerf, "AR_process exec");

        if (m_imgdbg_enabled) {
            DVP_Image_t camImage;
            videoBuf->copyInfo(&camImage);
            camImage.color = FOURCC_Y800;
            camImage.planes = 1;
            camImage.numBytes = camImage.width * camImage.height;
            m_imgdbg[0].pImg = &camImage;
            ImageDebug_Write(m_imgdbg, m_numImgDbg);
        }
    }
    buf->setTimestamp(videoBuf->timestamp());
    buf->ready();
    return NOERROR;
}

arxstatus_t ProjectorPoseEngine::computeProjectorPose(ARXProjectorPose *buf) {
    if (buf->status == LOST_TRACKING) {
        projectorFrameCounter = 0;
        return NOERROR;
    }

    float proj_pose[12];
    transformCameratoProjector(cameraPoseMatrix, proj_pose, relCamProj);

    // Move projector location by a pre-determined amount.
    // +ve value of displacement will move projector closer to world origin.
    // -ve values will be move projector farther away from world origin.
    // The projector moves along a vector aligned with the direction in which the projector is facing
    moveProjector(proj_pose, appGallery.projectorDisplacement);

    // Weighted update for Bproj
    float alpha;
    if (buf->status == TRACKING_AND_MOVING) {
        alpha = mAlpha1;
    } else {
        alpha = mAlpha2;
    }

    if (projectorFrameCounter <= BWFILTER_NZEROS) {
        for (int i = 0; i < 12; i++) {
            xv[i][projectorFrameCounter] = proj_pose[i] / BWFILTER_GAIN;
            yv[i][projectorFrameCounter] = proj_pose[i];
        }
    } else if (mEnableFilter) {
        butterworthFilter(proj_pose);
    }

    if (projectorFrameCounter > 0) {
        for (int i = 0; i < 12; i++)
            proj_pose[i] = prevProjPose[i] + alpha * (proj_pose[i] - prevProjPose[i]);
    }
    memcpy(prevProjPose, proj_pose, sizeof(float) * 12);

    // Compute extrinsic projector matrix
    matrixMultiply3x3_3x4_float(projectorK, proj_pose, projector_p);

    // Compute coordinates of the projector
    AR_computeCameraCoordinates(projector_p, projectorCenter);

    // Get image coordinates of virtual object using projector matrix
    getImageCoordinatesOfObject(projector_p, &virtObject, projector_w, projector_h);

    // Get the homography used to warp the digital content before projection
    getHomographyToWarpContent(projector_w, projector_h, &virtObject, warpH, mARTITempBuffer);

    //Cast a ray from camera center to z=0 plane to figure out where
    //the pointer is
    float t = -(projectorCenter[2] / proj_pose[10]);
    float pX = projectorCenter[0] + (proj_pose[8] * t);
    float pY = projectorCenter[1] + (proj_pose[9] * t);
    buf->pointerX = pX / appGallery.objScale;
    buf->pointerY = (pY - (float)appLibarti.projectorYCoord) / appGallery.objScale;

    buf->view_matrix[0] = proj_pose[0];
    buf->view_matrix[4] = proj_pose[1];
    buf->view_matrix[8] = proj_pose[2];
    buf->view_matrix[12] = proj_pose[3];

    buf->view_matrix[1] = -proj_pose[4];
    buf->view_matrix[5] = -proj_pose[5];
    buf->view_matrix[9] = -proj_pose[6];
    buf->view_matrix[13] = -proj_pose[7];

    buf->view_matrix[2] = -proj_pose[8];
    buf->view_matrix[6] = -proj_pose[9];
    buf->view_matrix[10] = -proj_pose[10];
    buf->view_matrix[14] = -proj_pose[11];

    buf->view_matrix[3] = 0;
    buf->view_matrix[7] = 0;
    buf->view_matrix[11] = 0;
    buf->view_matrix[15] = 1;

    projectorFrameCounter++;
    return NOERROR;

}
arxstatus_t ProjectorPoseEngine::DelayedCamera2ALock() {
    status_e status = STATUS_SUCCESS;
    DVP_BOOL lockState = DVP_TRUE;

    status = m_pCam->sendCommand(VCAM_CMD_LOCK_AWB, &lockState, sizeof(DVP_BOOL));
    status = m_pCam->sendCommand(VCAM_CMD_LOCK_AE, &lockState, sizeof(DVP_BOOL));
    return NOERROR;
}

void ProjectorPoseEngine::Teardown() {
    Lock();
    free(mARImage.imageData);
    free(mARTIBuffer);
    free(mARTITempBuffer);
    mARImage.imageData = NULL;

    if (mARHandle) {
        AR_close(mARHandle);
        mARHandle = NULL;
    }

    if (m_imgdbg_enabled) {
        ImageDebug_Close(m_imgdbg, m_numImgDbg);
    }
    Unlock();
    ARXEngine::Teardown();
}

arxstatus_t ProjectorPoseEngine::SetProperty(uint32_t property, int32_t value) {
    ARX_PRINT(ARX_ZONE_ENGINE, "ProjectorPoseEngine: received property %u=%d", property, value);
    switch (property) {
        case PROP_ENGINE_PROJPOSE_RESET:
            mReset = true;
            break;
        case PROP_ENGINE_PROJPOSE_ZOOM: {
            float val = *((float *) &value);
            appGallery.objScale = val;
            defineVirtualObject();
            break;
        }
        default:
            return ARXEngine::SetProperty(property, value);
    }
    ARX_PRINT(ARX_ZONE_ENGINE, "ProjectorPoseEngine: accepted property %u=%d", property, value);
    return NOERROR;
}

arxstatus_t ProjectorPoseEngine::GetProperty(uint32_t property, int32_t *value) {
    ARX_PRINT(ARX_ZONE_ENGINE, "ProjectorPoseEngine: querying property %u", property);
    switch (property) {
        default:
            return ARXEngine::GetProperty(property, value);
    }
    ARX_PRINT(ARX_ZONE_ENGINE, "ProjectorPoseEngine: queried property %u=%d", property, *value);
    return NOERROR;
}

arxstatus_t ProjectorPoseEngine::loadARParams() {
    char input[100];
    unsigned int value;
    float fvalue;
    FILE *fp = fopen("/data/misc/Parameters/libartiParameters.txt", "r");

    if (fp == NULL) {
        ARX_PRINT(ARX_ZONE_WARNING, "ProjectorPoseEngine: failed opening libarti parameter file");
        return FAILED;
    }

    while (!feof(fp)) {
        fscanf(fp, "%s", input);

        if (strcmp(input, "-harrisThreshold") == 0) {
            fscanf(fp, "%d", &value);
            mHarrisThreshold = (unsigned int) value;
        }
        if (strcmp(input, "-harrisSensitivity") == 0) {
            fscanf(fp, "%f", &fvalue);
            // converting value to SQ0.15
            mHarrisSensitivity = (short) (fvalue * 32767);
        }
        if (strcmp(input, "-nonmaxWindowSize") == 0) {
            fscanf(fp, "%d", &value);
            mNonmaxWindowSize = (short) value;
        }
        if (strcmp(input, "-featureMatchRadius") == 0) {
            fscanf(fp, "%d", &value);
            mFeatureMatchRadius = (int) value;
        }
        if (strcmp(input, "-featureMatchInlierRatio1") == 0) {
            fscanf(fp, "%f", &fvalue);
            mFeatureMatchInlierRatio1 = (float) fvalue;
        }
        if (strcmp(input, "-featureMatchInlierRatio2") == 0) {
            fscanf(fp, "%f", &fvalue);
            mFeatureMatchInlierRatio2 = (float) fvalue;
        }
        if (strcmp(input, "-featuresMinimum") == 0) {
            fscanf(fp, "%d", &value);
            mFeaturesMinimum = (int) value;
        }
        if (strcmp(input, "-extendWorkspace") == 0) {
            fscanf(fp, "%d", &value);
            mExtendWorkspace = (int) value;
        }
        if (strcmp(input, "-projectorYCoord") == 0) {
            fscanf(fp, "%d", &value);
            appLibarti.projectorYCoord = (int) value;
        }
        if (strcmp(input, "-enableFilter") == 0) {
            fscanf(fp, "%d", &value);
            ARX_PRINT(ARX_ZONE_ALWAYS, "ProjectorPoseEngine: enableFilter:%d", value);
            mEnableFilter = (value != 0);
        }
        if (strcmp(input, "-alpha1") == 0) {
            fscanf(fp, "%f", &fvalue);
            mAlpha1 = fvalue;
        }
        if (strcmp(input, "-alpha2") == 0) {
            fscanf(fp, "%f", &fvalue);
            mAlpha2 = fvalue;
        }
        if (strcmp(input, "//") == 0) {
            fscanf(fp, "%[\n]");
        }

    }

    ARX_PRINT(ARX_ZONE_ALWAYS, "ProjectorPoseEngine: mHarrisThreshold:%u", mHarrisThreshold);
    ARX_PRINT(ARX_ZONE_ALWAYS, "ProjectorPoseEngine: mHarrisSensitivity:%d", mHarrisSensitivity);
    ARX_PRINT(ARX_ZONE_ALWAYS, "ProjectorPoseEngine: mNonmaxWindowSize:%d", mNonmaxWindowSize);
    ARX_PRINT(ARX_ZONE_ALWAYS, "ProjectorPoseEngine: mFeatureMatchRadius:%d", mFeatureMatchRadius);
    ARX_PRINT(ARX_ZONE_ALWAYS, "ProjectorPoseEngine: mFeatureMatchInlierRatio1:%f",
            mFeatureMatchInlierRatio1);
    ARX_PRINT(ARX_ZONE_ALWAYS, "ProjectorPoseEngine: mFeatureMatchInlierRatio2:%f",
            mFeatureMatchInlierRatio2);
    ARX_PRINT(ARX_ZONE_ALWAYS, "ProjectorPoseEngine: mFeaturesMinimum:%d", mFeaturesMinimum);
    ARX_PRINT(ARX_ZONE_ALWAYS, "ProjectorPoseEngine: mExtendWorkspace:%d", mExtendWorkspace);
    ARX_PRINT(ARX_ZONE_ALWAYS, "ProjectorPoseEngine: mEnableFilter:%d", mEnableFilter);
    ARX_PRINT(ARX_ZONE_ALWAYS, "ProjectorPoseEngine: mAlpha1:%f", mAlpha1);
    ARX_PRINT(ARX_ZONE_ALWAYS, "ProjectorPoseEngine: mAlpha2:%f", mAlpha2);

    AR_setParameter(mARHandle, LIBARTI_HARRIS_CORNER_THRESHOLD, (void *) &mHarrisThreshold,
            AR_TYPE_U32);
    AR_setParameter(mARHandle, LIBARTI_HARRIS_SENSITIVITY, (void *) &mHarrisSensitivity,
            AR_TYPE_S16);
    AR_setParameter(mARHandle, LIBARTI_NONMAX_WINDOWSIZE, (void *) &mNonmaxWindowSize, AR_TYPE_S16);
    AR_setParameter(mARHandle, LIBARTI_FEATUREMATCH_RADIUS, (void *) &mFeatureMatchRadius,
            AR_TYPE_S32);
    AR_setParameter(mARHandle, LIBARTI_FEATUREMATCH_INLIER_RATIO_1,
            (void *) &mFeatureMatchInlierRatio1, AR_TYPE_F32);
    AR_setParameter(mARHandle, LIBARTI_FEATUREMATCH_INLIER_RATIO_2,
            (void *) &mFeatureMatchInlierRatio2, AR_TYPE_F32);
    AR_setParameter(mARHandle, LIBARTI_FEATURES_MINIMUM, (void *) &mFeaturesMinimum, AR_TYPE_S32);
    AR_setParameter(mARHandle, LIBARTI_MODE_EXTENDWORKSPACE, (void *) &mExtendWorkspace,
            AR_TYPE_S32);
    AR_setParameter(mARHandle, LIBARTI_SET_PROJECTOR_YCOORD, (void *) &appLibarti.projectorYCoord,
            AR_TYPE_S32);

    fclose(fp);
    return NOERROR;
}

arxstatus_t ProjectorPoseEngine::loadGalleryParams() {

    char input[100];
    unsigned int value;
    float fvalue;

    FILE *fp = fopen("/data/misc/Parameters/galleryParameters.txt", "r");
    if (fp == NULL) {
        ARX_PRINT(ARX_ZONE_WARNING, "ProjectorPoseEngine: failed opening gallery parameter file");
        return FAILED;
    }

    while (!feof(fp)) {
        fscanf(fp, "%s", input);

        if (strcmp(input, "-projW") == 0) {
            fscanf(fp, "%d", &value);
            appGallery.projW = (int) value;
        }
        if (strcmp(input, "-projH") == 0) {
            fscanf(fp, "%d", &value);
            appGallery.projH = (int) value;
        }
        if (strcmp(input, "-objX") == 0) {
            fscanf(fp, "%f", &fvalue);
            appGallery.objX = (float) fvalue;
        }
        if (strcmp(input, "-objY") == 0) {
            fscanf(fp, "%f", &fvalue);
            appGallery.objY = (float) fvalue;
        }
        if (strcmp(input, "-objZ") == 0) {
            fscanf(fp, "%f", &fvalue);
            appGallery.objZ = (float) fvalue;
        }
        if (strcmp(input, "-objScale") == 0) {
            fscanf(fp, "%f", &fvalue);
            appGallery.objScale = (float) fvalue;
        }
        if (strcmp(input, "-projectorDisplacement") == 0) {
            fscanf(fp, "%f", &fvalue);
            appGallery.projectorDisplacement = (float) fvalue;
            ARX_PRINT(ARX_ZONE_ALWAYS, "projectorDisplacement:%f", appGallery.projectorDisplacement);
        }
        if (strcmp(input, "//") == 0) {
            fscanf(fp, "%[\n]");
        }
    }
    fclose(fp);
    return NOERROR;
}

arxstatus_t ProjectorPoseEngine::loadCalibration() {
    FILE *fp = fopen("/data/misc/Parameters/CameraCalibration.txt", "r");
    if (fp == NULL) {
        ARX_PRINT(ARX_ZONE_WARNING, "ProjectorPoseEngine: failed opening camera calibration file");
        return FAILED;
    }
    // Read in calibration data from file: Camera intrinsics
    for (int i = 0; i < 9; i++) {
        fscanf(fp, "%f", &cameraK[i]);
    }
    fclose(fp);
    ARX_PRINT(ARX_ZONE_ALWAYS, "cameraK [%1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f]",
            cameraK[0], cameraK[1], cameraK[2], cameraK[3], cameraK[4], cameraK[5], cameraK[6], cameraK[7], cameraK[8]);
    // Read in calibration data from file: Projector intrinsics
    fp = fopen("/data/misc/Parameters/ProjectorCalibration.txt", "r");
    if (fp == NULL) {
        ARX_PRINT(ARX_ZONE_WARNING,
                "ProjectorPoseEngine: failed opening projector calibration file");
        return FAILED;
    }
    for (int i = 0; i < 9; i++) {
        fscanf(fp, "%f", &projectorK[i]);
    }
    fclose(fp);
    ARX_PRINT(ARX_ZONE_ALWAYS, "projectorK [%1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f]",
                projectorK[0], projectorK[1], projectorK[2], projectorK[3], projectorK[4], projectorK[5], projectorK[6], projectorK[7], projectorK[8]);
    // Read in calibration data from file: Relative Orientation of Camera & Projector
    //Please note This in the form of 3*4
    //so, first row would be something like,
    //[R11 R12 R13 Tx]
    //[R21 R22 R23 Ty]
    //[R31 R32 R33 Tz]
    fp = fopen("/data/misc/Parameters/RelativeOrientationCamProj.txt", "r");
    if (fp == NULL) {
        ARX_PRINT(ARX_ZONE_WARNING,
                "ProjectorPoseEngine: failed opening relative orientation calibration file");
        return FAILED;
    }

    for (int i = 0; i < 12; i++) {
        fscanf(fp, "%f", &relCamProj[i]);
    }

    fclose(fp);
    ARX_PRINT(ARX_ZONE_ALWAYS, "relCamProj [%1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f]",
                    relCamProj[0], relCamProj[1], relCamProj[2], relCamProj[3], relCamProj[4], relCamProj[5], relCamProj[6],
                    relCamProj[7], relCamProj[8], relCamProj[9], relCamProj[10], relCamProj[11]);

    return NOERROR;
}

}
