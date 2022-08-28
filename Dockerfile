# syntax=docker/dockerfile:1

FROM ubuntu:18.04

#RUN echo "deb http://ftp.us.debian.org/debian testing main contrib non-free" >> /etc/apt/sources.list
RUN apt-get update
RUN apt-get install -y libssl-dev 
#RUN apt-get install -y build-essential

ARG ROOT

WORKDIR /app

COPY ${ROOT}/lib/librtpad.so ${ROOT}/lib/libshm_client.so /app/

# grpc libs
COPY ${ROOT}/lib/libprotobuf.so.3.13.0.0 ${ROOT}/lib/libgrpc++.so.1 ${ROOT}/lib/libgrpc.so.12 ${ROOT}/lib/libgpr.so.12 ${ROOT}/lib/libupb.so.12 ${ROOT}/lib/libabsl_strings.so ${ROOT}/lib/libre2.so /app/
COPY ${ROOT}/lib/libaddress_sorting.so.12 ${ROOT}/lib/libabsl_status.so /app/
COPY ${ROOT}/lib/libshm_client.so ${ROOT}/lib/librtpad.so ${ROOT}/lib/libprotobuf.so.3.13.0.0 ${ROOT}/lib/libgrpc++.so.1 ${ROOT}/lib/libz.so.1 ${ROOT}/lib/libgrpc.so.12 ${ROOT}/lib/libgpr.so.12 ${ROOT}/lib/libupb.so.12 ${ROOT}/lib/libabsl_strings.so ${ROOT}/lib/libre2.so ${ROOT}/lib/libaddress_sorting.so.12 ${ROOT}/lib/libabsl_status.so ${ROOT}/lib/libabsl_str_format_internal.so ${ROOT}/lib/libabsl_cord.so ${ROOT}/lib/libabsl_bad_optional_access.so ${ROOT}/lib/libabsl_hashtablez_sampler.so ${ROOT}/lib/libabsl_time.so ${ROOT}/lib/libabsl_throw_delegate.so ${ROOT}/lib/libabsl_synchronization.so ${ROOT}/lib/libabsl_spinlock_wait.so ${ROOT}/lib/libabsl_raw_logging_internal.so ${ROOT}/lib/libabsl_strings_internal.so ${ROOT}/lib/libabsl_int128.so ${ROOT}/lib/libabsl_exponential_biased.so ${ROOT}/lib/libabsl_stacktrace.so ${ROOT}/lib/libabsl_time_zone.so ${ROOT}/lib/libabsl_base.so ${ROOT}/lib/libabsl_graphcycles_internal.so ${ROOT}/lib/libabsl_symbolize.so ${ROOT}/lib/libabsl_malloc_internal.so ${ROOT}/lib/libabsl_debugging_internal.so ${ROOT}/lib/libabsl_demangle_internal.so ${ROOT}/lib/libabsl_dynamic_annotations.so ${ROOT}/lib/libspdlog.so ${ROOT}/lib/libspdlog.so.1 ${ROOT}/lib/libspdlog.so.1.9.2 /app/

# configure .so paths
ENV LD_LIBRARY_PATH /app

COPY ${ROOT}/bin/shm_server /app/
CMD /app/shm_server
