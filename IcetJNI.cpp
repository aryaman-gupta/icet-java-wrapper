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
Java_com_example_IcetJNI_setCentroids(JNIEnv* env, jclass, jlong handle, jobjectArray positions)
{
IcetContext* context = reinterpret_cast<IcetContext*>(handle);
if (!context) {
std::cerr << "setCentroids called with null context!" << std::endl;
return;
}

jsize numRows = env->GetArrayLength(positions);
std::vector<std::vector<float>> temp(numRows);

for (jsize i = 0; i < numRows; i++) {
jfloatArray rowObj = (jfloatArray) env->GetObjectArrayElement(positions, i);
if (!rowObj) {
continue;
}
jsize rowLen = env->GetArrayLength(rowObj);

temp[i].resize(rowLen);
env->GetFloatArrayRegion(rowObj, 0, rowLen, temp[i].data());

env->DeleteLocalRef(rowObj);
}

context->setCentroids(temp);
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

// For demonstration, we pass getMPICommunicator() as well
// (In practice, your context might already know how to get it)
extern "C" MPI_Comm getMPICommunicator();
MPI_Comm mpiComm = getMPICommunicator();

// Now invoke our method
context->compositeFrame(subImage, camPos, width, height, mpiComm);

// Release camera position array
env->ReleaseFloatArrayElements(camPosArray, camPos, 0);

// If you want to return or display the composited result to Java,
// you might do so here (e.g., create a DirectByteBuffer from the
// final color array, then call a Java method via JNI).
}
