#include <jni.h>
#include "IcetContext.h"
#include <vector>
#include <iostream>

// ------------------------------------------------------------------
// 1) Create / Destroy
// ------------------------------------------------------------------
extern "C" JNIEXPORT jlong JNICALL
Java_com_example_IcetJNI_createNativeContext(JNIEnv* env, jclass clazz)
{
    // Allocate a new context
    IcetContext* context = new IcetContext();
    // Return as jlong
    return reinterpret_cast<jlong>(context);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_IcetJNI_destroyNativeContext(JNIEnv* env, jclass clazz, jlong handle)
{
    IcetContext* context = reinterpret_cast<IcetContext*>(handle);
    if (context) {
        delete context;
        context = nullptr;
    }
}

// ------------------------------------------------------------------
// 2) setupICET
// ------------------------------------------------------------------
extern "C" JNIEXPORT void JNICALL
Java_com_example_IcetJNI_setupICET(JNIEnv* env, jclass, jlong handle, jint width, jint height)
{
    IcetContext* context = reinterpret_cast<IcetContext*>(handle);
    if (!context) {
        std::cerr << "setupICET called with null context!" << std::endl;
        return;
    }
    context->setupICET(width, height);
}

// ------------------------------------------------------------------
// 3) setCentroids
//     positions: 2D float array from Java (float[][] in Kotlin/Java)
// ------------------------------------------------------------------
extern "C" JNIEXPORT void JNICALL
Java_com_example_IcetJNI_setProcessorCentroid(JNIEnv* env, jclass, jlong handle, jint processorID, jfloatArray position)
{
    IcetContext* context = reinterpret_cast<IcetContext*>(handle);
    if (!context) {
        std::cerr << "setCentroids called with null context!" << std::endl;
        return;
    }

    jsize length = env->GetArrayLength(position);

    if (length != 3) {
        std::cerr << "setCentroids called with invalid position array length! The length should be "
                     "3 but is " << length << std::endl;
        return;
    }

    std::vector<float> positionVec(length);
    env->GetFloatArrayRegion(position, 0, length, positionVec.data());

    context->setProcessorCentroid(processorID, positionVec);
}

// ------------------------------------------------------------------
// 4) compositeFrame
// ------------------------------------------------------------------
extern "C" JNIEXPORT void JNICALL
Java_com_example_IcetJNI_compositeFrame(JNIEnv* env, jclass, jlong handle,
                                        jobject subImageBuffer,
                                        jfloatArray camPosArray,
                                        jint width, jint height)
{
    IcetContext* context = reinterpret_cast<IcetContext*>(handle);
    if (!context) {
        std::cerr << "compositeFrame called with null context!" << std::endl;
        return;
    }

// Get pointer to subImage data
    void* subImage = env->GetDirectBufferAddress(subImageBuffer);
    if (!subImage) {
        std::cerr << "subImage buffer is null" << std::endl;
        return;
    }

// Get pointer to camera position array
    jfloat* camPos = env->GetFloatArrayElements(camPosArray, nullptr);
    if (!camPos) {
        std::cerr << "camPos is null" << std::endl;
        return;
    }

    context->compositeFrame(subImage, camPos, width, height);

// Release camera position array
    env->ReleaseFloatArrayElements(camPosArray, camPos, 0);

// If you want to return or display the composited result to Java,
// you might do so here (e.g., create a DirectByteBuffer from the
// final color array, then call a Java method via JNI).
}
