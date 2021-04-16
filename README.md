# arkit2meshroom

### ARKit data
```shell
<scanID>
----------------------------------------------------------------------
Data uploaded by scanner app

|-- <scanID>.json
    Meta data of uploaded scan, such as the resolution of RGB and depth stream, number of captured frames, etc.
|-- <scanID>.jsonl
    Camera informations of each frame, including timestamp, intrinsics, camera transform, euler angles and exposure duration
|-- <scanID>.depth.zlib
    Compressed depth stream with resolution 256x192, containing depth values of captured frames
|-- <scanID>.confidence.zlib
    Compressed confidence maps with resolution 256x192, containing confidence level of depth frames
|-- <scanID>.mp4
    RGB color stream, resolution can vary based on different modes in Scanner App
```

Inside the sample-data, color stream and depth stream are already decoded into image frames.  
The image frames are decoded with a **skip step size 10**, so every 1 of 10 frames is decoded.

```shell
Other data in sample-data
----------------------------------------------------------------------

|-- color.zip
    RGB images decoded from <scanID>.mp4
|-- depth.zip
    depth maps decoded from <scanID>.depth.zlib
|-- SensorDepth.zip
    depth maps as input for meshroom with sensor depth values
|-- meshroom_known.zip
    MehsroomCache generated when reconstruction with known camera poses
    meshroom_known/MeshroomCache/StructureFromMotion/cd578e8c71add1aafd50ba8df99a967782a30853 is generated during the first iteration from CameraInit to StructureFromMotion to get a sample cameras.sfm  
    meshroom_known/MeshroomCache/StructureFromMotion/0d5d4682705ef1916a63c9d08021adb18b2043db is generated during the second iteration from FeatureExtraction to Publish with known camera poses as input
    meshroom_known/MeshroomCache/Meshing/2489e4c53ce3f8af37efcf19dd1080c55a9990ec is the meshing result with known camera poses, no sensor depth  
    meshroom_known/MeshroomCache/Meshing/beb58a108403cb735060383c78148b96f2de5d5a is the meshing result with sensor depth and camera poses as inputs

|-- cameras_known.sfm
    Input camera data for reconstruction with known camera poses for FeatureExtraction node
```

### Convert ARKit Information to data required by Meshroom
Assign known camera poses stored in `scanID.jsonl` to **cameras.sfm** in the StructureFromMotion node.  (note the skip step size in sample-data is 10)  
More details are available [here](converter/README.md)

I made some modifications to the [meshroom/meshroom/core/desc.py](https://github.com/3dlg-hcvc/meshroom/blob/9769c9e48dc24afd71094c748f9464e6678d2c25/meshroom/core/desc.py#L358), and added a binary file for known camera poses reconstruction [meshroom/bin/meshroom_knownposes](https://github.com/3dlg-hcvc/meshroom/blob/dev/knownposes/bin/meshroom_knownposes)  
The modified branch is [here](https://github.com/3dlg-hcvc/meshroom/tree/dev/knownposes), it starts from FeactureExtraction node and ends at Publish node.




