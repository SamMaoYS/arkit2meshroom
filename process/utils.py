import os
import sys
import shutil
import re
import json

import subprocess as subp
import traceback
import datetime
from timeit import default_timer as timer

class IO:
    INFO = 0
    WARN = 1
    ERR = 2
    
    @staticmethod
    def MULTISCAN_LOG(msg, log_type=INFO):
        msg_color = '\033[0m'
        if log_type == IO.WARN:
            warn_color = '\033[93m'
            sys.stdout.write(f'{warn_color}Warning: {msg} {msg_color}\n')
        elif log_type == IO.ERR:
            err_color = '\033[91m'
            sys.stdout.write(f'{err_color}Error: {msg} {msg_color}\n')
        else:
            sys.stdout.write(f'{msg_color}{msg}{msg_color}\n')
    
    @staticmethod
    def file_exist(file_path, ext=''):
        if not os.path.exists(file_path) or not os.path.isfile(file_path):
            return False
        elif ext in os.path.splitext(file_path)[1] or not ext:
            return True
        return False

    @staticmethod
    def folder_exist(folder_path):
        if not os.path.exists(folder_path) or os.path.isfile(folder_path):
            return False
        else:
            return True

    @staticmethod
    def make_clean_folder(path_folder):
        if not os.path.exists(path_folder):
            os.makedirs(path_folder)
        else:
            shutil.rmtree(path_folder)
            os.makedirs(path_folder)

    @staticmethod
    def ensure_dir_exists(path):
        try:
            if not os.path.isdir(path):
                os.makedirs(path)
        except OSError:
            if not os.path.isdir(path):
                raise

    @staticmethod
    def sorted_alphanum(file_list):
        """sort the file list by arrange the numbers in filenames in increasing order

        :param file_list: a file list
        :return: sorted file list
        """
        if len(file_list) <= 1:
            return file_list
        convert = lambda text: int(text) if text.isdigit() else text
        alphanum_key = lambda key: [convert(c) for c in re.split('([0-9]+)', key)]
        return sorted(file_list, key=alphanum_key)

    @staticmethod
    def get_file_list(path, ext=''):
        if not os.path.exists(path):
            raise OSError('Path {} not exist!'.format(path))

        file_list = []
        for filename in os.listdir(path):
            file_ext = os.path.splitext(filename)[1]
            if (ext in file_ext or not ext) and os.path.isfile(os.path.join(path, filename)):
                file_list.append(os.path.join(path, filename))
        file_list = IO.sorted_alphanum(file_list)
        return file_list

    @staticmethod
    def write_json(data, filename):
        if IO.folder_exist(os.path.dirname(filename)):
            with open(filename, "w+") as fp:  
                json.dump(data, fp, indent=4)
        if not IO.file_exist(filename):
            raise f'Cannot create file {filename}'

    @staticmethod
    def read_json(filename):
        if IO.file_exist(filename):
            with open(filename, "r") as fp:  
                data = json.load(fp)
            return data

def call(cmd, log, rundir='', env=None, desc=None, testMode=None):
    if not cmd:
        log.warning('No command given')
        return 0
    cwd = os.getcwd()
    res = -1
    try:
        start_time = timer()
        if rundir:
            os.chdir(rundir)
            log.info('Currently in ' + os.getcwd())
        log.info('Running ' + str(cmd))
        prog = subp.Popen(cmd, stdout=subp.PIPE, stderr=subp.PIPE, env=env)
        out, err = prog.communicate()
        if out:
            log.info(out)
        if err:
            log.error('Errors reported running ' + str(cmd))
            log.error(err)
        end_time = timer()
        delta_time = end_time - start_time
        desc_str = desc + ', ' if desc else ''
        desc_str = desc_str + 'cmd="' + str(cmd) + '"'
        log.info('Time=' + str(datetime.timedelta(seconds=delta_time)) + ' for ' + desc_str)
        res = prog.returncode
    except Exception as e:
        prog.kill()
        out, err = prog.communicate()
        log.error(traceback.format_exc())
    os.chdir(cwd)
    return res

