#include <jni.h>
#include "IcetContext.h"
#include <vector>
#include <iostream>
#include <cstring>

// ------------------------------------------------------------------
// 1) Create / Destroy
// ------------------------------------------------------------------
extern "C" JNIEXPORT jlong JNICALL
Java_graphics_scenery_natives_IceTWrapper_createNativeContext(JNIEnv* env, jclass clazz)
{
    // Allocate a new context
    IcetContext* context = new IcetContext();
    // Return as jlong
    return reinterpret_cast<jlong>(context);
}

extern "C" JNIEXPORT void JNICALL
Java_graphics_scenery_natives_IceTWrapper_destroyNativeContext(JNIEnv* env, jclass clazz, jlong handle)
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
Java_graphics_scenery_natives_IceTWrapper_setupICET(JNIEnv* env, jclass, jlong handle, jint width, jint height)
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
Java_graphics_scenery_natives_IceTWrapper_setProcessorCentroid(JNIEnv* env, jclass, jlong handle, jint processorID, jfloatArray position)
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
 * Helper function for implementing flat and layered compositing, with common
 * argument checks and result handling. The composite call itself must be
 * provided as a callback with the signature
 * `(IcetContext&, void* colorData) -> IcetContext::Bytes`.
 */
template<typename TDoComposite>
jobject composite(JNIEnv* env, jlong handle,
        jobject subImageBuffer,
        jint width, jint height,
        TDoComposite doComposite)
{
    IcetContext* context = reinterpret_cast<IcetContext*>(handle);
    if (!context) {
        std::cerr << "composite called with null context!" << std::endl;
        return nullptr;
    }

// Get pointer to subImage data
    void* subImage = env->GetDirectBufferAddress(subImageBuffer);
    if (!subImage) {
        std::cerr << "subImage buffer is null" << std::endl;
        return nullptr;
    }

    // Perform the actual compositing.
    auto result = doComposite(*context, subImage);

    auto colorPtr = result.first;
    auto dataSize = result.second;

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
Java_graphics_scenery_natives_IceTWrapper_compositeFrame(JNIEnv* env, jclass, jlong handle,
                                        jobject subImageBuffer,
                                        jfloatArray camPosArray,
                                        jint width, jint height)
{
    return composite(env, handle, subImageBuffer, width, height, [&](
            IcetContext& context,
            void* subImage) -> IcetContext::Bytes
    {
    // Get pointer to camera position array
        jfloat* camPos = env->GetFloatArrayElements(camPosArray, nullptr);
        if (!camPos) {
            std::cerr << "camPos is null" << std::endl;
            return {nullptr, 0};
        }

        auto result = context.compositeFrame(subImage, camPos, width, height);
        env->ReleaseFloatArrayElements(camPosArray, camPos, 0);

        return result;
    });
}

/**
 * Returns a ByteBuffer with the composited RGBA8 data on rank 0,
 * or null on non-zero ranks (assuming the chosen IceT strategy only yields the final image on rank 0).
 *
 * Kotlin signature might be:
 *     external fun compositeLayered(handle: Long,
 *                                   color: ByteBuffer,
 *                                   depth: ByteBuffer,
 *                                   width: Int,
 *                                   height: Int,
 *                                   numLayers: Int): ByteBuffer?
 */
extern "C" JNIEXPORT jobject JNICALL
Java_graphics_scenery_natives_IceTWrapper_compositeLayered(JNIEnv* env, jclass, jlong handle,
                                        jobject colorBuffer,
                                        jobject depthBuffer,
                                        jint width, jint height,
                                        jint numLayers)
{
    // Get pointer to depth data.
    void* depthData = env->GetDirectBufferAddress(depthBuffer);
    if (!depthData) {
        std::cerr << "depth buffer is null" << std::endl;
        return nullptr;
    }

    return composite(env, handle, colorBuffer, width, height, [&](
            IcetContext& context,
            void* colorData) -> IcetContext::Bytes
    {
        return context.compositeLayered(colorData, depthData, width, height, numLayers);
    });
}
