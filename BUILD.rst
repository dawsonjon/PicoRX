Pi Pico Rx - Build Instructions
"""""""""""""""""""""""""""""""

Getting the Code
----------------

.. code::

  sudo apt install git
  git clone https://github.com/dawsonjon/PicoRX.git
  cd PicoRX
  git submodule init
  git submodule update

Install Build Dependencies
--------------------------

Install the following packages using your distributions package manager:

- `cmake`
- `gcc-arm-none-eabi`

For Ubuntu and other Debian based Linux distributions, the following command
line can be used to install all dependencies at once.

.. code::

  sudo apt install git cmake gcc-arm-none-eabi

Install Pi Pico SDK (Optional)
------------------------------

This step is now optional and only necessary if offline (no internet) builds
are desired. If this step is skipped, the configure step of the CMake build will
pull down the necessary Pico SDK dependencies.

Follow the `Getting started with the Raspberry Pi Pico <https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf>`_ quick start guide to install the C/C++ SDK.

.. code::

  sudo apt install wget #if wget not installed
  wget https://raw.githubusercontent.com/raspberrypi/pico-setup/master/pico_setup.sh
  chmod +x pico_setup.sh
  ./pico_setup.sh

Build Project
-------------

Build the project using these cmake commands.

.. code::

  mkdir build
  cd build
  cmake -DPICO_BOARD=pico ..
  make

**NOTE**: When using the installed SDK described in the previous section, ensure
the ``PICO_SDK_PATH`` environment variable exists on your path by running:
``echo ${PICO_SDK_PATH}``. If the output is empty, refresh the environment by
either starting a new shell or by running: ``source ~/.bashrc``.