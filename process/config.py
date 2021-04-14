import os
import utils

DATA_DIR = '../staging/'
# Output directory of RGB images
COLOR_FOLDER = 'color'
# Output directory of depth maps
DEPTH_FOLDER = 'depth'
# Output directory of meshroom results
PHOTOGRAMMETRY_RESULT_DIR = 'meshroom'

# System specific paths for processing server binaries
TOOLS_DIR = '../'
SCRIPTS_DIR = './'
DEPENDENCIES_DIR = 'dependencies'
PHOTOGRAMMETRY_DIR = 'meshroom'
CONVERTER_DIR = 'converter'

SKIP_STEP = 10
MAX_CPUS = 8
MAX_GPUS = 1
