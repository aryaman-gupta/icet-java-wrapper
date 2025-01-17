#ifndef ICET_CONTEXT_HPP
#define ICET_CONTEXT_HPP

#include <IceT.h>
#include <IceTMPI.h>
#include <mpi.h>
#include <vector>

class IcetContext {
public:
    IcetContext();
    ~IcetContext();

    void setupICET(int windowWidth, int windowHeight);
    void setCentroids(const std::vector<std::vector<float>> &positions);
    void compositeFrame(
            void *subImage,
            const float *camPos,
            int windowWidth,
            int windowHeight,
            // anything else needed for compositing
            // e.g., a pointer back to JNIEnv if you want to do callbacks in here
            // or simply return a pointer to the composited buffer
            IceTCommunicator external_mpi_comm // for demonstration
    );

private:
    IceTCommunicator  m_comm;
    IceTContext       m_context;
    std::vector<std::vector<float>> m_procPositions;
    // Other needed members (caches, buffers, etc.)
};

#endif //ICET_CONTEXT_HPP
