SDK Installation
================

Vimba X is Allied Vision's current-generation SDK. It is fully GenICam-compliant
and supports SHR cameras (via the SVS-Vistek GigE transport layer) out of the box.

.. warning::
   Do **not** use the legacy Vimba SDK for new projects. Vimba X is the
   required SDK for all current development.

Download
--------

Download **Vimba X 2026-1** from:

https://www.alliedvision.com/en/support/software-downloads/vimba-x-sdk/vimba-x

Online developer documentation is also available at:

https://docs.alliedvision.com/Vimba_X/

Select the Linux x86_64 package (``VimbaX_2026-1_Linux_x86_64.tar.gz``).

Install
-------

.. code-block:: bash

   # Extract the archive
   tar -xzf VimbaX_2026-1_Linux_x86_64.tar.gz -C ~/

   # The SDK is now at ~/VimbaX_2026-1/
   ls ~/VimbaX_2026-1/

Directory structure::

   VimbaX_2026-1/
   ├── api/
   │   ├── include/         ← Headers: VmbCPP/, VmbC/, VmbImageTransform/
   │   ├── lib/x86_64/      ← Shared libraries: libVmbCPP.so, libVmbImageTransform.so
   │   └── examples/        ← Example projects (VmbC/, VmbCPP/)
   ├── bin/
   │   ├── VmbC.xml         ← Transport layer configuration
   │   └── VimbaX_Viewer    ← GUI viewer for testing cameras
   └── cti/
       ├── VimbaGigETL.cti          ← GigE Vision transport layer
       ├── VimbaGigETL_Install.sh   ← GigE TL installer
       └── VimbaUSBTL_Install.sh    ← USB TL installer (not needed here)

Install the GigE Transport Layer
----------------------------------

The transport layer (TL) is a kernel-level driver that handles the GigE Vision
protocol. It must be installed before any camera will be detected.

.. code-block:: bash

   cd ~/VimbaX_2026-1/cti/
   sudo ./VimbaGigETL_Install.sh

This registers the GigE TL with the system and sets the ``GENICAM_GENTL64_PATH``
environment variable.

Verify installation:

.. code-block:: bash

   echo $GENICAM_GENTL64_PATH
   # Should print the path to the cti directory

Verify Camera Detection with Vimba X Viewer
--------------------------------------------

Before writing any code, confirm the camera is detected:

.. code-block:: bash

   cd ~/VimbaX_2026-1/bin/
   ./VimbaX_Viewer

If the SHR camera appears in the camera list, your hardware and transport
layer are working correctly. If it does not appear, revisit
:doc:`hardware_setup` and :doc:`system_settings`.

.. tip::
   In Vimba X Viewer you can explore all camera features, set the pixel format,
   and trigger test captures before writing any code. This is the fastest way
   to validate the hardware setup.

Environment Variables
----------------------

Add the Vimba X libraries to your library path so your application can find
them at runtime:

.. code-block:: bash

   echo 'export LD_LIBRARY_PATH=~/VimbaX_2026-1/api/lib/x86_64:$LD_LIBRARY_PATH' >> ~/.bashrc
   source ~/.bashrc
