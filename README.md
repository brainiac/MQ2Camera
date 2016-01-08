MQ2Camera
=========

MQ2 plugin to manipulate the game's third person camera.

Currently this provides a distance option that lets you zoom the camera further away from your character.

Usage
-----

* **/camera distance _distance_** - Set a new maximum distance. Cannot be lower than the default max camera distance.
* **/camera distance reset** - Reset the camera distance to the default
* **/camera info** - show the current settings for the camera distance

Updating
-------

The plugin uses pattern matching to find offsets at runtime. See [MQ2Camera.cpp](/MQ2Camera.cpp) for more info.


Screenshot
----------

![Screenshot of MQ2Camera in use to zoom the camera out really far](/screenshot.png?raw=true)
