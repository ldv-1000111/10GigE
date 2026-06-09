cmake_minimum_required(VERSION 3.16)
project(SHR_Backend CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# ── Qt 6.8 ──────────────────────────────────────────────────
find_package(Qt6 6.2 REQUIRED COMPONENTS Core Network SerialPort)

# ── Vimba X ─────────────────────────────────────────────────
if(NOT DEFINED VIMBAX_DIR)
    set(VIMBAX_DIR "$ENV{VIMBAX_DIR}" CACHE PATH "Vimba X install dir")
endif()
if(NOT VIMBAX_DIR OR NOT EXISTS "${VIMBAX_DIR}")
    set(VIMBAX_DIR "$ENV{HOME}/VimbaX_2026-1" CACHE PATH "Vimba X install dir" FORCE)
endif()

message(STATUS "Vimba X dir: ${VIMBAX_DIR}")
set(VIMBAX_API "${VIMBAX_DIR}/api")

if(NOT EXISTS "${VIMBAX_API}/include/VmbCPP/VmbCPP.h")
    message(FATAL_ERROR
        "VmbCPP.h not found in ${VIMBAX_API}/include.\n"
        "Run scripts/00_install_vimba.sh and re-source ~/.bashrc")
endif()

include_directories(
    "${VIMBAX_API}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}"   # module headers: camera/, gnss/, etc.
)

set(VMB_CPP "${VIMBAX_API}/lib/libVmbCPP.so")
set(VMB_IMG "${VIMBAX_API}/lib/libVmbImageTransform.so")

# ── Sources ─────────────────────────────────────────────────
add_executable(SHR_Backend
    src/main.cpp
    camera/CameraWorker.cpp
    gnss/NmeaReader.cpp
    transform/ImageTransform.cpp
    storage/SidecarWriter.cpp
    trigger/TriggerServer.cpp
    server/BackendServer.cpp
)

target_link_libraries(SHR_Backend
    PRIVATE
        Qt6::Core
        Qt6::Network
        Qt6::SerialPort
        ${VMB_CPP}
        ${VMB_IMG}
        pthread
)

# ── Copy Vimba X libs and CTI files next to binary ──────────
add_custom_command(TARGET SHR_Backend POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${VMB_CPP} ${VMB_IMG}
        $<TARGET_FILE_DIR:SHR_Backend>
    COMMENT "Copying Vimba X shared libs"
)

# Copy Camera Simulator TL .cti
set(SIMULATOR_CTI "${VIMBAX_DIR}/cti/VimbaCameraSimulatorTL.cti")
if(EXISTS "${SIMULATOR_CTI}")
    add_custom_command(TARGET SHR_Backend POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SIMULATOR_CTI}"
            $<TARGET_FILE_DIR:SHR_Backend>
        COMMENT "Copying Camera Simulator TL"
    )
endif()

# Copy VmbC.xml (TL configuration)
set(VMBC_XML "${VIMBAX_DIR}/bin/VmbC.xml")
if(EXISTS "${VMBC_XML}")
    add_custom_command(TARGET SHR_Backend POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${VMBC_XML}"
            $<TARGET_FILE_DIR:SHR_Backend>
        COMMENT "Copying VmbC.xml"
    )
endif()

# Copy all .cti transport layer files
file(GLOB CTI_FILES "${VIMBAX_DIR}/cti/*.cti")
foreach(CTI_FILE ${CTI_FILES})
    add_custom_command(TARGET SHR_Backend POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CTI_FILE}"
            $<TARGET_FILE_DIR:SHR_Backend>
    )
endforeach()

# ── Install target (for deploy script) ──────────────────────
install(TARGETS SHR_Backend DESTINATION bin)
install(FILES ${VMB_CPP} ${VMB_IMG} DESTINATION lib)
if(EXISTS "${SIMULATOR_CTI}")
    install(FILES "${SIMULATOR_CTI}" DESTINATION lib)
endif()

# ── Print summary ───────────────────────────────────────────
message(STATUS "")
message(STATUS "  SHR_Backend configuration:")
message(STATUS "    Build type:   ${CMAKE_BUILD_TYPE}")
message(STATUS "    Qt6:          ${Qt6_VERSION}")
message(STATUS "    Vimba X:      ${VIMBAX_DIR}")
message(STATUS "    Binary:       ${CMAKE_BINARY_DIR}/SHR_Backend")
message(STATUS "")
