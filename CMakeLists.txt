cmake_minimum_required (VERSION 3.5)

project (silber)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)  # Turn off compiler-specific extensions
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")

ADD_LIBRARY(silber-1.2 STATIC
    SharedMemoryReceiver.cpp
    SharedMemorySender.cpp
    ProcCommunicator.cpp
    ClientProcCommunicator.cpp
    ServerProcCommunicator.cpp
    SlotRegistry.cpp
    OSSemaphore.cpp
    SilberLogging.cpp)

# install library if it is needed by multiple targets
install(TARGETS silber-1.2
EXPORT silber-1.2-config
 DESTINATION lib)

 install(FILES 
    Message.h 
    SharedMemoryReceiver.h
    ServerProcCommunicator.h
    SharedMemorySender.h
    ClientProcCommunicator.h
    ProcCommunicator.h
    SharedMemoryReceiver.h DESTINATION include)

add_executable(silber_test tests.cpp)
target_link_libraries(silber_test silber-1.2)
