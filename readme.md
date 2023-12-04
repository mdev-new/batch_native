```
==============================================================================
                                 Batch native                                 
==============================================================================

This is the documentation for the official Batch native modules.
For documentation on community-made modules, contact their respective authors.
For documentation on developing custom modules, refer to developer_docs.txt.

Written by MousieDev & Kenan238
Last update: 13th of September, 2023

=== Table of contents ===
=> Features
=> New environment variables
=> Example set-up
=> Credits
=> License


=== Features ===
- Fast & small
- Easy to setup in both new and existing projects
- Self injecting (no third-party exes required)
- Featuring:
  - mouse, keyboard and Xbox controller polling
  - Discord RPC
  - map renderer
  - audio player


=== New environment variables ===
-- GetInput --
(o)   mousexpos, mouseypos: mouse position measured in cells
(o)   click: mouse click ; left=1,right=2,middle=4
(o)   wheeldelta: up=-1,down=1 ; reset this manually once you process
(o)   keyspressed: is formatted like this: -keycode1-keycode2-keycode3- ; uses virtual key codes, not characters, keycode reference: https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes; that means keyboard layouts do not matter, the physical location of keys is considered
(o)   controller#: xbox controller with index # state ; # = number between 1 and 4
(o)   getinputInitialized: contains 1 if injection was successful
(i)   noresize: 1 to disable resizing
(i)   rasterx, rastery: turn on the raster font and set its size to x and y ; can be changed after injection
(i)   limitMouseX, limitMouseY: limit the x and y range of mouse, unsets mousexpos or mouseypos if out of range ; can be changed after injection
(i)   ctrl_deadzone: deadzone for the controllers' thumbsticks in %'s
(i)   getinput_tps: how many times should the main loop run per second

All input variables are *optional*

-- Discord RPC --
(i)   discordappid - your own app id (created on the discord developer portal)
(i+o) discordupdate - set this to anything on startup and when updating the rpc; gets nulled on update
(i)   discordstate - state (for example "Editing main.c")
(i)   discorddetails - details (for example "Line 50; column 100")
(i)   discordlargeimg - name of the asset that gets used as the large image
(i)   discordlargeimgtxt - text when user mouses over the large image
(i)   discordsmallimg - name of the asset that gets used as the small image
(i)   discordsmallimgtxt - text when user mouses over the small image

Input variables are NOT optional.

-- Map Renderer --
(i)   levelWidth - width of the level
(i)   levelHeight - height of the level
(i)   viewXoff - X offset of the camera
(i)   viewYoff - Y offset of the camera
(i)   mapFile - name of the file containing the current map

Input variables are NOT optional.

-- Audio player --
No environmental variables in use, only the pipes "\\.\pipe\BatAudQ0" .. "\\.\pipe\BatAudQ3"


i=in,o=out


=== Example setup ===
-- GetInput --

set /a mousexpos=mouseypos=click=wheeldelta=0 & :: Reset the variables
rundll32 getinput.dll,inject

-- Discord RPC --

set /a discordappid=0000000000000000000 &:: fill in your own obviously
set discordstate=Hello world
set discorddetails=Test
set discordlargeimg=big-img
set discordlargeimgtxt=Large image text
set discordsmallimg=small-img
set discordsmallimgtxt=Small image Test
rundll32 discord_rpc.dll,inject

-- Map renderer --

set /a levelWidth=113
set /a levelHeight=78
set /a viewYoff=0
set /a viewXoff=0
set mapFile=map.txt
rundll32 map_rndr.dll,inject

-- Audio player --

<nul set/p"=INSERT_FILENAME_HERE.mp3" > \\.\pipe\BatAudQx
where the 'x' can be a number between 0 and 3


=== Credits ===
Kenan238  - original getinput.exe
MousieDev - extensive rewrite, making it a dll, main maintainer
Yeshi     - intensive testing and bug reporting by usage in his games


=== License ===
Copyright (C) 2021-2023 batch_native authors.  All rights reserved.

In this license,
Software refers to any files related to batch_native.

By using the files distributed to you in any way, you agree with this license.

You have the permission to do pretty much whatever with the Software,
under a few following conditions:
a) Your forks including your changes are kept public.
b) If you are reusing code and not forking the repo, give an attribution, both in the code and in the documentation (if it exists).

Making an pull request to the original repository (https://github.com/mdev-new/batch_native) would be very much appreciated.

The Software is provided "as is", without any warranty of any kind.
In no event shall the authors or copyright holders be liable for any claim, damage or other liability.

We do reserve the right to alter this license at any time, without any prior
notice and by continuing to use the Software, you automatically agree with the latest
version of the license, published at https://github.com/mdev-new/batch_native/blob/master/_dist/batch_native.txt.
Thou may not alter this license.
```