# Lunchbox Window Manager
A tiling window manager for X11

There are only a few dependencies required to build lunchbox:

- `libx11-dev`
- `libxcursor-dev`
- `libcairo2-dev`

To run the lunchbox window manager first login to X without a window manager using an xterm session.

If you don't have an xterm session you can install one with `sudo make install` in the project directory.
This copies the `lunchbox.desktop` file to `/usr/share/xsessions`.

Then run `make` and the `lunchbox` executable:

`./lunchbox &`

Open some other programs from the commandline as there is no inbuilt launcher. Desktop managers and
launchers (e.g., the old netbook-launcher package) will coexist easily with their own workspaces.

If you need to change between programs after they are open click on the program menu on the bottom LHS.
To exit, type exit from the xterm window after closing all the other windows.
