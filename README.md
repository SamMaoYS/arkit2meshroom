# arkit2meshroom

### ARKit data
Each scanning data can be stored in the current directory in a folder named `staging`.
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

### Convert ARKit Information to data required by Meshroom
Assign known camera poses stored in `scanID.jsonl` to **cameras.sfm** in the StructureFromMotion node.  
More details are available [here](converter/README.md)

### Processing Scripts
More details are available [here](process/README.md)
- Decode ARKit streams to image frames
```
cd process
python scan_processor.py -i /path/to/staging/scanID --action convert -s skip_frame_step(int)
``` 

- Meshroom reconstruction with known camera poses
```
cd process
python scan_processor.py -i /path/to/staging/scanID --action convert --action photogrammetry --action knownposes -s skip_frame_step(int)
``` 

- Meshroom reconstruction with known camera poses + assign sensor depth values to depth maps
```
cd process
python scan_processor.py -i /path/to/staging/scanID --action convert --action photogrammetry --action knownposes --action sensordepth -s skip_frame_step(int)
``` 




