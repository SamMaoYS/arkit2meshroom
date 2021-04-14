#ifndef OBV_LINKER_H
#define OBV_LINKER_H

#include "utils.h"
#include <meshio.h>

class ObvLinker {
public:
    ObvLinker();
    ~ObvLinker();

    virtual void importCameras(const std::string& filepath, int step);
    virtual void importMesh(const std::string& filepath);
    virtual void exportMesh(const std::string& filepath);
    void linkVertices();
    inline int getVertNum() const { return _positions.cols(); }

    inline const MatrixXf& getPositions() const { return _positions; };
    inline const RowMatrixX16f& getTransformArray() const { return _transform_array; };
    inline const std::vector<std::vector<int>>& getAssociatedCameras() const { return _associated_cameras; }

private:
    MatrixXu _faces;
    MatrixXf _positions;
    MatrixXf _normals; // vertex normals
    MatrixXu8 _colors;
    RowMatrixX9f _intrinsics_array;
    RowMatrixX16f _transform_array;
    std::vector<std::vector<int>> _associated_cameras;
};


#endif //OBV_LINKER_H
