# arkit2meshroom
Transfer ARKit information to meshroom reconstruction pipeline

###  Dependencies
* AliceVision  
Set the _LD_LIBRARY_PATH_ in **configure.sh** to `/path/to/alicevision/lib` and set the _DEPENDENCIES_DIR_ variable in **CMakeLists.txt** to `/path/to/dependencies`.  
My dependencies strucure:  
dependencies  
    - meshroom
        - AliceVision
        - build
        - install (include, lib, bin, etc)
    - rapidjson
* rapidjson [url](https://github.com/Tencent/rapidjson.git)
* Boost 1.70 
This is already installed when installing the AliceVision library. But some extra module is used in this codebase, so I reinstalled the Boost by
```
cd meshroom/build/boost
./bootstrap.sh
./b2 install --prefix=../install -j 8
```

### Build
```
mkdir build
cd build
cmake -DDEPENDENCIES_DIR=/path/to/dependencies ..
make -j 8
```

### Convert ARKit Information to data required by Meshroom
- assign known camera to poses cameras.sfm
`./run.sh`   
`--in_sfm /path/to/MeshroomCache/StructureFromMotion/uid/cameras.sfm`  
`--in_traj /path/to/arkit/scanID/scanID.jsonl`  
`--step frame_skip_step(int)`
`--out_sfm /path/to/MeshroomCache/StructureFromMotion/uid/cameras_knwon.sfm`

- assign sensor depth values to depth maps
`./run.sh`   
`--in_sfm /path/to/MeshroomCache/StructureFromMotion/uid/cameras.sfm`  
`--in_exr /path/to/MeshroomCache/DepthMap/uid`  
`--in_exr_abs /path/to/staging/scanID/depth`  
`--step frame_skip_step(int)`  
`--out_exr /path/to/MeshroomCache/SensorDepth`
