# Get the base Ubuntu image
FROM ubuntu:23.04

ENV TZ=Europe/Kiev
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# Update apps on the base image
RUN apt -y update && apt  install -y

# Install the Clang compiler
RUN apt -y install git g++ cmake
