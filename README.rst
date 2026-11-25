PCB Arts VPX Firmware
=====================

Firmware for the PCB Arts VPX vapor phase soldering oven.

======================  =========  =========================================
Project                 MCU        Purpose
======================  =========  =========================================
``core-bootloader``     STM32F302  Bootloader of the core (controller) board
``core-fw``             STM32F302  Core firmware (oven control)
``display-bootloader``  STM32U5A9  Bootloader, flashes signed update files
``display-fw``          STM32U5A9  TouchGFX user interface
======================  =========  =========================================

Requirements
------------

The projects can be built either via make or directly in STM32CubeIDE.
Both produce the same build.
Either way, Python 3 with ``tools/requirements.txt``
(``pip install -r tools/requirements.txt``)
is needed for the keys and signing (see `Signing`_) and for plotting reflow logs.

Via make
   - STM32CubeIDE (its headless builder is used)
   - STM32CubeProgrammer (``make flash``)
   - STM32CubeMX (``make generate`` only)
   - ``arm-none-eabi`` binutils (``make hex``, ``flash`` and ``size`` only)

   Python is run via ``uv run python`` by default.
   Use ``PYTHON=python3`` for a plain Python installation.
   The ST tool paths are pinned in ``tools/common.mk``
   and can be overridden with ``CUBEIDE=``, ``PROGRAMMER=`` and ``CUBEMX=``
   (see ``make -C <project> help``).

Via STM32CubeIDE
   - STM32CubeIDE (includes the toolchain and CubeMX code generation)
   - STM32CubeProgrammer (only for flashing via ``STM32_Programmer_CLI``)

Building
--------

``display-bootloader`` needs the public key of the signing key compiled in,
so create it once before the first build (see `Signing`_).

Via make
   ``make`` builds all projects, ``make <project>`` a single one
   (``BUILD_CONFIG=Debug|Release``).
   This runs CubeIDE's headless builder, i.e. the same build as in the IDE.

Via STM32CubeIDE
   Build the projects as usual in the IDE.

Flashing
--------

All projects are flashed over SWD.

Via make
   ``make -C <project> flash`` builds and programs the project,
   for ``display-fw`` including the external OSPI flash.

Via STM32CubeIDE
   Create a run/debug configuration (STM32 C/C++ Application).
   For ``display-fw`` add the external loader
   ``display-fw/MX25LM51245G_STM32U599J-DK.stldr`` for its OSPI content.

Via STM32_Programmer_CLI
   .. code-block:: sh

      STM32_Programmer_CLI -c port=SWD -d <project>/Debug/<project>.elf -hardRst

   For ``display-fw`` add ``-el display-fw/MX25LM51245G_STM32U599J-DK.stldr``.

Signing
-------

Updates are installed by ``display-bootloader`` from signed update files
on an SD card.
Keys, images and signing are handled by ``tools/imgtool.py``.

Via make
   .. code-block:: sh

      make privkey   # create the signing key keys/key.pem (once)
      make pubkey    # write public key for display-bootloader to
                     # keys/include/fw_update_sig_pk.c_inc (rebuild to apply)
      make sign      # build + sign update files -> out/<config>/signed/

   ``make hash`` creates update files with a zero signature instead,
   accepted only with the third party jumper on the core board.

Via Python
   The same commands without make, e.g. on Windows after building in CubeIDE:
   ``python tools/imgtool.py privkey``, ``pubkey`` and
   ``sign --config Debug|Release``.
   ``python tools/imgtool.py --help`` lists all commands and options.
