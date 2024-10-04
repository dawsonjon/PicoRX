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

Install Pi Pico SDK
-------------------

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
  cmake -DPICO_BOARD=pico -DPICO_SDK_PATH=~/pico/pico-sdk ..
  make
