Pi Pico Rx Transceiver Experiments
----------------------------------

This branch contains an experimental build investigating the addition of
transmit capability to the Pi Pico RX receiver platform.

Some time ago I developed a standalone transmitter based on similar techniques
and hardware. While it appeared feasible to combine this transmitter with the
Pi Pico RX to create a simple transceiver, the resulting spurious performance
was not satisfactory. Although the concept worked, unwanted emissions and
spectral purity became significant concerns that needed to be addressed before
the approach could be considered practical.

The primary goal of these experiments is therefore to investigate methods of
improving spurious performance and to assess the overall viability of building
a simple, low-cost transceiver around the Pi Pico RX architecture.

A number of different approaches have been explored. Some experiments focus on
incremental improvements to the original design, while others revisit the
problem from first principles and investigate entirely different transmitter
architectures. Along the way I have also examined a range of more advanced
transmit features, including speech processing and Controlled Envelope Single
Sideband (CESSB), to better understand what level of performance can
realistically be achieved using the RP2040/RP2350 platform.

The results presented here are not intended as a finished design. Rather, they
document an ongoing engineering investigation into the capabilities and
limitations of software-defined transmit techniques on the Raspberry Pi Pico,
highlighting both successful approaches and ideas that proved less
effective than expected.


Caution
=======

This branch is an experimental research effort and should be viewed in the
spirit of scientific exploration rather than as a finished design.

The material presented here documents investigations, prototypes, measurements,
successes, and failures as I explore the possibility of adding transmit
capability to the Pi Pico RX platform. Many of the techniques and circuits
described are works in progress and may change substantially as the project
evolves.

No guarantees are made regarding performance, spectral purity, regulatory
compliance, or suitability for on-air operation. In particular, some
experimental transmitter configurations may produce unacceptable levels of
spurious emissions or other unwanted signals. Anyone wishing to reproduce
these experiments is responsible for verifying the performance of their own
equipment and ensuring compliance with applicable regulations before
transmitting.

This project should not be considered a construction guide, product, or
recommendation for practical use. Designs, software, hardware configurations,
and conclusions may change significantly as new ideas are tested and better
approaches are discovered.

Finally, while I am happy to share my findings, I am unable to provide ongoing
support for individual builds, hardware modifications, bug fixes,
troubleshooting, or feature requests. The information is published primarily as
a record of my own experiments and to encourage further investigation by
others with similar interests.

