SDK Installation — Vimba X 2026-1
===================================

Download
---------

Go to the Allied Vision download page (free account required):

https://www.alliedvision.com/en/support/software-downloads/

Download **Vimba X 2026-1** for Linux x86_64. The archive is named:

.. code-block:: text

   VimbaX_Setup-2026-1-Linux64.tar.gz

Extract
--------

.. code-block:: bash

   cd ~
   tar -xf ~/Downloads/VimbaX_Setup-2026-1-Linux64.tar.gz

This creates ``~/VimbaX_2026-1/``. The structure relevant to C++ development:

.. code-block:: text

   ~/VimbaX_2026-1/
   │
   ├── api/
   │   ├── include/
   │   │   ├── VmbCPP/          ← C++ API headers  (VmbCPP.h, Camera.h, Frame.h ...)
   │   │   ├── VmbC/            ← C API headers
   │   │   └── VmbImageTransform/  ← Image Transform headers (VmbTransform.h)
   │   │
   │   ├── lib/
   │   │   ├── libVmbCPP.so        ← C++ API library       (link in CMake)
   │   │   ├── libVmbC.so          ← C API library
   │   │   ├── libVmbImageTransform.so  ← Image Transform library
   │   │   ├── VmbC.xml            ← Transport layer configuration
   │   │   └── GenICam/            ← GenICam runtime (loaded automatically)
   │   │
   │   ├── examples/
   │   │   └── VmbCPP/          ← C++ examples (AsynchronousGrab, ChunkAccess ...)
   │   │
   │   └── source/VmbCPP/       ← C++ API source (rebuild for newer compilers)
   │
   ├── bin/
   │   └── VimbaXViewer         ← GUI camera viewer and feature explorer
   │
   ├── cti/
   │   ├── Install_GenTL_Path.sh    ← run this to register transport layers
   │   ├── Uninstall_GenTL_Path.sh
   │   ├── VimbaGigETL.cti          ← GigE Vision transport layer
   │   ├── VimbaUSBTL.cti           ← USB3 Vision transport layer
   │   └── VimbaCameraSimulatorTL.cti  ← Camera Simulator (no hardware needed)
   │
   └── doc/
       └── VimbaX_Documentation/
           ├── cppAPIManual.html    ← C++ API reference
           ├── imagetransformManual.html
           └── sdkManual.html

Install the GenTL Transport Layer
-----------------------------------

This registers the transport layers with the system so cameras
can be discovered:

.. code-block:: bash

   cd ~/VimbaX_2026-1/cti/
   sudo ./Install_GenTL_Path.sh

Reboot
-------

.. code-block:: bash

   sudo reboot

The reboot is required — the GenTL registration does not take
effect until after a restart.

Set Environment Variables
--------------------------

After rebooting, add to ``~/.bashrc``:

.. code-block:: bash

   # Vimba X 2026-1
   export VIMBAX_DIR=~/VimbaX_2026-1
   export LD_LIBRARY_PATH=${VIMBAX_DIR}/api/lib:${LD_LIBRARY_PATH:-}
   export GENICAM_GENTL64_PATH=${VIMBAX_DIR}/cti

Activate:

.. code-block:: bash

   source ~/.bashrc

Verify Installation
--------------------

Check the key files are present:

.. code-block:: bash

   ls $VIMBAX_DIR/api/include/VmbCPP/VmbCPP.h
   ls $VIMBAX_DIR/api/lib/libVmbCPP.so
   ls $VIMBAX_DIR/api/lib/libVmbImageTransform.so
   ls $VIMBAX_DIR/cti/VimbaCameraSimulatorTL.cti

Launch the Viewer to confirm the Camera Simulator is working:

.. code-block:: bash

   $VIMBAX_DIR/bin/VimbaXViewer

A simulated camera should appear in the camera list on the left.
If it does not appear, verify ``GENICAM_GENTL64_PATH`` is set
correctly and contains ``VimbaCameraSimulatorTL.cti``.

.. note::
   The Vimba X Viewer is named ``VimbaXViewer`` (no space, no
   underscore) in the actual installation — not ``VimbaX_Viewer``
   as shown in some Allied Vision documentation.

What We Use for C++
--------------------

For building ``SHR_Backend``, CMakeLists.txt references:

.. list-table::
   :header-rows: 1
   :widths: 45 55

   * - File
     - Purpose
   * - ``api/include/VmbCPP/VmbCPP.h``
     - Main C++ API header
   * - ``api/include/VmbImageTransform/VmbTransform.h``
     - Image Transform header
   * - ``api/lib/libVmbCPP.so``
     - C++ API shared library
   * - ``api/lib/libVmbImageTransform.so``
     - Image Transform shared library
   * - ``api/lib/VmbC.xml``
     - Transport layer config (deployed with binary)
   * - ``cti/VimbaCameraSimulatorTL.cti``
     - Camera Simulator (deployed with binary)
   * - ``cti/VimbaGigETL.cti``
     - GigE Vision TL (deployed with binary for real cameras)

The ``doc/`` folder contains the full offline HTML documentation —
open ``doc/VimbaX_Documentation/index.html`` in a browser for the
complete API reference.
