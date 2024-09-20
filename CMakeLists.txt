cmake_minimum_required (VERSION 3.0)

project (silber)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)  # Turn off compiler-specific extensions

ADD_LIBRARY(silber-1.1 STATIC
    SharedMemoryReceiver.cpp
    SharedMemorySender.cpp
    ProcCommunicator.cpp
    ClientProcCommunicator.cpp
    ServerProcCommunicator.cpp)

# install library if it is needed by multiple targets
install(TARGETS silber-1.1
EXPORT silber-1.1-config
 DESTINATION lib)

 install(FILES 
    Message.h 
    SharedMemoryReceiver.h
    ServerProcCommunicator.h
    SharedMemorySender.h
    ClientProcCommunicator.h
    ProcCommunicator.h
    SharedMemoryReceiver.h DESTINATION include)

add_executable(silber_test main.cpp)
target_link_libraries(silber_test silber-1.1)
