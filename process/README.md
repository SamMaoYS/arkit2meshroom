## Mehsroom reconstruction scripts

### Envrionment
```
pip install -r requirements.txt
```
Meshroom python environment is installed via anaconda virtual environment, with envrionment name as `meshroom`, python version 3.7.
In `scan_process.py` I run meshroom binaries by using `conda run -n meshroom ./xxx.sh ...`.

I made some modifications to the [meshroom/meshroom/core/desc.py](https://github.com/3dlg-hcvc/meshroom/blob/9769c9e48dc24afd71094c748f9464e6678d2c25/meshroom/core/desc.py#L358), and added a binary file for known camera poses reconstruction [meshroom/bin/meshroom_knownposes](https://github.com/3dlg-hcvc/meshroom/blob/dev/knownposes/bin/meshroom_knownposes)  
The modified branch is [here](https://github.com/3dlg-hcvc/meshroom/tree/dev/knownposes)

### Configuration
Specify the output directory, directory of libraries, arguments in the **config.py**.

### Decode ARKit streams to image frames
```
cd process
python scan_processor.py -i /path/to/staging/scanID --action convert -s skip_frame_step(int)
``` 

### Meshroom reconstruction with known camera poses
```
cd process
python scan_processor.py -i /path/to/staging/scanID --action convert --action photogrammetry --action knownposes -s skip_frame_step(int)
``` 

### Meshroom reconstruction with known camera poses + assign sensor depth values to depth maps
```
cd process
python scan_processor.py -i /path/to/staging/scanID --action convert --action photogrammetry --action knownposes --action sensordepth -s skip_frame_step(int)

