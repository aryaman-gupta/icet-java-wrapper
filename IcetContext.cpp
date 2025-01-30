#include "IcetContext.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <cstring>

// TODO: generalize to use external MPI communicator
MPI_Comm getMPICommunicator() {
    return MPI_COMM_WORLD;
}

IcetContext::IcetContext()
        : m_comm(ICET_COMM_NULL)
        , m_context(nullptr)
{
    // Initialize MPI if it is not already initialized
    int mpiInitialized;
    MPI_Initialized(&mpiInitialized);
    if (!mpiInitialized) {

        std::cout << "MPI not initialized, initializing now" << std::endl;

        MPI_Init(nullptr, nullptr);
        mpiSelfInitialized = true;
    }
}

IcetContext::~IcetContext()
{
    if (mpiSelfInitialized) {
        MPI_Finalize();
    }
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

void IcetContext::setProcessorCentroid(int processorID, const std::vector<float>& position)
{
    m_procPositions[processorID] = position;
}

auto IcetContext::compositeFrame(
        void *subImage,
        const float *camPos,
        int windowWidth,
        int windowHeight) -> Bytes
{
    return composite(windowWidth, windowHeight, [&]([[maybe_unused]] int commRank, int commSize) {
        icetEnable(ICET_ORDERED_COMPOSITE);

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

        icetSetDepthFormat(ICET_IMAGE_DEPTH_NONE);
        icetCompositeOrder(procs.data());

        IceTFloat background_color[4] = { 0.f, 0.f, 0.f, 0.f };
        return icetCompositeImage(
                subImage,      // color buffer
                nullptr,       // geometry/depth buffer
                nullptr,
                nullptr,
                nullptr,
                background_color
        );
    });
}

auto IcetContext::compositeLayered(
        void *colorData,
        void *depthData,
        int windowWidth,
        int windowHeight,
        int numLayers) -> Bytes
{
    return composite(windowWidth, windowHeight, [&](
            [[maybe_unused]] int commRank,
            [[maybe_unused]] int commSize)
    {
        icetSetDepthFormat(ICET_IMAGE_DEPTH_FLOAT);

        IceTFloat background_color[4] = { 0.f, 0.f, 0.f, 0.f };
        return icetCompositeImageLayered(
                colorData,
                depthData,
                numLayers,
                nullptr,
                nullptr,
                nullptr,
                background_color
        );
    });
}

template<typename TDoComposite>
auto IcetContext::composite(
        int windowWidth,
        int windowHeight,
        TDoComposite doComposite) -> Bytes
{
    // Make sure we have a valid context
    if (m_context == nullptr) {
        std::cerr << "Error: Called compositeFrame without a valid IceT context.\n";
        return {nullptr, 0};
    }

    // Switch to our context
    icetSetContext(m_context);

    // Example from your snippet:
    icetSetColorFormat(ICET_IMAGE_COLOR_RGBA_UBYTE);

    icetStrategy(ICET_STRATEGY_SEQUENTIAL);
    icetSingleImageStrategy(ICET_SINGLE_IMAGE_STRATEGY_RADIXK);

    icetCompositeMode(ICET_COMPOSITE_MODE_BLEND);

    // Assume we want the actual MPI communicator for ranking:
    int commRank, commSize;
    MPI_Comm mpiComm = getMPICommunicator();
    MPI_Comm_rank(mpiComm, &commRank);
    MPI_Comm_size(mpiComm, &commSize);

    IceTImage image = doComposite(commRank, commSize);

    if (commRank == 0) {
        const IceTUByte* colorData = icetImageGetColorub(image);
        if (!colorData) {
            // This could be null if something's off with the image or strategy
            return {nullptr, 0};
        }
        size_t dataSize = static_cast<size_t>(windowWidth) * windowHeight * 4;

        if (m_colorBuffer.size() < dataSize) {
            m_colorBuffer.resize(dataSize);
        }

        std::memcpy(m_colorBuffer.data(), colorData, dataSize);

        return {m_colorBuffer.data(), dataSize};
    } else {
        return {nullptr, 0};
    }
}
