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
    void compositeFrame(
            void *subImage,
            const float *camPos,
            int windowWidth,
            int windowHeight,
    );

private:
    IceTCommunicator  m_comm;
    IceTContext       m_context;
    std::unordered_map<int, std::vector<float>> m_procPositions;
};

#endif //ICET_CONTEXT_HPP
