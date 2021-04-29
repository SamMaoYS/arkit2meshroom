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
    auto intrinsics_array = _linker->getIntrinsicsArray();

    vector<IndexT> views_id(views.size());
    for (auto &view_iter : views) {
        int cam_idx = stoi(utils::io::getFileName(view_iter.second->getImagePath(), false));
        views_id[cam_idx] = view_iter.second->getViewId();
    }

    VectorXf intrinsics_param = intrinsics_array.row(0);
    bool suc = _sfm_data.getIntrinsics().at(views.at(views_id[0])->getIntrinsicId())->updateFromParams( \
        {intrinsics_param(0), intrinsics_param(6), intrinsics_param(7), 0, 0, 0});

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

template <typename T>
vector<size_t> sort_indexes(const vector<T> &v) {

  // initialize original index locations
  vector<size_t> idx(v.size());
  iota(idx.begin(), idx.end(), 0);

  stable_sort(idx.begin(), idx.end(),
       [&v](size_t i1, size_t i2) {return v[i1] < v[i2];});

  return idx;
}

void Converter::buildABC() {
    _linker->linkVertices(_sfm_data);

    sfmData::Views &views = _sfm_data.getViews();

    int vert_num = _linker->getVertNum();
    auto positions = _linker->getPositions();
    auto intrinsics_array = _linker->getIntrinsicsArray();
    auto transform_array = _linker->getTransformArray();
    auto visibility = _linker->getAssociatedCameras();
    auto points_score = _linker->getAssociatedScores();

    vector<IndexT> views_id(views.size());
    for (auto &view_iter : views) {
        int cam_idx = stoi(utils::io::getFileName(view_iter.second->getImagePath(), false));
        views_id[cam_idx] = view_iter.second->getViewId();
    }

    auto isclose = [](float a, float b, float tol) { return fabs(a-b) < tol; };
    auto lum_diff = [](float a, float b) { return fabs(a-b); };

    // Pick cameras by pixel color
    int zero_viz_count = 0;
    float tol = 10;
    for (int p=0; p < visibility.size(); ++p) {
        auto & cameras = visibility[p];
        auto & score_array = points_score[p];

        int max_count = 0;
        int index = -1;
        for (int i=0; i<score_array.size(); ++i) {
            int count = 0;
            for (int j=0; j<score_array.size(); ++j) {
                if (isclose(score_array[i], score_array[j], tol))
                    count++;
            }

            if (count > max_count) {
                max_count = count;
                index = i;
            }
        }

        if (index < 0) {
            zero_viz_count++;
            continue;
        }

        float majority = score_array[index];
        vector<float> diffs(cameras.size());
        for (int idx = 0; idx < cameras.size(); ++idx) {
            // diffs[idx] = lum_diff(luminance_array[idx], majority);
            diffs[idx] = fabs(score_array[idx]);
        }
        vector<size_t> best_idx = sort_indexes<float>(diffs);
        int k = 5;
        vector<int> top_k;
        for (int i=0; i<k && i<best_idx.size(); i++) {
            top_k.emplace_back(cameras[best_idx[i]]);
        }
        cameras = top_k;
        cout << visibility[p].size() << " visible camera in point " << p << endl;
    }

    MatrixXf colormap = MatrixXf::Zero(3, positions.cols());
    for (int i=0; i<colormap.cols(); i++) {
        if (visibility[i].size() < 1)
            continue;
        // float value = float(visibility[i].size())/15.0*255.0;
        float value = 255.0;
        colormap.col(i)(0) = value;
        colormap.col(i)(1) = value;
        colormap.col(i)(2) = value;
    }
    _linker->assignColorMap(colormap);

    cout << zero_viz_count << " points have no visibility" << endl;
    cout << "Number of cameras: " << views.size() << endl;
    VectorXf intrinsics_param = intrinsics_array.row(0);
    bool suc = _sfm_data.getIntrinsics().at(views.at(views_id[0])->getIntrinsicId())->updateFromParams( \
        {intrinsics_param(0), intrinsics_param(6), intrinsics_param(7), 0, 0, 0});
    
    cout << suc << endl;

    for (auto &view_iter : views) {
        int cam_idx = stoi(utils::io::getFileName(view_iter.second->getImagePath(), false));
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
    else if (utils::io::checkExtension(filepath, ".ply"))
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

bool Converter::assignSensorDepth(const std::string& srgb_folder, const std::string& depth_folder, const std::string& output_folder) {
    mvsUtils::MultiViewParams mp(_sfm_data, srgb_folder, output_folder, "", false);

    if (!utils::io::makeCleanFolder(output_folder) || !utils::io::pathExists(srgb_folder) || !utils::io::pathExists(depth_folder))
        return false;

    long t1 = clock();
// #pragma omp parallel for
    for(int rc = 0; rc < mp.ncams; rc++) {
        const int width = mp.getWidth(rc);
        const int height = mp.getHeight(rc);

        string depth_idx = utils::io::getFileName(_sfm_data.getView(mp.getViewId(rc)).getImagePath(), false);

        std::vector<float> depth_map;
        int w, h;
        fs::path abs_dir = fs::absolute(depth_folder);
        imageIO::readImage(abs_dir.string()+"/"+depth_idx+".exr", w, h, depth_map, imageIO::EImageColorSpace::NO_CONVERSION);
        cv::Mat depth_mat(h, w, CV_32FC1, depth_map.data());
        depth_mat.setTo(NAN, depth_mat == 0);
        depth_mat.setTo(NAN, depth_mat > 4.0);
        if (width != w || height != h) {
            cv::resize(depth_mat, depth_mat, cv::Size(width, height), 0, 0);
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
        writeImage(getFileNameFromIndex(&mp, rc, mvsUtils::EFileType::depthMap, 1), width, height, depth_map_abs, EImageQuality::LOSSLESS,  colorspace, metadata);
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

