#ifndef CONVERT_H
#define CONVERT_H

#include <iostream>

#define DEBUG 0

#define EIGEN_MAX_ALIGN_BYTES 0
#define EIGEN_MAX_STATIC_ALIGN_BYTES 0

#include <aliceVision/sfmData/SfMData.hpp>
#include <aliceVision/sfmDataIO/sfmDataIO.hpp>
#include <aliceVision/sfmDataIO/AlembicImporter.hpp>
#include <aliceVision/sfmDataIO/AlembicExporter.hpp>

#include "obv_linker.h"

using namespace aliceVision;
using namespace aliceVision::sfmDataIO;

class ObvLinker;
class Converter: public ObvLinker {
public:
    explicit Converter();
    ~Converter();

    void importSFM(const std::string& filename);

    void importCameras(const std::string& filepath, int step=2) override;
    void importMesh(const std::string& filepath) override;

    // Assign camera poses from ARKit to Meshroom .sfm file
    void exportSFM(const std::string& filepath);

    void exportABC(const std::string& filepath);
    void exportMesh(const std::string& filepath) override;

    // Convert ARKit depth to Meshroom depth maps
    bool assignSensorDepth(const std::string& srgb_folder, const std::string& depth_folder, const std::string& output_folder);
    bool linearizeSRGB(const std::string& srgb_folder, const std::string& output_folder);

protected:
    // Assign camera poses to sfm
    void linkKnownPoses();
    void buildABC();
    void removeLandmarksWithoutObservations();

private:
    std::unique_ptr<ObvLinker> _linker;
    sfmData::SfMData _sfm_data;
};


#endif //CONVERT_H
