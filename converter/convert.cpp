#include "convert.h"

#include <memory>
#include <vector>

#include <aliceVision/mvsData/Point3d.hpp>
#include <aliceVision/mvsData/StaticVector.hpp>
#include <aliceVision/camera/camera.hpp>
#include <aliceVision/sfmDataIO/viewIO.hpp>

#include <aliceVision/mvsUtils/common.hpp>
#include <aliceVision/mvsUtils/fileIO.hpp>
#include <aliceVision/mvsData/imageIO.hpp>
#include <aliceVision/image/all.hpp>

#include <opencv2/opencv.hpp>

#include "utils.h"

namespace fs = std::experimental::filesystem;
using namespace std;

Converter::Converter() {
    _linker.reset(new ObvLinker());
}

Converter::~Converter() {
    _linker.reset();
}

void Converter::importSFM(const string &filename) {
    if (!sfmDataIO::Load(_sfm_data, filename, sfmDataIO::ESfMData::ALL)) {
        cerr << "The input SfMData file '" << filename << "' cannot be read.";
    }
}

void Converter::importCameras(const string &filepath, int step) {
    _linker->importCameras(filepath, step);
}

void Converter::importMesh(const string &filepath) {
    _linker->importMesh(filepath);
}

void Converter::linkKnownPoses() {
    sfmData::Views &views = _sfm_data.getViews();

    vector<string> empty;
    _sfm_data.setFeaturesFolders(empty);
    _sfm_data.setMatchesFolders(empty);

    auto transform_array = _linker->getTransformArray();

    for (auto &view_iter : views) {
        int cam_idx = stoi(utils::io::getFileName(view_iter.second->getImagePath(), false));
        Eigen::VectorXf transform = transform_array.row(cam_idx);
        Eigen::Map<Eigen::Matrix4f> transform_m(transform.data(), 4, 4);
        transform_m = transform_m.inverse().eval();
        // camera pose to camera extrinsics
        Mat34 trans34 = transform_m.block<3, 4>(0, 0).cast<double>();

        geometry::Pose3 view_transform(trans34);
        sfmData::CameraPose cam_pose(view_transform, true);
        _sfm_data.setPose(*view_iter.second, cam_pose);
    }
}

void Converter::buildABC() {
    _linker->linkVertices();

    sfmData::Views &views = _sfm_data.getViews();

    int vert_num = _linker->getVertNum();
    auto positions = _linker->getPositions();
    auto transform_array = _linker->getTransformArray();
    auto visibility = _linker->getAssociatedCameras();

    vector<IndexT> views_id(views.size());
    cout << "Number of cameras: " << views.size() << endl;

    for (auto &view_iter : views) {
        int cam_idx = stoi(utils::io::getFileName(view_iter.second->getImagePath(), false));
        views_id[cam_idx] = view_iter.second->getViewId();
        Eigen::VectorXf transform = transform_array.row(cam_idx);
        Eigen::Map<Eigen::Matrix4f> transform_m(transform.data(), 4, 4);
        // camera pose to camera extrinsics
        transform_m = transform_m.inverse().eval();
        Mat34 trans34 = transform_m.block<3, 4>(0, 0).cast<double>();

        geometry::Pose3 view_transform(trans34);
        sfmData::CameraPose cam_pose(view_transform);
        _sfm_data.setPose(*view_iter.second, cam_pose);
    }

    _sfm_data.getLandmarks().clear();

    const double unknownScale = 0.0;
    for (std::size_t i = 0; i < vert_num; ++i) {
        const Vec3 &point = positions.col(i).cast<double>();
        sfmData::Landmark landmark(point, feature::EImageDescriberType::UNKNOWN);
        // set landmark observations from ptsCams if any
        if (!visibility[i].empty()) {
            for (int cam : visibility[i]) {
                const sfmData::View &view = _sfm_data.getView(views_id[cam]);
                const camera::IntrinsicBase *intrinsicPtr = _sfm_data.getIntrinsicPtr(view.getIntrinsicId());
                const sfmData::Observation observation(
                        intrinsicPtr->project(_sfm_data.getPose(view).getTransform(), point, true), UndefinedIndexT,
                        unknownScale); // apply distortion
                landmark.observations[view.getViewId()] = observation;
            }
        }
        _sfm_data.getLandmarks()[i] = landmark;
    }

    this->removeLandmarksWithoutObservations();
}

void Converter::exportSFM(const std::string &filepath) {
    this->linkKnownPoses();
    if (utils::io::checkExtension(filepath, ".sfm"))
        sfmDataIO::Save(_sfm_data, filepath, sfmDataIO::ESfMData::ALL_DENSE);
}

void Converter::exportABC(const string &filepath) {
    this->buildABC();
    if (utils::io::checkExtension(filepath, ".abc"))
        sfmDataIO::Save(_sfm_data, filepath, sfmDataIO::ESfMData::ALL_DENSE);
}

void Converter::exportMesh(const std::string& filepath) {
    if (utils::io::checkExtension(filepath, ".obj"))
        _linker->exportMesh(filepath);
}

void Converter::removeLandmarksWithoutObservations() {
    auto &landmarks = _sfm_data.getLandmarks();
    for (auto it = landmarks.begin(); it != landmarks.end();) {
        if (it->second.observations.empty())
            it = landmarks.erase(it);
        else
            ++it;
    }
}

bool Converter::assignSensorDepth(const std::string& depth_folder, const std::string& abs_depth_folder, const std::string& output_folder) {
    mvsUtils::MultiViewParams mp(_sfm_data, "", depth_folder, output_folder, "", true);

    if (!utils::io::makeCleanFolder(output_folder) || !utils::io::pathExists(depth_folder) || !utils::io::pathExists(abs_depth_folder))
        return false;

    long t1 = clock();
// #pragma omp parallel for
    for(int rc = 0; rc < mp.ncams; rc++) {
        int w = mp.getWidth(rc);
        int h = mp.getHeight(rc);

        string abs_depth_idx = utils::io::getFileName(_sfm_data.getView(mp.getViewId(rc)).getImagePath(), false);

        std::vector<float> depth_map;
        int width, height;
        fs::path abs_dir = fs::absolute(abs_depth_folder);
        imageIO::readImage(abs_dir.string()+"/"+abs_depth_idx+".exr", width, height, depth_map, imageIO::EImageColorSpace::NO_CONVERSION);
        cv::Mat depth_mat(height, width, CV_32FC1, depth_map.data());
        depth_mat.setTo(NAN, depth_mat == 0);
        if (width != w || height != h) {
            cv::resize(depth_mat, depth_mat, cv::Size(w, h), 0, 0);
        }
        double min_depth, max_depth;
        cv::minMaxLoc(depth_mat, &min_depth, &max_depth);
        cv::patchNaNs(depth_mat, -1);
        std::vector<float> depth_map_abs(depth_mat.begin<float>(), depth_mat.end<float>());
        const int nb_depth_values = std::count_if(depth_map_abs.begin(), depth_map_abs.end(), [](float v) { return v > 0.0f; });

        oiio::ParamValueList metadata = imageIO::getMetadataFromMap(mp.getMetadata(rc));
        metadata.push_back(oiio::ParamValue("AliceVision:nbDepthValues", oiio::TypeDesc::INT32, 1, &nb_depth_values));
        metadata.push_back(oiio::ParamValue("AliceVision:downscale", mp.getDownscaleFactor(rc)));
        metadata.push_back(oiio::ParamValue("AliceVision:CArr", oiio::TypeDesc(oiio::TypeDesc::DOUBLE, oiio::TypeDesc::VEC3), 1, mp.CArr[rc].m));
        metadata.push_back(oiio::ParamValue("AliceVision:iCamArr", oiio::TypeDesc(oiio::TypeDesc::DOUBLE, oiio::TypeDesc::MATRIX33), 1, mp.iCamArr[rc].m));
        
        
        metadata.push_back(oiio::ParamValue("AliceVision:maxDepth", (float)max_depth));
        metadata.push_back(oiio::ParamValue("AliceVision:minDepth", (float)min_depth));
        
        std::vector<double> matrix_proj = mp.getOriginalP(rc);
        metadata.push_back(oiio::ParamValue("AliceVision:P", oiio::TypeDesc(oiio::TypeDesc::DOUBLE, oiio::TypeDesc::MATRIX44), 1, matrix_proj.data()));
        
        using namespace imageIO;
        OutputFileColorSpace colorspace(EImageColorSpace::NO_CONVERSION);
        writeImage(getFileNameFromIndex(&mp, rc, mvsUtils::EFileType::depthMap, 0), w, h, depth_map_abs, EImageQuality::LOSSLESS,  colorspace, metadata);
    }
    cout << mvsUtils::formatElapsedTime(t1) << endl;

    return true;
}

bool Converter::linearizeSRGB(const std::string& srgb_folder, const std::string& output_folder) {
    if (!utils::io::makeCleanFolder(output_folder) || !utils::io::pathExists(srgb_folder))
        return false;

    const float median_camera_exposure = _sfm_data.getMedianCameraExposureSetting();
    sfmData::Views &views = _sfm_data.getViews();
    for (auto &view_iter : views) {
        image::Image<image::RGBfColor> image;
        string src_img = view_iter.second->getImagePath();
        readImage(src_img, image, image::EImageColorSpace::LINEAR);

        float camera_exposure = view_iter.second->getCameraExposureSetting();
        float ev = std::log2(1.0 / camera_exposure);
        float exposureCompensation = median_camera_exposure / camera_exposure;
        oiio::ParamValueList metadata = image::readImageMetadata(src_img);
        metadata.push_back(oiio::ParamValue("AliceVision:EV", ev));
        metadata.push_back(oiio::ParamValue("AliceVision:EVComp", exposureCompensation));

        fs::path abs_dir = fs::absolute(output_folder);
        writeImage(abs_dir.string()+"/"+ to_string(view_iter.second->getViewId())+".exr", image, image::EImageColorSpace::AUTO, metadata);
    }

    return true;
}
