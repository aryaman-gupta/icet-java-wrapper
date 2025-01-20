#ifndef ICET_CONTEXT_HPP
#define ICET_CONTEXT_HPP

#include <IceT.h>
#include <IceTMPI.h>
#include <mpi.h>
#include <vector>
#include <unordered_map>

class IcetContext {
public:
    IcetContext();
    ~IcetContext();

    void setupICET(int windowWidth, int windowHeight);
    void setProcessorCentroid(int processorID, const std::vector<float>& position);

    /**
    * Performs compositing and returns the final color buffer on rank 0,
    * or nullptr on other ranks. The returned pointer is owned by IceT
    * until the next compositing call. If you need it longer, copy it.
    */
    std::pair<const unsigned char*, size_t> compositeFrame(
            void *subImage,
            const float *camPos,
            int windowWidth,
            int windowHeight
    );

private:
    IceTCommunicator  m_comm;
    IceTContext       m_context;
    std::unordered_map<int, std::vector<float>> m_procPositions;

    // A reusable buffer to store the final composited image on rank 0:
    std::vector<unsigned char> m_colorBuffer;
    bool mpiSelfInitialized = false;
};

#endif //ICET_CONTEXT_HPP
