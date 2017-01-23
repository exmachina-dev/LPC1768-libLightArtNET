Libartnet
=========

Libartnet is an implementation of the ArtNet protocol. ArtNet allows the
transmission of DMX and related data over IP networks.

This is a port of libartnet on mbed platform. For the original version, see (libartnet)[https://github.com/OpenLightingProject/libartnet]

# Requirements

For this lib to work, you need broadcast features in socket. This feature is not yet merged so you need to use the our [mbed-os](https://github.com/exmachina-dev/mbed-os) repository.


# Installation

With mbed-cli, clone the repository into the `lib/` directory or use submodules:

```sh
$ git submodule add lib/libartnet https://github.com/exmachina-dev/libartnet
$ git submodule init
$ git submodule update
```

# Configuration

Some features are disabled by default to save space on microcontroller. Those features are:
- RDM
- TOD (enable it if you want RDM)
- Firmware uploading
- DMX input

To enable it create a config.h file with those constants defined:

```cpp
#ifndef CONFIG_H_
#define CONFIG_H_

// Enable TOD feature
#define ARTNET_FEATURE_TOD

// Enable RDM feature
#define ARTNET_FEATURE_RDM

// Enable firmware upload feature
#define ARTNET_FEATURE_FIRMWARE

// Enable input feature
#define ARTNET_FEATURE_INPUT

// Enable debug prints
// #define ARTNET_DEBUG

#endif
```

# Links

For an example program, see [ArtNetMbed](https://github.com/exmachina-dev/ArtNetMbed)
Code repository: [libartnet](https://github.com/exmachina-dev/libartnet)

Forked from [OpenLightingProject/libartnet](https://github.com/OpenLightingProject/libartnet)

See [authors file](AUTHORS) for contributors.

