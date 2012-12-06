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

#ifndef _TICAMERAPOSEENGINE_H_
#define _TICAMERAPOSEENGINE_H_

#include <engine/ARXEngine.h>
#include <arx/ARXBufferTypes.h>

#include <arti/LIBARTI.h>
#include <arti/ARLIB.h>

#define BWFILTER_NZEROS 4
#define BWFILTER_NPOLES 4
#define BWFILTER_GAIN     3.834690819e+01 // Cutoff frequency = 5Hz

namespace tiarx {

class ImageBuffer;

typedef struct{
    unsigned int harrisThreshold;
    short harrisSensitivity;
    short nonmaxWindowSize;
    int featureMatchRadius;
    int featureMinimum;
    float featureMatchInlierRatio1;
    float featureMatchInlierRatio2;
    int extendWorkspace;
    int projectorYCoord; // experimental: added for Pico LTI
} App_libartiParams;


typedef struct{
    int projW;
    int projH;
    float objX;
    float objY;
    float objZ;
    float objScale;
    float projectorDisplacement;
} App_galleryParams;

class ProjectorPoseEngine : public ARXEngine
{
public:
    /** Constructor */
    ProjectorPoseEngine();

    /** Deconstructor */
    virtual ~ProjectorPoseEngine();

    virtual arxstatus_t SetProperty(uint32_t property, int32_t value);
    virtual arxstatus_t GetProperty(uint32_t property, int32_t *value);
    virtual arxstatus_t Setup();
    virtual arxstatus_t Process(ImageBuffer *previewBuf, ImageBuffer *videoBuf);
    virtual void Teardown();

    /** Overloaded to set AR specific requirements */
    arxstatus_t DelayedCamera2ALock();

private:
    arxstatus_t loadARParams();
    arxstatus_t loadGalleryParams();
    arxstatus_t loadCalibration();
    arxstatus_t computeProjectorPose(ARXProjectorPose *buf);

    void defineVirtualObject();
    void moveProjector( float B[], float offset );
    void transformCameratoProjector(float B[],float Bproj[], float RelCamProj[]);
    void getHomographyToWarpContent( int projW, int projH, AR_object *obj, float warpH[], short *buffer);
    void matrixMultiply3x3_3x4_float(float A[], float B[], float C[]);
    void matrixVectorMultiply_3x4_4x1(float P[], float V[], float I[]);
    void getImageCoordinatesOfObject(float P[], AR_object *obj, int imgW, int imgH);
    void butterworthFilter(float input[]);
    int getHotSpotIndex(float warpH[], float Bproj[], AR_object *virtObject);

    AR_handle mARHandle;
    AR_image mARImage;
    AR_object virtObject;
    App_libartiParams appLibarti;
    App_galleryParams appGallery;

    float cameraK[9];
    float projectorK[9];
    float relCamProj[12];
    float camera_p[12];
    float projector_p[12];
    float cameraPoseMatrix[12];
    float cameraCenter[4];
    float projectorCenter[4];
    float prevProjPose[12];
    float warpH[9];

    float xv[12][BWFILTER_NZEROS+1];
    float yv[12][BWFILTER_NPOLES+1];

    uint32_t projector_w;
    uint32_t projector_h;

    short *mARTIBuffer;
    short *mARTITempBuffer;

    bool mReset;
    uint32_t mInitialDelay;
    DVP_Perf_t mPerf;

    uint32_t projectorFrameCounter;

    unsigned int mHarrisThreshold;
    short mHarrisSensitivity;
    short mNonmaxWindowSize;
    int mFeatureMatchRadius;
    float mFeatureMatchInlierRatio1;
    float mFeatureMatchInlierRatio2;
    int mFeaturesMinimum;
    int mExtendWorkspace;

    bool mEnableFilter;
    float mAlpha1;
    float mAlpha2;

    int mPrevARStatus;

};


}
#endif

