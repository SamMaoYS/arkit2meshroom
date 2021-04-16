#ifndef OBV_LINKER_H
#define OBV_LINKER_H

#define EIGEN_MAX_ALIGN_BYTES 0
#define EIGEN_MAX_STATIC_ALIGN_BYTES 0

#include "utils.h"
#include <meshio.h>

#include <aliceVision/sfmData/SfMData.hpp>
#include <aliceVision/sfmDataIO/sfmDataIO.hpp>

using namespace aliceVision;
using namespace aliceVision::sfmDataIO;

class ObvLinker {
public:
    ObvLinker();
    ~ObvLinker();

    virtual void importCameras(const std::string& filepath, int step);
    virtual void importMesh(const std::string& filepath);
    virtual void exportMesh(const std::string& filepath);
    void linkVertices(sfmData::SfMData & sfm_data);
    inline int getVertNum() const { return _positions.cols(); }

    inline const MatrixXf& getPositions() const { return _positions; };
    inline const RowMatrixX16f& getTransformArray() const { return _transform_array; };
    inline const RowMatrixX9f& getIntrinsicsArray() const { return _intrinsics_array; };
    inline const std::vector<std::vector<int>>& getAssociatedCameras() const { return _associated_cameras; }
    inline const std::vector<std::vector<float>>& getAssociatedLuminance() const { return _associated_luminance; }

private:
    MatrixXu _faces;
    MatrixXf _positions;
    MatrixXf _normals; // vertex normals
    MatrixXu8 _colors;
    RowMatrixX9f _intrinsics_array;
    RowMatrixX16f _transform_array;
    std::vector<std::vector<int>> _associated_cameras;
    std::vector<std::vector<float>> _associated_luminance;
};


#endif //OBV_LINKER_H
