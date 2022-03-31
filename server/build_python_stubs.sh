#!/bin/bash

proto_path=`dirname $0`
gen_path="${proto_path}/../generated"
env_name="grpc"

found_env="n"
for environment in `conda env list | awk '{print $1}'`; do
    if [[ "${env_name}" == "${environment}" ]]; then
        found_env="y"
        break
    fi
done

if [[ "${found_env}" == "n" ]]; then
    pushd ${proto_path}
    conda env update -f environment.yml
    popd
fi

mkdir -p ${gen_path}
conda run -n grpc python -m grpc.tools.protoc -I${proto_path} --python_out=${gen_path} --grpc_python_out=${gen_path} shm_server.proto
