#include <iostream>
#include "convert.h"

int main(int argc, char *argv[]) {
    std::vector<std::string> args;
    std::string in_abc, in_sfm;
    std::string in_trajectory, in_mesh, in_exr, in_exr_abs;
    std::string in_srgb;
    std::string out_abc, out_sfm, out_mesh;
    std::string out_srgb, out_exr;
    int step = 1;
    bool help = false;

    try {
        for (int i = 1; i < argc; ++i) {
            if (strcmp("--help", argv[i]) == 0 || strcmp("-h", argv[i]) == 0) {
                help = true;
            }
            else if (strcmp("--in_abc", argv[i]) == 0) {
                if (++i >= argc) {
                    cerr << "Missing alembic input file argument!" << endl;
                    return -1;
                }
                in_abc = argv[i];
            }
            else if (strcmp("--in_sfm", argv[i]) == 0) {
                if (++i >= argc) {
                    cerr << "Missing camera sfm input file argument!" << endl;
                    return -1;
                }
                in_sfm = argv[i];
            }
            else if (strcmp("--in_traj", argv[i]) == 0) {
                if (++i >= argc) {
                    cerr << "Missing camera trajectory input file argument!" << endl;
                    return -1;
                }
                in_trajectory = argv[i];
            }
            else if (strcmp("--in_exr", argv[i]) == 0) {
                if (++i >= argc) {
                    cerr << "Missing input depth map folder!" << endl;
                    return -1;
                }
                in_exr = argv[i];
            }
            else if (strcmp("--in_exr_abs", argv[i]) == 0) {
                if (++i >= argc) {
                    cerr << "Missing input absolute depth map folder!" << endl;
                    return -1;
                }
                in_exr_abs = argv[i];
            }
            else if (strcmp("--in_mesh", argv[i]) == 0) {
                if (++i >= argc) {
                    cerr << "Missing mesh input file argument!" << endl;
                    return -1;
                }
                in_mesh = argv[i];
            }
            else if (strcmp("--in_srgb", argv[i]) == 0) {
                if (++i >= argc) {
                    cerr << "Missing sRGB folder argument!" << endl;
                    return -1;
                }
                in_srgb = argv[i];
            }
            else if (strcmp("--out_abc", argv[i]) == 0) {
                if (++i >= argc) {
                    cerr << "Missing alembic output file argument!" << endl;
                    return -1;
                }
                out_abc = argv[i];
            }
            else if (strcmp("--out_sfm", argv[i]) == 0) {
                if (++i >= argc) {
                    cerr << "Missing camera sfm output file argument!" << endl;
                    return -1;
                }
                out_sfm = argv[i];
            }
            else if (strcmp("--out_mesh", argv[i]) == 0) {
                if (++i >= argc) {
                    cerr << "Missing mesh output file argument!" << endl;
                    return -1;
                }
                out_mesh = argv[i];
            }
            else if (strcmp("--out_srgb", argv[i]) == 0) {
                if (++i >= argc) {
                    cerr << "Missing color images output directory argument!" << endl;
                    return -1;
                }
                out_srgb = argv[i];
            }
            else if (strcmp("--out_exr", argv[i]) == 0) {
                if (++i >= argc) {
                    cerr << "Missing depth images output directory argument!" << endl;
                    return -1;
                }
                out_exr = argv[i];
            }
            else if (strcmp("--step", argv[i]) == 0) {
                if (++i >= argc) {
                    cerr << "Missing camera skipping step size argument!" << endl;
                    return -1;
                }
                step = std::stoi(argv[i]);
            }
            else {
                if (strncmp(argv[i], "-", 1) == 0) {
                    cerr << "Invalid argument: \"" << argv[i] << "\"!" << endl;
                    help = true;
                }
                args.emplace_back(argv[i]);
            }
        }
    } catch (const std::exception &e) {
        cout << "Error: " << e.what() << endl;
        help = true;
    }

    if (args.size() > 1 || help) {
        cout << "Syntax: " << argv[0] << endl;
        cout << "Options:" << endl;
        cout << "   --in_abc <input>     Input file path to the meshroom alembic file" << endl;
        cout << "   --in_sfm <input>     Input file path to the meshroom camera sfm file" << endl;
        cout << "   --in_traj <input>    Input file path to the multiscan camera trajectory file" << endl;
        cout << "   --in_exr <input>     Input folder path that stores the exr format depth images" << endl;
        cout << "   --in_exr_abs <input> Input folder path that stores the absolute exr format depth images" << endl;
        cout << "   --in_mesh <input>    Input file path to the PLY/OBJ mesh file" << endl;
        cout << "   --in_srgb <input>    Input folder path to sRGB images" << endl;
        cout << "   --out_abc <output>   Output file path to the alembic file" << endl;
        cout << "   --out_sfm <output>   Output file path to the meshroom camera sfm file" << endl;
        cout << "   --out_mesh <output>  Output file path to the PLY/OBJ mesh file" << endl;
        cout << "   --out_srgb <output>  Output folder path that stores the sRGB colorspace images" << endl;
        cout << "   --out_exr <output>   Output folder path that stores the exr format depth images" << endl;
        cout << "   --step <count>       Camera skipping step size argument for reading camera trajectories" << endl;
        cout << "   -h, --help           Display this message" << endl;
        return -1;
    }

    try {
        sfmData::SfMData sfm_data;

        Converter converter;

        if (!in_sfm.empty())
            converter.importSFM(in_sfm);
        if (!in_trajectory.empty() && step > 0)
            converter.importCameras(in_trajectory, step);
        if (!in_mesh.empty())
            converter.importMesh(in_mesh);
        if (!in_srgb.empty())
            converter.importSRGB(in_srgb);
        if (!out_abc.empty())
            converter.exportABC(out_abc);
        if (!out_sfm.empty())
            converter.exportSFM(out_sfm);
        if (!out_exr.empty())
            converter.assignSensorDepth(in_exr, in_exr_abs, out_exr);
        if (!out_mesh.empty())
            converter.exportMesh(out_mesh);
        if (!out_srgb.empty())
            converter.linearizeSRGB(in_srgb, out_srgb);

        return 0;
    } catch (const std::exception &e) {
        cerr << "Caught runtime error : " << e.what() << endl;
        return -1;
    }
}

