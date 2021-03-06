# About

(copied from the original README, which is included at the bottom)

Yeahlaunch is a very simple application launcher which puts a horizontal menu
on top of your screen.  The menu will be hidden and only a one pixel border will
be visible.  If you point on that border the menu will be shown in an animated 
way.

# Installation

Installation can be performed with:

```
make
su -c "make -C `pwd` install"
```

or, if you use `sudo`,

```
make
sudo make install
```

My `.yeahlaunchrc` now contains:

```
-bg=blue
Firefox=firefox
Thunderbird=thunderbird
=
XTerm=/usr/bin/xterm
=
OpenOffice=/opt/openoffice4/program/soffice
Writer=/opt/openoffice4/program/swriter
Calc=/opt/openoffice4/program/scalc
Math=/opt/openoffice4/program/smath
Base=/opt/openoffice4/program/sbase
Impress=/opt/openoffice4/program/simpress
Printers=/opt/openoffice4/program/spadmin
=
XSane=/usr/bin/xsane
XFig=/usr/bin/xfig -nosplash
GIMP=/usr/bin/gimp
XCalc=/usr/bin/xcalc
XMag=/usr/bin/xmag
```

# New in 0.4:

- If you have just an `=` on its own in the config file, yeahlaunch will build its
  own separator, so you don't need the weird

  ```
      =/bin/false
  ```

  line any more.  You can still use it if you want variable width separators,
  though.

- If you don't specify `-x` or `-rx`, on the command line or in the config file,
  then the launcher will center itself on the top of your screen.

- A missing configuration file no longer causes an annoying message.

- Minor code cleanups.

# New in 0.3:

`yeahlaunch` will now read a configuration file named `.yeahlaunchrc` in
your home directory.  The format is:

`name=value`

where lines that begin with a dash (`-`) are treated as command-line arguments
and everything else becomes a button title.

If you want a different configuration for different invocations (multihead,
different positions on the screen, etc.), you can pass `-c` on the command
line to read an alternate configuration file.  (Using `-c` in the config file
does nothing, though.)

Here's an example of my `.yeahlaunchrc`:

```
-x=600
-bg=blue
Firefox=firefox
    =/bin/false
VMWare=/usr/bin/vmplayer
    =/bin/false
OpenOffice=/opt/openoffice.org/program/soffice
Writer=/opt/openoffice.org/program/swriter
Calc=/opt/openoffice.org/program/scalc
Math=/opt/openoffice.org/program/smath
Base=/opt/openoffice.org/program/sbase
Impress=/opt/openoffice.org/program/simpress
Printers=/opt/openoffice.org/program/spadmin
    =/bin/false
XSane=/usr/bin/xsane
XFig=/usr/bin/xfig
```

The blanks act as spacers between "program groups."

# New in 0.2:

- The menu entry is highlighted when clicked (see `-afg` below).

- Right-clicking launches the command and hides yeahlaunch instantly.

- New placement option `-rx` to which yeahlaunch will be placed right aligned.
  Specify `-rx (your screen width here)` and yeahlaunch will be right aligned to the
  top right corner of your screen.

- Leaving yeahlaunch sideways hides it as well now.

# Original README

Yeahlaunch is a very simple application launcher which puts a horizontal menu
on top of your screen.  The menu will be hidden and only a one pixel border will
be visible.  If you point on that border the menu will be shown in an animated 
way.

Yeahlaunch has the following options:

```
-fg color     foreground color (defaults to white)
-afg color    highlight  color (defaults to yellow)
-bg color     background color (defaults to black)
-x number     x placement (defaults to 0)
-step number  step size for the animation(defaults to 3)
-fn font name (defaults to fixed)
-h            prints this help message
```

All other options on the command line are recognized in the following way:

`label command` (label is the string that is displayed, command is the command to execute)

This ~means~ used to mean you have to specify commands.
If a command contains spaces or a "-" you have to use quotes (see example
below).

This is what I have in my `.xinitrc`:

```
~/yeah/yeahlaunch/yeahlaunch -fn "*-aqui-*" -x 700 -fg "light steel blue" \
TVtime tvtime \
Opera opera \
Firefox firefox \
NEdit nedit \
Gimp "gimp-2.0" \
"  " "" \
Home rox  \
Yeah "rox ~/yeah" \
Vertikal "rox ~/vertikal" &
```

Installation is done by a simple `make; make install` (*therealbstern* adds:
this only works if you like building things as root, or your everyday user account can write
to `/usr/local/bin`).
