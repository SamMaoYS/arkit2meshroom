#!/usr/bin/env python
#
# Process scans from ipad and runs recons pipeline
# Run with ./scan_processor.py (or python scan_processor.py on Windows)

import argparse
import os
import logging

import utils
import traceback
from glob import glob
import config as cfg
import json

from utils import IO

PROCESSES = ['convert', 'photogrammetry', 'knownposes', 'sensordepth']

# ffmpeg to call for converting iPad raw files to image files
SCRIPTS_DIR = cfg.SCRIPTS_DIR
SCRIPTS_BIN = os.path.join(SCRIPTS_DIR, '')
# dependencies directory
DEPENDENCIES_DIR = os.path.join(cfg.TOOLS_DIR, cfg.DEPENDENCIES_DIR)
# module to call for photogrammetry reconstruction
PHOTOGRAMMETRY_DIR = os.path.join(DEPENDENCIES_DIR, cfg.PHOTOGRAMMETRY_DIR)
PHOTOGRAMMETRY_BIN = os.path.join(PHOTOGRAMMETRY_DIR, '')

CONVERTER_DIR = os.path.join(cfg.TOOLS_DIR, cfg.CONVERTER_DIR)
CONVERTER_DIR = os.path.join(CONVERTER_DIR, '')

FORMAT = '%(asctime)-15s [%(levelname)s] %(message)s'
formatter = logging.Formatter(FORMAT)
logging.basicConfig(format=FORMAT)
log = logging.getLogger('scan_processor')
log.setLevel(logging.INFO)


def decode_color(color_stream, color_out, desc, skip=1):
    try:
        ret = 0
        if os.path.isfile(color_stream):
            IO.make_clean_folder(color_out)
            ret = utils.call(['ffmpeg', '-i', color_stream, '-vf', f'select=not(mod(n\\,{skip}))',
                             '-vsync', 'vfr', '-start_number', '0',
                             os.path.join(color_out, '%d.png')], log, desc=desc)

        log.info(f'Decoding color stream is ended, return code {ret}')
        return ret
    except Exception as e:
        log.error(traceback.format_exc())
        raise e


def decode_depth(depth_stream, confidence_stream, depth_out, desc, skip=1):
    ret = 0
    try:
        if os.path.isfile(depth_stream):
            IO.make_clean_folder(depth_out)
            env = os.environ.copy()
            env['PYTHONPATH'] = ":".join(SCRIPTS_DIR)
            if os.path.isfile(confidence_stream):
                ret = utils.call(['python', 'depth2png.py', '-in', depth_stream, '-in_confi', confidence_stream, '-W', '256', '-H', '192', '-S', str(skip),
                                '-L', '1', '--filter', '-o', depth_out], log, SCRIPTS_DIR, env=env, desc=desc)
            else:
                ret = utils.call(['python', 'depth2png.py', '-in', depth_stream, '-W', '256', '-H', '192', '-S', str(skip),
                             '-o', depth_out], log, SCRIPTS_DIR, env=env, desc=desc)
        log.info(f'Decoding depth stream is ended, return code {ret}')
        return ret
    except Exception as e:
        log.error(traceback.format_exc())
        raise e

def get_camera_sfm_path(graph_file):
    if not os.path.isfile(graph_file):
        return None
    with open(graph_file, 'r') as f:
        data = json.load(f)
    if not data.get('graph', None) or not data['graph'].get('StructureFromMotion_1', None):
        return None
    uid0 = data['graph']['StructureFromMotion_1']['uids']['0']
    return os.path.join('StructureFromMotion', f'{uid0}', 'cameras.sfm')

def get_depthmaps_dir(graph_file):
    if not os.path.isfile(graph_file):
        return None
    with open(graph_file, 'r') as f:
        data = json.load(f)
    if not data.get('graph', None) or not data['graph'].get('DepthMap_1', None):
        return None
    uid0 = data['graph']['DepthMap_1']['uids']['0']
    return os.path.join('DepthMap', f'{uid0}')

def update_config(config):
    if config.get('all'):
        for name in PROCESSES:
            config[name] = True
    else: 
        if config.get('from'):
            fromSeen = False
            for name in PROCESSES:
                fromSeen = fromSeen or name == config['from']
                config[name] = fromSeen
        if config.get('actions'):
            actions = json.loads(config['actions'])
            for action in actions:
                if action in PROCESSES:
                    config[action] = True
    
    return config


def process_scan_dir(path, name, config):
    # Check if valid scan dir
    namebase = os.path.join(path, name)
    validNovh = os.path.isfile(namebase + ".mp4")
    if not validNovh:
        msg = 'path %s not a valid scan dir' % path
        log.info(msg)
        return msg

    # Wrapper around process_scan_dir_basic but with logging to file
    fh = logging.FileHandler(os.path.join(path, 'process.log'))
    fh.setLevel(logging.INFO)
    fh.setFormatter(formatter)
    log.addHandler(fh)
    msg = ''
    try:
        msg = process_scan_dir_basic(path, name, config)
        log.info(msg)
    finally:
        log.removeHandler(fh)
        fh.close()
    return msg


def process_scan_dir_basic(path, name, config):
    # Convert to sens file
    path = os.path.abspath(path)
    filename = os.path.basename(path)
    outbase = os.path.join(cfg.DATA_DIR, filename)
    outbase = os.path.abspath(outbase)
    meta_file = os.path.join(path, filename + '.json')
    color_stream = os.path.join(path, filename + '.mp4')
    depth_stream = os.path.join(path, filename + '.depth.zlib')
    confidence_stream = os.path.join(path, filename + '.confidence.zlib')
    camera_file = os.path.join(path, filename + '.jsonl')
    
    color_dir = os.path.join(outbase, cfg.COLOR_FOLDER)
    depth_dir = os.path.join(outbase, cfg.DEPTH_FOLDER)
    meshroom_result_dir = os.path.join(outbase, cfg.PHOTOGRAMMETRY_RESULT_DIR)
    IO.ensure_dir_exists(outbase)

    color_exists = os.path.exists(color_dir)
    depth_exists = os.path.exists(depth_dir)
    with open(meta_file, 'r') as f:
        meta = json.load(f)

    # TODO: not utilsize return value yet
    ret = 0
    skip_frames = cfg.SKIP_STEP
    max_cpus = cfg.MAX_CPUS
    max_gpus = cfg.MAX_GPUS
    # Decode video to frames
    if config.get('convert'):
        if config.get('overwrite') or (not color_exists or not depth_exists):
            ret = decode_color(color_stream, color_dir, 'convert color', skip_frames)
            if ret != 0:
                return 'Scan at %s aborted: decode color stream failed' % path
            ret = decode_depth(depth_stream, confidence_stream, depth_dir, 'convert depth', skip_frames)
            if ret != 0:
                return 'Scan at %s aborted: decode depth stream failed' % path

            color_frames = IO.get_file_list(color_dir, '.png')
            depth_frames = IO.get_file_list(depth_dir, '.exr')
            if len(color_frames) == 0:
                return 'Scan at %s aborted: no decoded color images (convert failed)' % path
            if len(depth_frames) == 0:
                return 'Scan at %s aborted: no decoded depth images (convert failed)' % path
            if len(color_frames) != len(depth_frames):
                return 'Scan at %s aborted: the number of decoded depth images does not match color ' \
                    'images (convert failed)' % path
        else:
            log.info('skipping convert')

    # Meshroom Reconstruction
    if config.get('photogrammetry'):
        color_frames = IO.get_file_list(color_dir, '.png')
        if len(color_frames) == 0:
            return 'Scan at %s aborted: no decoded color images (convert failed)' % path

        log.info(f'Start processing using meshroom photogrammetry')
        IO.make_clean_folder(meshroom_result_dir)
        env = os.environ.copy()
        env['PYTHONPATH'] = ":".join(PHOTOGRAMMETRY_BIN)
        if not config.get('knownposes'):
            ret = utils.call(['conda', 'run', '-n', 'meshroom','./meshroom_batch.sh', '--input', color_dir, 
                '--output', os.path.join(meshroom_result_dir, 'result'), 
                '--cache', os.path.join(meshroom_result_dir, 'MeshroomCache'), 
                '--save', os.path.join(meshroom_result_dir, 'graph.mg'), 
                '--paramOverrides', f"FeatureExtraction:maxThreads={max_cpus}", f"DepthMap:nbGPUs={max_gpus}",
                '--toNode', 'Publish', '--forceCompute'], log, PHOTOGRAMMETRY_BIN, env=env)
        else:
            ret = utils.call(['conda', 'run', '-n', 'meshroom','./meshroom_batch.sh', '--input', color_dir, 
                '--output', os.path.join(meshroom_result_dir, 'result'), 
                '--cache', os.path.join(meshroom_result_dir, 'MeshroomCache'), 
                '--save', os.path.join(meshroom_result_dir, 'graph.mg'), 
                '--paramOverrides', f"FeatureExtraction:maxThreads={max_cpus}", f"DepthMap:nbGPUs={max_gpus}",
                '--toNode', 'StructureFromMotion', '--forceCompute'], log, PHOTOGRAMMETRY_BIN, env=env)

            sfm_path = get_camera_sfm_path(os.path.join(meshroom_result_dir, 'graph.mg'))
            sfm_path = os.path.join(meshroom_result_dir, 'MeshroomCache', sfm_path)
            if not sfm_path or not os.path.isfile(sfm_path):
                return 'Scan at %s aborted: no camera sfm file (photogrammetry failed)' % path
            new_sfm_path = os.path.splitext(sfm_path)[0] + '_known' + os.path.splitext(sfm_path)[1]

            # link knwon poses to meshroom camera sfm file
            ret = utils.call(['./run.sh', '--in_sfm', sfm_path, '--in_traj', camera_file, 
                '--out_sfm', new_sfm_path, '--step', str(skip_frames)], log, CONVERTER_DIR)
            
            ret = utils.call(['conda', 'run', '-n', 'meshroom','./meshroom_knownposes.sh', 
                '-g', os.path.join(meshroom_result_dir, 'graph.mg'), 
                '-sfm', new_sfm_path,
                '--output', os.path.join(meshroom_result_dir, 'result'), 
                '--cache', os.path.join(meshroom_result_dir, 'MeshroomCache'), 
                '--save', os.path.join(meshroom_result_dir, 'graph_known.mg'), 
                '--paramOverrides', "StructureFromMotion:lockScenePreviouslyReconstructed=true", "StructureFromMotion:lockAllIntrinsics=true",
                    f"FeatureExtraction:maxThreads={max_cpus}", f"DepthMap:nbGPUs={max_gpus}",
                '--toNode', 'Publish', '--forceCompute'], log, PHOTOGRAMMETRY_BIN, env=env)

            if config.get('sensordepth'):
                sfm_path = get_camera_sfm_path(os.path.join(meshroom_result_dir, 'graph_known.mg'))
                sfm_path = os.path.join(meshroom_result_dir, 'MeshroomCache', sfm_path)
                if not sfm_path or not os.path.isfile(sfm_path):
                    return 'Scan at %s aborted: no camera sfm file (photogrammetry failed)' % path
                
                sensor_depth_dir = os.path.join(meshroom_result_dir, 'MeshroomCache', 'SensorDepth')

                valid_depth = get_depthmaps_dir(os.path.join(meshroom_result_dir, 'graph_known.mg'))
                valid_depth = os.path.join(meshroom_result_dir, 'MeshroomCache', valid_depth)

                # Make sensor depth as meshroom depth maps
                ret = utils.call(['./run.sh', '--in_sfm', sfm_path, '--in_exr', valid_depth, 
                    '--in_exr_abs', depth_dir, '--step', str(skip_frames), '--out_exr', sensor_depth_dir], log, CONVERTER_DIR)

    return 'Scan at %s processed' % path

def main():
    # Argument processing
    parser = argparse.ArgumentParser(description='Process scans!!!')
    parser.add_argument('-i', '--input', dest='input', action='store',
                        required=True,
                        help='Input directory or list of directories (if file)')
    parser.add_argument('--from', dest='from', action='store',
                        default='convert',
                        choices=PROCESSES,
                        help='Which command to start from')
    parser.add_argument('--action', dest='actions', action='append',
                        choices=PROCESSES,
                        help='What actions to do')
    parser.add_argument('-s', '--step', dest='step', type=int, action='store',
                        required=False,
                        help='Skip every step size of frames when decoding streams')
    parser.add_argument('--meshroom_dir', dest='meshroom_dir', type=str, action='store',
                        required=False,
                        help='Meshroom output directory')
    parser.add_argument('--cpus', dest='cpus', type=int, action='store',
                        required=False,
                        help='Specify the maximum number of cpus for meshroom reconstruction')
    parser.add_argument('--gpus', dest='gpus', type=int, action='store',
                        required=False,
                        help='Specify the maximum number of gpus for meshroom reconstruction')
    parser.add_argument('--overwrite', dest='overwrite', action='store_true', default=False,
                        help='Overwrite existing files')
    parser.add_argument('--novh', dest='novh', action='store_true', default=False,
                        help='Remove _vh suffixes')

    args = parser.parse_args()
    config = {}
    if args.overwrite:
        config['overwrite'] = True
    if args.novh:
        config['novh'] = True
    if args.actions:
        for action in args.actions:
            config[action] = True
    else:
        config = update_config(vars(args))

    if args.step:
        cfg.SKIP_STEP = args.step
        print(f'skip step is changed to {args.step}')

    if args.step:
        cfg.MAX_CPUS = args.cpus
        print(f'maximum number of cpus is changed to {args.cpus}')

    if args.step:
        cfg.MAX_GPUS = args.gpus
        print(f'maximum number of gpus is changed to {args.gpus}')

    if args.meshroom_dir:
        cfg.PHOTOGRAMMETRY_RESULT_DIR = args.meshroom_dir
        print(f'meshroom output directory is changed to {args.meshroom_dir}')

    if os.path.isdir(args.input):
        name = os.path.relpath(args.input, args.input + '/..')
        process_scan_dir(args.input, name, config)
    else:
        print('Please specify directory or file as input')


if __name__ == "__main__":
    main()
