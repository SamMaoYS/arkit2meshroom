#include "obv_linker.h"

#include <ctime>
#include <vector>
#include <memory>

#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <omp.h>

#include <rapidjson/document.h>

#include <aliceVision/image/all.hpp>

using namespace std;

ObvLinker::ObvLinker() = default;

ObvLinker::~ObvLinker() = default;

void ObvLinker::importCameras(const std::string& filepath, int step) {
    std::clock_t start;
    double duration;
    start = std::clock();

    boost::iostreams::mapped_file mmap_file(filepath, boost::iostreams::mapped_file::readonly);
    auto f = mmap_file.const_data();

    vector<string> camera_array;
    boost::split(camera_array, f, boost::is_any_of("\n"));
    cout << camera_array.size() << endl;
    int valid_camera_num = ceil(1.0*camera_array.size() / step);

    _intrinsics_array.resize(valid_camera_num, 9);
    _transform_array.resize(valid_camera_num, 16);

    Eigen::Vector4f tmp_vec;
    tmp_vec << 1.0, -1.0, -1.0, 1.0;
    auto coordinate_transform = tmp_vec.asDiagonal();

    int cam_idx = 0;
    for (int it=0; it < camera_array.size() && !camera_array[it].empty() && camera_array[it] != "\n"; it+=step) {
        rapidjson::StringStream s(camera_array[it].c_str());
        rapidjson::Document d;
        d.ParseStream(s);

        rapidjson::Value& v_intrinsics = d["intrinsics"];
        Eigen::VectorXf intrinsics(v_intrinsics.GetArray().Size());
        for (int i=0; i<v_intrinsics.GetArray().Size(); ++i) {
            intrinsics(i) = v_intrinsics.GetArray()[i].GetFloat();
        }
        _intrinsics_array.row(cam_idx) = intrinsics;

        rapidjson::Value& v_transform = d["transform"];
        Eigen::VectorXf transform(v_transform.GetArray().Size());
        for (int i=0; i<v_transform.GetArray().Size(); ++i) {
            transform(i) = v_transform.GetArray()[i].GetFloat();
        }
        Eigen::Map<Eigen::Matrix4f> transform_m(transform.data(), 4, 4);
        transform_m = transform_m * coordinate_transform;
        Eigen::Map<Eigen::VectorXf> valid_transform(transform_m.data(), transform_m.size());

        _transform_array.row(cam_idx) = valid_transform;
        ++cam_idx;
    }

    duration = (double)( std::clock() - start ) / (double) CLOCKS_PER_SEC;
    std::cout<<"timer: "<< duration <<'\n';
}

void ObvLinker::importMesh(const string &filepath) {
    load_mesh_or_pointcloud(filepath, _faces, _positions, _normals, _colors, true);
}

void ObvLinker::exportMesh(const string &filepath) {
    write_mesh(filepath, _faces, _positions, _normals);
}

void ObvLinker::linkVertices(sfmData::SfMData & sfm_data) {
    if (!_intrinsics_array.size() || !_transform_array.size()) {
        cout << "Error: Empty cameras, please import camera data before link vertices!" << endl;
        return;
    }
    if (!_positions.size()) {
        cout << "Error: Empty position data!" << endl;
        return;
    }
    if (!_normals.size()) {
        cout << "Error: Empty vertex normal data!" << endl;
        return;
    }

    const int num_cam = _intrinsics_array.rows();
    Eigen::Matrix4Xf pos4(4, _positions.cols());
    pos4 << _positions, Eigen::RowVectorXf::Ones(_positions.cols());

    _associated_cameras.resize(_positions.cols());
    _associated_scores.resize(_positions.cols());
    cout << _associated_cameras.size() << endl;

    for (int it = 0; it < num_cam; ++it) {
        Eigen::MatrixX4f intrinsics = Eigen::MatrixX4f::Zero(3, 4);
        intrinsics.block<3,3>(0,0) = Eigen::Map<Eigen::Matrix3f>(_intrinsics_array.row(it).data(), 3,3);
        intrinsics = intrinsics / intrinsics(2, 2);
        Eigen::MatrixX4f transform = Eigen::MatrixX4f::Zero(4, 4);
        transform = Eigen::Map<Eigen::Matrix4f>(_transform_array.row(it).data(), 4, 4);
        transform = transform / transform(3, 3);
        transform = transform.inverse().eval();
        Eigen::Matrix<float, 3, 4> project = intrinsics * transform;

        Eigen::Vector3f camera_z = transform.col(2).head(3);
        Eigen::VectorXf visibility_raw = _normals.transpose() * camera_z;
        Eigen::Array<bool, Eigen::Dynamic, 1> visibility = (visibility_raw.array() < 0.0).array();

        float margin = 300;
        float w = 1920;
        float h = 1440;
        Eigen::Matrix2Xf pos2 = (project * pos4).colwise().hnormalized();
        Eigen::Array<bool, Eigen::Dynamic, 1> x_in = (pos2.row(0).array() >= (0.0+margin)).array() * (pos2.row(0).array() < (1920-margin)).array();
        Eigen::Array<bool, Eigen::Dynamic, 1> y_in = (pos2.row(1).array() >= (0.0+margin)).array() * (pos2.row(1).array() < (1440-margin)).array();
        Eigen::Array<bool, Eigen::Dynamic, 1> in = x_in * y_in * visibility;
        cout << in.cast<int>().sum() << " visible points in camera " << it << endl;

        sfmData::Views &views = sfm_data.getViews();

        vector<IndexT> views_id(views.size());
        for (auto &view_iter : views) {
            int cam_idx = stoi(utils::io::getFileName(view_iter.second->getImagePath(), false));
            views_id[cam_idx] = view_iter.second->getViewId();
        }        

        // image::Image<image::RGBfColor> image;
        // string src_img = views.at(views_id[it])->getImagePath();
        // readImage(src_img, image, image::EImageColorSpace::LINEAR);

        for (int p = 0; p < _positions.cols(); p++) {
            if (in(p)) {
                _associated_cameras[p].emplace_back(it);
                Eigen::Vector2f pixel = pos2.col(p);
                float dist = (pixel(0)-w/2.0)*(pixel(0)-w/2.0) + (pixel(1)-h/2.0)*(pixel(1)-h/2.0);
                dist = sqrt(dist);
                // float luminance = float(image(pixel(1), pixel(0)));
                // _associated_luminance[p].emplace_back(luminance);
                _associated_scores[p].emplace_back(dist);
            }
        }
    }
}



