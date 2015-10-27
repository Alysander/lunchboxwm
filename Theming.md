Themes
======

Creating a theme for Lunchbox is relatively straight forward. The first step is to copy an existing theme (such as "original") which can be found in the `theme` folder. The name of the subfolder is used as the name of the theme so when you launch Lunchbox to select the theme write that name after the command. For example to load the minimal theme run:

`./lunchbox minimal &`

Each theme contains the following files at a miminum but can also include a style for pallette windows (called utility windows) so that they can be customized separately:

```
 fonts
 menubar_active.png
 menubar_inactive.png
 menubar_normal.png
 menubar_regions
 popup_menu_active_focussed.png
 popup_menu_active_hover.png
 popup_menu_active.png
 popup_menu_inactive.png
 popup_menu_normal_focussed.png
 popup_menu_normal_hover.png
 popup_menu_normal.png
 popup_menu_regions
 program_frame_active_focussed.png
 program_frame_active.png
 program_frame_inactive.png
 program_frame_normal_focussed.png
 program_frame_normal.png
 program_frame_regions
```

To change the appearance of a component in a particular state you can simply edit these image files directly with an image editor provided you don't move or change the size or position of the widgets or the image itself. For example, to change the way the buttons on all frames look when they are pressed change the `program_frame_active.png` image and the `program_frame_active_focussed.png` image. The popup menu files provide the background for all the popup menus in Lunchbox and the menu bar files provide the background for the menubar items.

Lunchbox currently does not support hover mode but future versions will. To make your theme future proof include the additional the hover variations of each window type.

The next most important part of the theme is the region file. If you want to change the size or position of a widget for a particular frame type, you need to edit its corresponding regions file.

Each row in each region file starts with an identifier for the widget and is then followed by 4 integers. These correspond to the x, y, width and height of each widget. If the numbers are less than zero, the width of the entire component is added to that integer. In this way, values "wrap" around and can specify if widgets should be left or right aligned (or top or bottom if it is the y coordinate). It is also used to indicate what remainder of the window width or height they should be.

If a line is omitted, the corresponding widget will not be shown. For cases when the background of the widget need to be different from the position of the widget, a corresponding "tile" can be specified.

The font file determines the font size, colour (including transparency) and weight but is somewhat limited because different fonts are not yet available.

An example fonts file is shown below:
```
small  Sans 13.5
medium Sans 13.5
large  Sans 13.5
normal                 n  1.0   1.0   1.0   1.0
active                 b  1.0   1.0   1.0   1.0
inactive               n  0.5   0.5   0.5   1.0
normal_hover           n  1.0   1.0   1.0   1.0
active_hover           b  1.0   1.0   1.0   1.0
normal_focussed        n  1.0   1.0   1.0   1.0
active_focussed        b  1.0   1.0   1.0   1.0
normal_focussed_hover  n  1.0   1.0   1.0   1.0
active_focussed_hover  b  1.0   1.0   1.0   1.0
```
The three different font sizes are specified in the first three lines (as well as the font name). The font sizes are in points. The lines after that specify how fonts should be drawn on the different window types, whether they should be "normal" (n) or "bold" (b). The four numbers are floating point numbers between 0 and 1 that correspond the amount of red, green, blue and transparency (the alpha channel).

Finally, once you have made your new theme please share it at let us know!