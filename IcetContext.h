#ifndef ICET_CONTEXT_HPP
#define ICET_CONTEXT_HPP

#include <IceT.h>
#include <IceTMPI.h>
#include <mpi.h>
#include <vector>
#include <unordered_map>

class IcetContext {
public:
    using Bytes = std::pair<const unsigned char*, size_t>;

    IcetContext();
    ~IcetContext();

    void setupICET(int windowWidth, int windowHeight);
    void setProcessorCentroid(int processorID, const std::vector<float>& position);

    /**
    * Performs compositing and returns the final color buffer on rank 0,
    * or nullptr on other ranks. The returned pointer is owned by IceT
    * until the next compositing call. If you need it longer, copy it.
    */
    Bytes compositeFrame(
            void *subImage,
            const float *camPos,
            int windowWidth,
            int windowHeight
    );

    /**
    * Performs layered compositing and returns the final color buffer on rank 0,
    * or nullptr on other ranks. The returned pointer is owned by IceT
    * until the next compositing call. If you need it longer, copy it.
    */
    Bytes compositeLayered(
            void *colorData,
            void *depthData,
            int windowWidth,
            int windowHeight,
            int numLayers
    );

private:
    IceTCommunicator  m_comm;
    IceTContext       m_context;
    std::unordered_map<int, std::vector<float>> m_procPositions;

    // A reusable buffer to store the final composited image on rank 0:
    std::vector<unsigned char> m_colorBuffer;
    bool mpiSelfInitialized = false;

    /**
    * Helper function that performs setup common to layered and flat image.
    * It also handles the result image depending on this processe's rank.
    * The actual IceT composite function must be invoked by a callback with the
    * signature `(int commRank, int commSize) -> IceTImage`.
    */
    template<typename TDoCompositie>
    Bytes composite(
            int windowWidth,
            int windowHeight,
            TDoCompositie doComposite
    );
};

#endif //ICET_CONTEXT_HPP
