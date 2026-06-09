.. SHR 10GigE Complete Solution Documentation
   ============================================

SHR 10GigE + Bedrock V3000 — Complete Solution
================================================

**Author: Luis Viveros · June 2026**

Ultra-High-Resolution Machine Vision with Geolocation on Linux

----

.. toctree::
   :maxdepth: 1
   :caption: Introduction

   intro

.. toctree::
   :maxdepth: 2
   :caption: Part I — The SHR Camera Series

   series_intro
   series_models
   series_models_101mp
   series_models_127mp
   series_models_151mp
   series_models_245mp
   series_interfaces
   series_applications

.. toctree::
   :maxdepth: 2
   :caption: Part II — Host Platform: Bedrock V3000

   platform_intro
   platform_streaming
   platform_v3000_specs
   platform_vs_r8000
   platform_qt_role

.. toctree::
   :maxdepth: 2
   :caption: Part III — System & SDK Setup

   hardware_setup
   system_settings
   sdk_installation
   dev_x86_setup
   laptop_setup
   nuc_setup
   project_setup

.. toctree::
   :maxdepth: 2
   :caption: Part IV — Application Architecture

   arch_overview
   arch_usb_network
   arch_backend_server

.. toctree::
   :maxdepth: 2
   :caption: Part IV-A — V3000 Backend (C++)

   camera_discovery
   feature_configuration
   async_acquisition
   image_transformation
   saving_frames
   trigger_overview
   trigger_action_commands
   trigger_hardware_ttl
   trigger_software_qt

.. toctree::
   :maxdepth: 2
   :caption: Part IV-B — Android Frontend (Qt QML)

   android_setup
   android_backend_client
   android_qml_ui
   qt_ui_layout
   qt_stylesheet

.. toctree::
   :maxdepth: 2
   :caption: Part V — Complete Application

   full_application
   building_and_running

.. toctree::
   :maxdepth: 2
   :caption: Part VI — GNSS/NMEA Metadata Injection

   gnss_intro
   gnss_nmea_primer
   gnss_rs232_v3000
   gnss_architecture
   gnss_serial_reader
   gnss_frame_tagging
   gnss_sidecar_file
   gnss_qt_ui
   gnss_full_implementation

.. toctree::
   :maxdepth: 1
   :caption: Reference

   troubleshooting
   api_reference
   links
   git_rtd
