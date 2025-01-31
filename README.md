# OpenXcom [![Workflow Status][workflow-badge]][actions-url]

[workflow-badge]: https://github.com/OpenXcom/OpenXcom/workflows/ci/badge.svg
[actions-url]: https://github.com/OpenXcom/OpenXcom/actions

OpenXcom is an open-source clone of the popular "UFO: Enemy Unknown" ("X-COM:
UFO Defense" in the USA release) and "X-COM: Terror From the Deep" videogames
by Microprose, licensed under the GPL and written in C++ / SDL.

See more info at the [website](https://openxcom.org)
and the [wiki](https://www.ufopaedia.org/index.php/OpenXcom).

Uses modified code from SDL\_gfx (LGPL) with permission from author.

## Realistic accuracy and cover system
[Video](https://www.youtube.com/watch?v=S9lrOClZVQQ)

Features:
* new, more predictable, shooting spread. Shots won't fly miles away from target, and in case when you've taken really good aim and missed - your bullet will zip an inch above targets ear 
* "Chance to hit" number now precisely correct - it works like rolling dice in D&D. If you get 5% accuracy number, it means that game rolls 1-100 and if it's less or equal 5 - you hit. If you get 100% shot accuracy - you'll get it one hundred percent sure.
* If you see the target - you can shoot it. Now for aimed shot or kneeled position x-com operative will raise his weapon to the eyes level. That means you can shoot above stone fence while kneeled. 
* Shot accuracy highly depends on how much of a target is exposed. If you've got just the tip of your head over the fence - that pesky muton have a merely 5% to shoot you. Generally it works in both ways, but you'll get additional 2% for kneeling (aliens can't do that). You can fire a single snap shot while kneeled, triggering enemy's return fire which he will most probably miss, then stand up for full height and shoot with better accuracy.
* Units aim to the middle of the exposed part, so shots spread around that point.
* Similar to "off-center shooting" option, firing unit will check three lines of fire, shifting weapon slightly left and right, and select variant where its target is more exposed
* Due to a less shooting spread, missed shots will often hit your cover, often destroyng it, which makes you exposed for next shots. So pay attention to that, some objects are more sturdy and can endure more damage
* If distance to a target is 10 tiles or less - you'll get additional 10% accuracy multiplier per tile. Same for snap/auto, but it's 5 tiles and +20% per tile
* If that is not enough to get 100% shot accuracy for a tile adjanced to enemy - instead of multiplier, as a bonus you'll get the difference between your accuracy and 100%, divided by number of tiles

## 'Rejected features' fork

This is a fork from official master of OXCE+ which allows some new features which
have been rejected by official OXCE+ developers. Currently there are 2 new features
implemented:

### More than 8 bases

Now, player can choose the limit of available bases to build using a new entry in
file 'openxcom.cfg': 

oxceMaxBases= maxNumBases

Default is 8 bases.
If more than the 8 visible bases, player can scroll bases miniview using RMB at the 
edges of the miniview. A blue arrow appears when there are more bases at either edge.
 
### Hangars reworked 
Every hangar and craft has a "hangarType" tag, so a Hangar with an specific "hangarType"
value can only store crafts with the same "hangarType". "hangaType" tag can be given 
at both facilities and crafts, in their corresponding YAML ruleset. If no tag is given, 
that craft/hangar will have a "-1" default tag, so their behavior will be the same
as in vanilla engine.
Another feature added is the possibility of allocate more than 1 craft at an specific
hangar facility, so all crafts will be shown at Basescape, and can be right-clicked
to go to craft screen. If more than craft is defined for an hangar facility, a new 
YAML tag "craftSlots" should be defined in the hangar ruleset. This tag will be 
followed by as many "position entries" as crafts are allowed in the hangar.
Every position is a "- [x, y, z]" entry, where x,y are position offesets respect to 
the center of the hangar sprite, and z is ignored. This way, modders can choose where
they want to place crafts in an hangar which allows more than one.  

An "example MOD" is included in "Examples" folder. Though this mod is a submod of the
fantastic master mod "The Xcom Files", you can adapt it to any other mod or vanilla 
game, which needed using differentiated hangars and/or more than 1 craft per hangar.

### Windows Binary

For convenience, a ZIP file with windows binary has been added to folder 'WindowsBinaries'.
 This exe has been tested to work in a Windows10 machine (64 bits), thought it has not been
intensively tested. It also include extra Language files needed for some specific features

[Link to Windows Binary](/WindowsBinaries/UnzipInOXCEFolder.zip)

You can just place the binary and folder in the game folder of the original one, and just click
it when you want start the game with these new features.

## Installation

OpenXcom requires a vanilla copy of the X-COM resources -- from either or both
of the original games.  If you own the games on Steam, the Windows installer
will automatically detect it and copy the resources over for you.

If you want to copy things over manually, you can find the Steam game folders
at:

    UFO: "Steam\SteamApps\common\XCom UFO Defense\XCOM"
    TFTD: "Steam\SteamApps\common\X-COM Terror from the Deep\TFD"

Do not use modded versions (e.g. with XcomUtil) as they may cause bugs and
crashes.  Copy the UFO subfolders to the UFO subdirectory in OpenXcom's data
or user folder and/or the TFTD subfolders to the TFTD subdirectory in OpenXcom's
data or user folder (see below for folder locations).

## Mods

Mods are an important and exciting part of the game.  OpenXcom comes with a set
of standard mods based on traditional XcomUtil and UFOExtender functionality.
There is also a [mod portal website](https://openxcom.mod.io/) with a thriving
mod community with hundreds of innovative mods to choose from.

To install a mod, go to the mods subdirectory in your user directory (see below
for folder locations).  Extract the mod into a new subdirectory.  WinZip has an
"Extract to" option that creates a directory whose name is based on the archive
name.  It doesn't really matter what the directory name is as long as it is
unique.  Some mods are packed with extra directories at the top, so you may
need to move files around inside the new mod directory to get things straighted
out.  For example, if you extract a mod to mods/LulzMod and you see something
like:

    mods/LulzMod/data/TERRAIN/
    mods/LulzMod/data/Rulesets/

and so on, just move everything up a level so it looks like:

    mods/LulzMod/TERRAIN/
    mods/LulzMod/Rulesets/

and you're good to go!  Enable your new mod on the Options -> Mods page in-game.

## Directory Locations

OpenXcom has three directory locations that it searches for user and game files:

<table>
  <tr>
    <th>Folder Type</th>
    <th>Folder Contents</th>
  </tr>
  <tr>
    <td>user</td>
    <td>mods, savegames, screenshots</td>
  </tr>
  <tr>
    <td>config</td>
    <td>game configuration</td>
  </tr>
  <tr>
    <td>data</td>
    <td>UFO and TFTD data files, standard mods, common resources</td>
  </tr>
</table>

Each of these default to different paths on different operating systems (shown
below).  For the user and config directories, OpenXcom will search a list of
directories and use the first one that already exists.  If none exist, it will
create a directory and use that.  When searching for files in the data
directory, OpenXcom will search through all of the named directories, so some
files can be installed in one directory and others in another.  This gives
you some flexibility in case you can't copy UFO or TFTD resource files to some
system locations.  You can also specify your own path for each of these by
passing a commandline argument when running OpenXcom.  For example:

    openxcom -data "$HOME/bin/OpenXcom/usr/share/openxcom"

or, if you have a fully self-contained installation:

    openxcom -data "$HOME/games/openxcom/data" -user "$HOME/games/openxcom/user" -config "$HOME/games/openxcom/config"

### Windows

User and Config folder:
- C:\Documents and Settings\\\<user\>\My Documents\OpenXcom (Windows 2000/XP)
- C:\Users\\\<user\>\Documents\OpenXcom (Windows Vista/7)
- \<game directory\>\user
- .\user

Data folders:
- C:\Documents and Settings\\\<user\>\My Documents\OpenXcom\data (Windows 2000/XP)
- C:\Users\\\<user\>\Documents\OpenXcom\data (Windows Vista/7/8)
- \<game directory\>
- . (the current directory)

### Mac OS X

User and Config folder:
- $XDG\_DATA\_HOME/openxcom (if $XDG\_DATA\_HOME is defined)
- $HOME/Library/Application Support/OpenXcom
- $HOME/.openxcom
- ./user

Data folders:
- $XDG\_DATA\_HOME/openxcom (if $XDG\_DATA\_HOME is defined)
- $HOME/Library/Application Support/OpenXcom (if $XDG\_DATA\_HOME is not defined)
- $XDG\_DATA\_DIRS/openxcom (for each directory in $XDG\_DATA\_DIRS if $XDG\_DATA\_DIRS is defined)
- /Users/Shared/OpenXcom
- . (the current directory)

### Linux

User folder:
- $XDG\_DATA\_HOME/openxcom (if $XDG\_DATA\_HOME is defined)
- $HOME/.local/share/openxcom (if $XDG\_DATA\_HOME is not defined)
- $HOME/.openxcom
- ./user

Config folder:
- $XDG\_CONFIG\_HOME/openxcom (if $XDG\_CONFIG\_HOME is defined)
- $HOME/.config/openxcom (if $XDG\_CONFIG\_HOME is not defined)

Data folders:
- $XDG\_DATA\_HOME/openxcom (if $XDG\_DATA\_HOME is defined)
- $HOME/.local/share/openxcom (if $XDG\_DATA\_HOME is not defined)
- $XDG\_DATA\_DIRS/openxcom (for each directory in $XDG\_DATA\_DIRS if $XDG\_DATA\_DIRS is defined)
- /usr/local/share/openxcom
- /usr/share/openxcom
- . (the current directory)

## Configuration

OpenXcom has a variety of game settings and extras that can be customized, both
in-game and out-game. These options are global and affect any old or new
savegame.

For more details please check the [wiki](https://ufopaedia.org/index.php/Options_(OpenXcom)).

## Development

OpenXcom requires the following developer libraries:

- [SDL](https://www.libsdl.org) (libsdl1.2)
- [SDL\_mixer](https://www.libsdl.org/projects/SDL_mixer/) (libsdl-mixer1.2)
- [SDL\_gfx](https://www.ferzkopp.net/wordpress/2016/01/02/sdl_gfx-sdl2_gfx/) (libsdl-gfx1.2), version 2.0.22 or later
- [SDL\_image](https://www.libsdl.org/projects/SDL_image/) (libsdl-image1.2)
- [yaml-cpp](https://github.com/jbeder/yaml-cpp), version 0.5.3 or later

The source code includes files for the following build tools:

- Microsoft Visual C++ 2010 or newer
- Xcode
- Make (see Makefile.simple)
- CMake

It's also been tested on a variety of other tools on Windows/Mac/Linux. More
detailed compiling instructions are available at the
[wiki](https://ufopaedia.org/index.php/Compiling_(OpenXcom)), along with
pre-compiled dependency packages.
