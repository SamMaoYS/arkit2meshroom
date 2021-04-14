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
    inline void importSRGB(const std::string& srgb_folder) {_srgb_folder=srgb_folder;}

    void importCameras(const std::string& filepath, int step=2) override;
    void importMesh(const std::string& filepath) override;

    void exportSFM(const std::string& filepath);
    void exportABC(const std::string& filepath);
    void exportMesh(const std::string& filepath) override;

    bool assignSensorDepth(const std::string& depth_folder, const std::string& abs_depth_folder, const std::string& output_folder);
    bool linearizeSRGB(const std::string& srgb_folder, const std::string& output_folder);

protected:
    void linkKnownPoses();
    void buildABC();
    void removeLandmarksWithoutObservations();

private:
    std::unique_ptr<ObvLinker> _linker;
    sfmData::SfMData _sfm_data;
    std::string _srgb_folder;
};


#endif //CONVERT_H