#include "IcetContext.h"
#include <cmath>
#include <algorithm>
#include <iostream>  // For debug prints, if desired

// Replace this with however you actually get your MPI_Comm
extern "C" MPI_Comm getMPICommunicator();

IcetContext::IcetContext()
        : m_comm(ICET_COMM_NULL)
        , m_context(ICET_INVALID_CONTEXT)
{
    // Constructor logic if needed
}

IcetContext::~IcetContext()
{
    // If you need to destroy the IceT context or communicator
    // For example: icetDestroyContext(m_context);
}

void IcetContext::setupICET(int windowWidth, int windowHeight)
{
    // 1. Retrieve your MPI_Comm from wherever you keep it
    MPI_Comm mpiComm = getMPICommunicator();
    if (mpiComm == MPI_COMM_NULL) {
        std::cerr << "Warning: MPI_COMM_NULL in setupICET\n";
        return;
    }

    // 2. Create IceT communicator from MPI
    m_comm = icetCreateMPICommunicator(mpiComm);

    // 3. Create an IceT context
    m_context = icetCreateContext(m_comm);

    // 4. Diagnostics, tile, etc.
    IceTBitField diag_level = ICET_DIAG_ALL_NODES | ICET_DIAG_WARNINGS;
    icetDiagnostics(diag_level);

    icetAddTile(0, 0, windowWidth, windowHeight, 0);
}

void IcetContext::setCentroids(const std::vector<std::vector<float>> &positions)
{
    // Just copy them into our member
    m_procPositions = positions;
}

void IcetContext::compositeFrame(
        void *subImage,
        const float *camPos,
        int windowWidth,
        int windowHeight,
        IceTCommunicator external_mpi_comm)
{
    // Make sure we have a valid context
    if (m_context == ICET_INVALID_CONTEXT) {
        std::cerr << "Error: Called compositeFrame without a valid IceT context.\n";
        return;
    }

    // Switch to our context
    icetSetContext(m_context);

    // Example from your snippet:
    icetSetColorFormat(ICET_IMAGE_COLOR_RGBA_UBYTE);
    icetSetDepthFormat(ICET_IMAGE_DEPTH_NONE);

    icetStrategy(ICET_STRATEGY_SEQUENTIAL);
    icetSingleImageStrategy(ICET_SINGLE_IMAGE_STRATEGY_RADIXK);

    icetCompositeMode(ICET_COMPOSITE_MODE_BLEND);
    icetEnable(ICET_ORDERED_COMPOSITE);

    // Assume we want the actual MPI communicator for ranking:
    int commRank, commSize;
    MPI_Comm mpiComm = getMPICommunicator();
    MPI_Comm_rank(mpiComm, &commRank);
    MPI_Comm_size(mpiComm, &commSize);

    // Build distances from camera for each process
    std::vector<float> distances(commSize);
    std::vector<int>   procs(commSize);
    for (int i = 0; i < commSize; i++) {
        float dx = m_procPositions[i][0] - camPos[0];
        float dy = m_procPositions[i][1] - camPos[1];
        float dz = m_procPositions[i][2] - camPos[2];
        distances[i] = std::sqrt(dx*dx + dy*dy + dz*dz);
        procs[i] = i;
    }

    // Sort procs by ascending distance
    std::sort(procs.begin(), procs.end(), [&](int a, int b){
        return distances[a] < distances[b];
    });

    icetCompositeOrder(procs.data());

    IceTFloat background_color[4] = { 0.f, 0.f, 0.f, 0.f };
    IceTImage image = icetCompositeImage(
            subImage,      // color buffer
            nullptr,       // geometry/depth buffer
            nullptr,
            nullptr,
            nullptr,
            background_color
    );

    // 'image' is the composited result. You can retrieve the color buffer via:
    //   const IceTUByte *color_data = icetImageGetColorub(image);
    // Then do whatever is needed with that data (e.g., pass it to Java).
}
