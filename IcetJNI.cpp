#include <jni.h>
#include "IcetContext.h"
#include <vector>
#include <iostream>
#include <cstring>

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

/**
 * Returns a ByteBuffer with the composited RGBA8 data on rank 0,
 * or null on non-zero ranks (assuming the chosen IceT strategy only yields the final image on rank 0).
 *
 * Kotlin signature might be:
 *     external fun compositeFrame(handle: Long,
 *                                 subImage: ByteBuffer,
 *                                 camPos: FloatArray,
 *                                 width: Int,
 *                                 height: Int): ByteBuffer?
 */
extern "C" JNIEXPORT jobject JNICALL
Java_com_example_MyIcetJNI_compositeFrame(JNIEnv* env, jclass, jlong handle,
                                        jobject subImageBuffer,
                                        jfloatArray camPosArray,
                                        jint width, jint height)
{
    IcetContext* context = reinterpret_cast<IcetContext*>(handle);
    if (!context) {
        std::cerr << "compositeFrame called with null context!" << std::endl;
        return nullptr;
    }

// Get pointer to subImage data
    void* subImage = env->GetDirectBufferAddress(subImageBuffer);
    if (!subImage) {
        std::cerr << "subImage buffer is null" << std::endl;
        return nullptr;
    }

// Get pointer to camera position array
    jfloat* camPos = env->GetFloatArrayElements(camPosArray, nullptr);
    if (!camPos) {
        std::cerr << "camPos is null" << std::endl;
        return nullptr;
    }

    auto result = context->compositeFrame(subImage, camPos, width, height);

    auto colorPtr = result.first;
    auto dataSize = result.second;

    env->ReleaseFloatArrayElements(camPosArray, camPos, 0);

    if (!colorPtr || dataSize == 0) {
        // Possibly non-root rank or an error
        return nullptr;
    }

    // Wrap the persistent buffer in a direct ByteBuffer.
    // It's valid at least until the next composite call or context destruction.
    jobject resultBuffer = env->NewDirectByteBuffer(
            const_cast<unsigned char*>(colorPtr),
            static_cast<jlong>(dataSize)
    );
    return resultBuffer;

}
