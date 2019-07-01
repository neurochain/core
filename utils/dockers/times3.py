#!/usr/bin/python3
import sys
import resource
import subprocess
import time
import os
import requests


def main():
    rusage_start = resource.getrusage(resource.RUSAGE_CHILDREN)
    time_start = time.time()
    child = subprocess.Popen(sys.argv[1:])
    child.communicate()
    time_end = time.time()
    rusage_end = resource.getrusage(resource.RUSAGE_CHILDREN)

    os_name = os.environ["CI_JOB_NAME"]
    commit = os.environ["CI_COMMIT_SHA"]
    branch = os.environ["CI_BUILD_REF_NAME"]
    build_type = os.environ["BUILD_TYPE"]

    request = (
        'qa,os="%s",branch="%s",build_type="%s" commit="%s",real=%f,user=%s,'
        "sys=%s,mem=%s,inb=%s,oub=%s,maj=%s,nivcsw=%s"
        % (
            os_name,
            branch,
            build_type,
            commit,
            (time_end - time_start),
            rusage_end.ru_utime - rusage_start.ru_utime,
            rusage_end.ru_stime - rusage_start.ru_stime,
            rusage_end.ru_maxrss,
            rusage_end.ru_inblock - rusage_start.ru_inblock,
            rusage_end.ru_oublock - rusage_start.ru_oublock,
            rusage_end.ru_majflt - rusage_start.ru_majflt,
            rusage_end.ru_nivcsw - rusage_start.ru_nivcsw,
        )
    )

    print(request)

    try:
        requests.post(
            "http://dashboard.neurochaintech.io:8086/write?db=qa",
            auth=("gitlab", os.environ["INFLUX_PUSH_PASSWORD"]),
            data=request,
        )
    except Exception:
        pass

    sys.exit(child.returncode)


if __name__ == "__main__":
    main()
